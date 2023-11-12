#pragma once

#include <Intra/Platform/Toolchain.h>
#include <Intra/Platform/Error.h>
#include <Intra/Platform/Concurrency.h>
#include <Intra/Platform/Socket.h>
#include <Intra/TypeErasure.h>
#include <Intra/Container/Compound.h>
#include <Intra/Container/Array.h>

namespace Intra { INTRA_BEGIN

// TODO: implement file system watcher events:
// Windows: CreateFile(..., FILE_FLAG_BACKUP_SEMANTICS, ...) + ReadDirectoryChangesW
// Linux: inotify: https://syntaxbug.com/367fd46a4d/
// BSD/Apple: kevent(EVFILT_FS)

#ifdef _WIN32
// Used as part of EventQueue on Windows to emulate epoll/kqueue for sockets.
class WinSockPoller
{
	using CallbackPtr = ICallback<void()>*;
	struct Subscription
	{
		int Fd;
		union
		{
			struct {CallbackPtr OnReadable, OnWritable, OnException;};
			CallbackPtr Callbacks[3];
		};
	};
	// 64 - is an OS limit, so we use multiple sockets per event, then do a non-blocking select among them
	enum {NumSocketGroups = 64};

	DynArray<Subscription> mSubscriptions[NumSocketGroups];
	DynArray<uint16> mFdToIndex; // translates (fd >> 2) into 1 + <index in mSubscriptions[(fd >> 2) % NumSocketGroups][index]>
	HANDLE mSocketEvents[NumSocketGroups];
	Atomic<bool> mFinished = false;
	Mutex mMutex;
public:
	// NOTE: In order to process all events, thread must be one of the threads that runs ProcessEvents or Run.
	//  Otherwise, callbacks from ScheduleCallback won't be called.
	WinSockPoller();

	WinSockPoller(WinSockPoller&&) = delete;
	WinSockPoller& operator=(WinSockPoller&&) = delete;

	~WinSockPoller();

	// NOTE: these calls force socket to become non-blocking
	void CallOnReadable(Socket& socket, CallbackPtr callback) {CallOnSocketEvent(socket, callback, 0);}
	void CallOnWritable(Socket& socket, CallbackPtr callback) {CallOnSocketEvent(socket, callback, 1);}
	void CallOnError(Socket& socket, CallbackPtr callback) {CallOnSocketEvent(socket, callback, 2);}

	void CallOnSocketEvent(Socket& socket, CallbackPtr callback, int callbackIndex);

	bool ScheduleCallback(CallbackPtr callback, HANDLE dstThread);
	bool Finish(HANDLE dstThread);
	bool IsFinished() const {return mFinished.Get();}

	LResult<int> ProcessEvents(Optional<TimeDelta> waitTimeout = {});
	void Run() {while(!IsFinished()) ProcessEvents();}

private:
	Subscription& subscriptionByFd(int fd);
};
#endif

class EventQueue
{
	using CallbackPtr = ICallback<void()>*;

#ifdef _WIN32
	HANDLE mCompletionPort;
	Mutex mMutex;

	struct TimerDesc
	{
		EventQueue* MyQueue;
		HANDLE Handle;
		CallbackPtr Callback;
		int64 Time;
		long Period;
	};

	// Create these objects lazily only when they are first needed (first socket attached or first timer created)
	Optional<WinSockPoller> mEventPoller;
	WinThread mEventThread;
	PCallable<void()> mFinishCallback; // storage for a callback scheduled on Finish() to wake each thread
	
	void callOnSocketEvent(Socket& socket, CallbackPtr callback, int callbackIndex);
	void initThread();
public:
	using TimerID = Shared<TimerDesc>;
	// EventQueue expects this extended struct instead of just OVERLAPPED
	struct OverlappedWithCallback
	{
		z_D::OVERLAPPED Overlapped;
		CallbackPtr Callback;
	};
	bool IsFinished() const {return bool(mFinishCallback);}

#elif defined(__linux__)
	using CallbackPtr = ICallback<void()>*;
	struct Subscription {CallbackPtr Callbacks[3];};

	DynArray<Subscription> mSubscriptions;
	DynArray<uint16> mFdToIndex; // translates fd into 1 + <index in mSubscriptions>
	int mEpollFd;
	int mEventFd;

	Mutex mMutex;
	Atomic<bool> mFinished = false;

	void reg(TimerID fd, CallbackPtr callback, int callbackIndex);
public:
	struct TimerID
	{
		int Fd = -1;
		explicit operator bool() const {return Fd == TimerID().Fd;}
	};

	bool IsFinished() const {return mFinished.Get();}
#elif INTRA_TARGET_IS_BSD
	int mKqueueFd;
	Atomic<bool> mFinished = false;
	Atomic<unsigned> mNumCallbacksSent = 0;
	Atomic<unsigned> mNumTimersCreated = 0;

	void reg(int id, CallbackPtr callback, z_D::EvFilt filter, int flags, int fflags = 0);
public:
	struct TimerID
	{
		int Id = 0;
		explicit operator bool() const {return Id == TimerID().Id;}
	};
#endif
	explicit EventQueue(uint32 numConcurrentThreads = 0);

	// NOTE: User must make sure that all threads waiting on EventQueue finish before EventQueue, otherwise there may be dangling references.
	~EventQueue();

	EventQueue(EventQueue&&) = delete;
	EventQueue& operator=(EventQueue&&) = delete;

	// NOTE: Plftform-specific behaviour: om Windows these calls force socket to become non-blocking
	void CallOnReadable(Socket& socket, CallbackPtr callback);
	void CallOnWritable(Socket& socket, CallbackPtr callback);
	void CallOnError(Socket& socket, CallbackPtr callback);

	bool ScheduleCallback(CallbackPtr callback);
	bool Finish();

	Result<TimerID> SetTimer(TimerID id, SystemTimestamp ts) {return setTimer(id, ts.Since1970.IntNanoseconds(), true, false);}
	Result<TimerID> SetTimer(TimerID id, TimeDelta delta, bool periodic) {return setTimer(id, delta.IntNanoseconds(), false, periodic);}
	ErrorCode FreeTimer(TimerID id);

	LResult<int> ProcessEvents(Optional<TimeDelta> waitTimeout = {});
	void Run() {while(!IsFinished()) ProcessEvents();}

private:
	Result<TimerID> setTimer(TimerID id, int64 value, bool abs, bool repeat);
};

} INTRA_END
