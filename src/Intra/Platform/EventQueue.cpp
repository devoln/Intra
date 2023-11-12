#include "EventQueue.h"

namespace Intra { INTRA_BEGIN

#ifdef _WIN32
WinSockPoller::WinSockPoller()
{
	for(auto& ev: mSocketEvents)
		ev = z_D::CreateEventW(nullptr, false, false, nullptr);
}

WinSockPoller::~WinSockPoller()
{
	for(auto& ev: mSocketEvents)
		z_D::CloseHandle(ev);
}

bool WinSockPoller::ScheduleCallback(CallbackPtr callback, HANDLE dstThread)
{
	return QueueUserAPC([](z_D::ULONG_PTR callbackPtr) {
		auto& callback = *reinterpret_cast<CallbackPtr>(callbackPtr);
		callback();
	}, dstThread, size_t(callback)) != 0;
}

bool WinSockPoller::Finish(HANDLE dstThread)
{
	return QueueUserAPC([](z_D::ULONG_PTR pollContextPtr) {
		auto& pollContext = *reinterpret_cast<WinSockPoller*>(pollContextPtr);
		pollContext.mFinished = true;
	}, dstThread, size_t(callback)) != 0;
}

LResult<int> WinSockPoller::ProcessEvents(Optional<TimeDelta> waitTimeout = {}) noexcept
{
	const auto milliseconds = waitTimeout? unsigned(Min(waitTimeout.IntMilliseconds(), MaxValue<uint32>)): MaxValue<uint32>;
	const auto eventIndex = z_D::WaitForMultipleObjectsEx(NumSocketGroups, mSocketEvents, false, milliseconds, true);
	if(eventIndex >= NumSocketGroups) return 0; // timeout, wait error, APC, or overlapped I/O completion

	Lock l(mMutex);
	auto& group = mSubscriptions[eventIndex];

	// More efficient but non-portable WinSock-only way to fill three fd_sets to pass to select
	const auto maxElemsPerFdset = 1 + group.size();
	DynArray<SOCKET> fdsets(maxElemsPerFdset * 3);
	auto& numReadable = fdsets[0],
		numWritable = fdsets[maxElemsPerFdset],
		numErrors = fdsets[2 * maxElemsPerFdset];
	for(auto& subscription: group)
	{
		const auto s = SOCKET(subscription.Fd);
		if(subscription.OnReadable) fdsets[++numReadable] = s;
		if(subscription.OnWritable) fdsets[maxElemsPerFdset + ++numWritable] = s;
		if(subscription.OnException) fdsets[2 * maxElemsPerFdset + ++numErrors] = s;
	}

	// Non-blocking select is expected to return some results for this socket group
	const z_D::timeval timeout = {};
	auto ret = z_D::select(1,
		reinterpret_cast<fd_set*>(&fdsets[0]),
		reinterpret_cast<fd_set*>(&fdsets[maxElemsPerFdset]),
		reinterpret_cast<fd_set*>(&fdsets[2 * maxElemsPerFdset]),
		reinterpret_cast<const timeval*>(&timeout)
	);
	if(ret < 0) return Error::Wrap(WSAGetLastError());

	// Call callbacks
	for(int callbackIndex = 0; callbackIndex < 3; callbackIndex++)
		for(size_t i = 1, n = fdsets[callbackIndex * maxElemsPerFdset]; i <= n; i++)
		{
			auto& subscription = subscriptionByFd(fdsets[i]);
			auto& callback = subscription.Callbacks[callbackIndex];
			callback();
			callback = nullptr;
		}
	INTRA_DEBUG_ASSERT(size_t(ret) == numReadable + numWritable + numErrors);
	return ret;
}

void WinSockPoller::Run() noexcept
{
	while(!mFinished)
	{
		// TODO: handle exceptions, log errors
		ProcessEvents();
	}
}

void WinSockPoller::CallOnSocketEvent(Socket& socket, CallbackPtr callback, int callbackIndex)
{
	const int fd = socket.Handle();
	Lock l(mMutex);
	const auto socketID = fd >> 2; // in Windows all handles including sockets are divisible by 4: lower bits are available for handle tagging
	const auto groupIndex = socketID % NumSocketGroups;
	auto& group = mSubscriptions[groupIndex];
	auto indexInGroup = group.size();
	if(mFdToIndex.size() > socketID && mFdToIndex[socketID])
		indexInGroup = mFdToIndex[socketID] - 1;
	mFdToIndex.Set(socketID, indexInGroup);
	if(group.size() <= indexInGroup)
		group.PushLast({.Fd = fd});

	auto& subscription = group[indexInGroup];
	subscription.Callbacks[callbackIndex] = callback;
	z_D::WSAEventSelect(SOCKET(fd),
		mSocketEvents[groupIndex], // TODO: to work with multiple providers sockets from different providers must be in different groups
		(subscription.OnReadable? 0x29: 0) | // FD_READ|FD_ACCEPT|FD_CLOSE
		(subscription.OnWritable? 0x12: 0) | // FD_WRITE|FD_CONNECT
		(subscription.OnError? 0x14: 0) // FD_OOB|FD_CONNECT (with error)
	);
}

WinSockPoller::Subscription& WinSockPoller::subscriptionByFd(int fd)
{
	const auto socketID = fd >> 2;
	const auto groupIndex = socketID % NumSocketGroups;
	auto& group = mSubscriptions[groupIndex];
	auto& res = group[mFdToIndex[socketID] - 1];
	INTRA_POSTCONDITION(res.Fd == fd);
	return res;
}


EventQueue::EventQueue(uint32 numConcurrentThreads):
	mCompletionPort(z_D::CreateIoCompletionPort(HANDLE(-1), nullptr, 0, numConcurrentThreads))
{}

EventQueue::~EventQueue()
{
	Finish();
	if(mEventThread.IsJoinable()) mEventThread.Join();
	z_D::CloseHandle(mCompletionPort);
}

void EventQueue::callOnSocketEvent(Socket& socket, CallbackPtr callback, int callbackIndex)
{
	initThread();
	auto callbackForEventThread = [this, callback] {ScheduleCallback(callback);}; // TODO: find a way to convert it to CallbackPtr
	mEventPoller.Unwrap().CallOnReadable(socket, callbackForEventThread);
}

// TODO: another way to emulate readiness API with pure IOCP is ReadFile(0 bytes) or WriteFile(1 byte)
//  asynchronously, set flag for extra written 1 byte, and check the flag in Send() method to skip 1 byte next time.
//  But this method doesn't support multiple writers to the same sockets and may produce redundant packets with TCP_NODELAY
//  Also this method only supports TCP, not UDP!
//  For UDP we never use asynchronous operations for sending, we can use ordinary send that either sends the whole packet or fails.
//  For reading we use ReadFile with biggest buffer.
// We can probably use it partially - only for reading from TCP to reduce the number of rescheduled read events from event thread.

void EventQueue::CallOnReadable(Socket& socket, CallbackPtr callback) {callOnSocketEvent(socket, callback, 0);}
void EventQueue::CallOnWritable(Socket& socket, CallbackPtr callback) {callOnSocketEvent(socket, callback, 1);}
void EventQueue::CallOnError(Socket& socket, CallbackPtr callback) {callOnSocketEvent(socket, callback, 2);}

bool EventQueue::ScheduleCallback(CallbackPtr callback)
{
	return z_D::PostQueuedCompletionStatus(mCompletionPort, 0, size_t(callback), nullptr) != 0;
}

bool EventQueue::Finish()
{
	if(IsFinished()) return true;
	if(mEventPoller)
	{
		mEventPoller.Unwrap().Finish(mEventThread.Handle());
		mEventPoller = {};
	}
	mFinished.Set(true);
	// this callback schedules itself until all the threads exit ProcessEvents()
	mFinishCallback = [this] {ScheduleCallback(mFinishCallback.get())};
	ScheduleCallback(mFinishCallback.get()); // start the scheduling chain
	return true;
}

LResult<int> EventQueue::ProcessEvents(Optional<TimeDelta> waitTimeout = {}) noexcept
{
	if(mFinished) return 0;
	const auto milliseconds = waitTimeout? unsigned(Min(waitTimeout.IntMilliseconds(), MaxValue<uint32>)): MaxValue<uint32>;
	unsigned long numEntriesRemoved = 1;
#if INTRA_BUILD_FOR_WINDOWS_XP // not alertable wait: can't receive APC
	z_D::OVERLAPPED_ENTRY completionPortEntries[1];
	completionPortEntries[0].Internal = 0;
	const bool ret = GetQueuedCompletionStatus(mCompletionPort, &completionPortEntries[0].NumBytesTransferred,
		&completionPortEntries[0].CompletionKey, &completionPortEntries[0].Overlapped, milliseconds) != 0;
#else
	z_D::OVERLAPPED_ENTRY completionPortEntries[5];
	const bool ret = 0 != GetQueuedCompletionStatusEx(mCompletionPort,
		reinterpret_cast<_OVERLAPPED_ENTRY*>(completionPortEntries),
		sizeof(completionPortEntries)/sizeof(completionPortEntries[0]),
		&numEntriesRemoved, milliseconds, true);
#endif
	if(!ret) return ErrorCode::LastError();
	for(uint32 i = 0; i < numEntriesRemoved; i++)
	{
		auto& entry = completionPortEntries[i];
		auto& callback = entry.CompletionKey?
			*reinterpret_cast<CallbackPtr>(entry.CompletionKey):
			*reinterpret_cast<OverlappedWithCallback*>(entry.Overlapped)->Callback;
		callback();
	}
	return numEntriesRemoved;
}

Result<EventQueue::TimerID> EventQueue::setTimer(TimerID timer, CallbackPtr callback, int64 value, bool abs, bool repeat)
{
	if(mFinished) return ErrorCode(ErrorCode::OperationCanceled);
	const bool newTimer = !timer;
	if(newTimer)
	{
		initThread();
		timer = Shared<TimerDesc>::New({
			.MyQueue = this,
			.Handle = z_D::CreateWaitableTimerW(nullptr, false, nullptr),
			.Callback = callback
		});
	}
	int64 fileTime = int64(uint64(value + 50) / 100);
	if(abs) fileTime += 116444736000000000LL;
	else fileTime = -fileTime;
	timer->Time = fileTime;
	timer->Period = repeat? long(uint64(value + 500000000) / 1000000000): 0;

	// Windows timers notify the same thread that set it previously via APC that thread can only receive if it is in alertable state.
	// Since we don't know if the current caller's thread is ever going to be in alertable state,
	//  we delegate this to EventQueue's own thread created for this purpose.
	// Then timer event handler schedules the callback to be run on any thread processing this EventQueue (I/O completion port).
	bool ok = 0 != QueueUserAPC([](z_D::ULONG_PTR timerHandle) {
		auto timer = Shared(Unsafe, reinterpret_cast<Shared<TimerDesc>::StorageHandle>(timerDesc));
		if(!timer->Handle) return; // FreeTimer has been called, don't continue and automatically release the reference
		z_D::SetWaitableTimer(timer->Handle,
			reinterpret_cast<_LARGE_INTEGER*>(&timer->Time), timer->Period,
			[](void* timerHandle) {
				auto timer = TimerID::ReconstructFromReleasedHandle(Unsafe, reinterpret_cast<TimerID::StorageHandle>(timerHandle));
				if(!timer->Handle) return; // FreeTimer has been called, don't call callback and automatically release the reference
				if(timer->Period) timer.ReleaseHandleToReconstructLater(Unsafe); // periodic timer ti going to get called again, avoid releasing the reference
				timer->MyQueue->ScheduleCallback(timer->Callback);
			}, timer.ReleaseHandleToReconstructLater(Unsafe), false);
	}, mEventThread.Handle(), size_t(timer.ReleaseHandleToReconstructLater(Unsafe)));
	if(!ok) return ErrorCode::LastError();
	return timer;
}

ErrorCode EventQueue::FreeTimer(TimerID id)
{
	auto timer = INTRA_MOVE(id.Desc);
	if(!z_D::CloseHandle(timer->Handle))
		return ErrorCode::LastError();
	timer->Handle = nullptr;
	return {};
}

void EventQueue::initThread()
{
	if(mEventThread.IsJoinable()) return;
	mEventPoller = WinSockPoller();
	mEventThread = WinThread([this] {mEventPoller.Unwrap().Run();});
}
#elif defined(__linux__)
EventQueue::EventQueue()
{
	mEpollFd = z_D::epoll_create(1);
	mEventFd = z_D::eventfd(0, 2048); // EFD_NONBLOCK
	z_D::epoll_event ev = {.events = 0x80000001, .data = MaxValue<uint64>}; // EPOLLIN|EPOLLET
	z_D::epoll_ctl(mEpollFd, 1, mEventFd, &ev); // CTL_ADD
}

EventQueue::~EventQueue()
{
	z_D::close(mEpollFd);
	z_D::close(mEventFd);
}

bool EventQueue::ScheduleCallback(CallbackPtr callback)
{
	uint64 x = uint64(callback);
	return z_D::write(mEventFd, &x, sizeof(x)) == sizeof(x);
}

bool EventQueue::Finish()
{
	mFinished.Set(true);
	z_D::epoll_event ev = {.events = 1, .data = MaxValue<uint64>}; // EPOLLIN
	z_D::epoll_ctl(mEpollFd, 3, mEventFd, &ev); // CTL_MOD; remove EPOLLET to notify all threads that are waiting on this epoll instead of only one
	// send null event that will do nothing and only needed to wake the threads
	return ScheduleCallback(CallbackPtr(1));
}

LResult<int> EventQueue::ProcessEvents(Optional<TimeDelta> waitTimeout = {}) noexcept
{
	if(mFinished.Get()) return 0;
	const auto milliseconds = waitTimeout? int(Min(waitTimeout.IntMilliseconds(), MaxValue<int>)): -1;
	z_D::epoll_event events[5];
	const int numEvents = z_D::epoll_wait(mEpollFd, reinterpret_cast<epoll_event*>(events), Length(events), milliseconds);
	if(numEvents < 0) return ErrorCode::LastError();
	int numCallbacksCalled = 0;
	for(int i = 0; i < numEvents; i++)
	{
		const auto index = events[i].data;
		if(index == MaxValue<uint64>) // ScheduleCallback or Finish
		{
			uint64 x = 0;
			while(z_D::read(mEventFd, &x, sizeof(x)) == sizeof(x))
			{
				if(x <= 1) continue;
				auto& callback = *CallbackPtr(x);
				callback();
				numCallbacksCalled++;
			}
			continue;
		}
		if(index >> 63) // timerfd
		{
			uint64 x = 0;
			if(z_D::read(mEventFd, &x, sizeof(x)) != sizeof(x)) continue;
		}
		constexpr char eventMasks[] = {9, 12, 8}; // EPOLLIN | EPOLLERR; EPOLLOUT | EPOLLERR; EPOLLERR
		CallbackPtr callbacks[3] = {};

		INTRA_SYNCHRONIZED(mMutex) // mSubscriptions may be reallocated when adding new fd in other threads
		{
			auto& subscription = mSubscriptions[index];
			for(int c = 0; c < 3; c++)
			{
				auto& callback = subscription.Callbacks[c];
				if(callback && (events[i].events & eventMasks[c]))
					Swap(callback, callbacks[c]);
			}
		}
		for(auto& callback: callbacks) (*callback)(), numCallbacksCalled++;
	}
	return numCallbacksCalled;
}

void EventQueue::CallOnReadable(Socket& socket, CallbackPtr callback) {reg(socket.Handle(), callback, 0);}
void EventQueue::CallOnWritable(Socket& socket, CallbackPtr callback) {reg(socket.Handle(), callback, 1);}
void EventQueue::CallOnError(Socket& socket, CallbackPtr callback) {reg(socket.Handle(), callback, 2);}

void EventQueue::reg(int fd, CallbackPtr callback, int callbackIndex)
{
	Lock l(mMutex);
	auto index = mSubscriptions.size();
	if(mFdToIndex.size() > fd && mFdToIndex[fd])
		index = mFdToIndex[fd] - 1;
	mFdToIndex.Set(fd, index);
	if(mSubscriptions.size() <= index)
		mSubscriptions.PushLast({});
	auto& subscription = mSubscriptions[index];
	subscription.Callbacks[callbackIndex] = callback;

	// it would be enough to do it only the first time but we don't track if fd was closed and opened again with the same fd
	z_D::epoll_event ev = {.events = 0x80000005, .data = index}; // EPOLLIN|EPOLLOUT|EPOLLET
	z_D::epoll_ctl(mEpollFd, 1, fd, &ev); // EPOLL_CTL_ADD; EEXIST is an expected error
}

ErrorCode EventQueue::FreeTimer(TimerID fd)
{
	if(z_D::close(int(size_t(fd))) < 0) // detaches automatically, no need for EPOLL_CTL_DEL
		return ErrorCode::LastError();
	return {};
}

Result<TimerID> EventQueue::setTimer(TimerID fd, int64 value, bool abs, bool repeat)
{
	const bool isNewTimer = !fd;
	if(isNewTimer)
	{
		fd = z_D::timerfd_create(!abs, 0); // abs? CLOCK_REALTIME: CLOCK_MONOTONIC
		reg(fd, callback, 0);
	}
	// NOTE: Linux implementation doesn't change abs for existing timer
	z_D::itimerspec desc = {};
	auto& ts = repeat? desc.it_interval: desc.it_value;
	ts = {.tvsec = time_t(value / 1000000000), .tv_nsec = long(value % 1000000000)};
	if(z_D::timerfd_settime(int(size_t(fd)), 0, reinterpret_cast<const ::itimerspec*>(&desc), nullptr) < 0)
		return ErrorCode::LastError();
	if(!isNewTimer) reg(int(size_t(fd)), callback, 0); // NOTE: setting timer and changing callback for existing timer are not an atomic operation
	return fd;
}
#elif INTRA_TARGET_IS_BSD
EventQueue::EventQueue(): mKqueueFd(z_D::kqueue()) {}
EventQueue::~EventQueue() {z_D::close(mKqueueFd);}

LResult<int> EventQueue::ProcessEvents(Optional<TimeDelta> waitTimeout = {}) noexcept
{
	if(mFinished.Get()) return 0;
	const auto timeout = waitTimeout.Or({}).To<z_D::timespec>();
	z_D::Kevent events[5];
	const int numEvents = z_D::kevent(mKqueueFd, nullptr, 0, events, int(Length(events)), waitTimeout? &timeout: nullptr);
	if(numEvents < 0)
		return ErrorCode::LastError();
	for(int i = 0; i < numEvents; i++)
	{
		if(events[i].filter == EvFilt::Write) // connect may return EINPROGRESS, after that we may get here
		{
			int err = 0;
			if(events[i].flags & EV_EOF)
				err = events[i].fflags; // connect failed
			// TODO: handle TCP connection result depending on `err` value
		}
		const auto callback = CallbackPtr(events[i].udata);
		if(callback) (*callback)();
	}
	return numEvents;
}

ErrorCode EventQueue::FreeTimer(TimerID id)
{
	z_D::Kevent ev = {
		.ident = int(size_t(id)), .filter = int16(z_D::EvFilt::Timer),
		.flags = 10 // EV_DELETE | EV_DISABLE
	};
	if(z_D::kevent(mKqueueFd, &ev, 1, nullptr, 0, nullptr) == -1)
		return ErrorCode::LastError();
	return {};
}

Result<TimerID> EventQueue::setTimer(TimerID id, int64 value, bool abs, bool repeat)
{
	if(!id) id = TimerID(int(++mNumTimersCreated));
	z_D::Kevent ev = {
		.ident = int(size_t(id)), .filter = int16(z_D::EvFilt::Timer),
		.flags = repeat? 5: 21, // EV_ADD | EV_ENABLE | [EV_ONESHOT]
		.fflags = abs? uint32(z_D::EvNote::AbsTime): 0,
		.udata = callback
	};
	if constexpr(!uint32(z_D::EvNote::AbsTime))
	{
		value -= SystemTimestamp::Now().Since1970.IntNanoseconds();
	}
	if constexpr(sizeof(ev.data) == sizeof(value) && z_D::EvNote::NSeconds != z_D::EvNote(0))
	{
		ev.data = value;
		ev.fflags |= uint32(z_D::EvNote::NSeconds);
	}
	else ev.data = index_t(Min(value / 1000000, MaxValue<index_t>));
	if(z_D::kevent(mKqueueFd, &ev, 1, nullptr, 0, nullptr) == -1)
		return ErrorCode::LastError();
	return id;
}

bool EventQueue::ScheduleCallback(CallbackPtr callback)
{
	reg(int(mNumCallbacksSent++), callback, z_D::EvFilt::User, 21, uint32(z_D::EvNote::Trigger)); // EV_ADD | EV_ENABLE | EV_ONESHOT
}

void EventQueue::CallOnReadable(Socket& socket, CallbackPtr callback) {reg(socket.Handle(), callback, z_D::EvFilt::Read, 33);} // EV_ADD | EV_CLEAR
void EventQueue::CallOnWritable(Socket& socket, CallbackPtr callback) {reg(socket.Handle(), callback, z_D::EvFilt::Write, 33);} // EV_ADD | EV_CLEAR
void EventQueue::CallOnError(Socket& socket, CallbackPtr callback) {
#ifndef __FreeBSD__
	reg(socket.Handle(), callback, z_D::EvFilt::Except, 33);
#endif
}

bool EventQueue::Finish()
{
	mFinished.Set(true);
	reg(int(mNumCallbacksSent++), callback, z_D::EvFilt::User, 5, uint32(z_D::EvNote::Trigger)); // EV_ADD | EV_ENABLE
}

void EventQueue::reg(int id, CallbackPtr callback, z_D::EvFilt filter, int flags, int fflags)
{
	const z_D::Kevent ev = {.ident = id, .filter = int16(filter), .flags = uint16(flags), .fflags = fflags, .udata = callback};
	z_D::kevent(mKqueueFd, &ev, 1, nullptr, 0, nullptr);
}
#endif

} INTRA_END
