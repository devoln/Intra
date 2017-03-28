#include "Thread/Thread.h"
#include "Platform/CppWarnings.h"
#include "Platform/PlatformInfo.h"
#include "Platform/Debug.h"
#include "Platform/Runtime.h"
#include "Thread/Atomic.h"
#include "Container/Utility/SparseArray.h"
#include "Container/Associative/LinearMap.h"
#include "Memory/SmartRef/Unique.h"
#include "Utils/Callable.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra {

struct ThreadData
{
	//Удаляется и устанавливается в null по завершении выполнения
	Utils::ICallable<void()>* Function;

	bool IsDetached;
};

static Mutex gThreadMutex;
static SparseArray<ThreadData, ushort> gThreadData;

}

#if(INTRA_LIBRARY_THREADING==INTRA_LIBRARY_THREADING_Dummy)

namespace Intra {

void Thread::create_thread(Thread::Func func) {func();}
void Thread::delete_thread() {}
bool Thread::Join() {return true;}
bool Thread::Join(uint timeoutMs) {return true;}
bool Thread::IsRunning() const {return false;}
void Thread::Detach() {}
Thread::NativeHandle Thread::GetNativeHandle() const {return null;}

uint Thread::CurrentId() {return 0;}

void Thread::Yield() {}

Mutex::Mutex(bool processPrivate) {(void)processPrivate;}
Mutex::~Mutex() {}
void Mutex::Lock() {}
bool Mutex::TryLock() {return true;}
void Mutex::Unlock() {}

}

#elif(INTRA_LIBRARY_THREADING==INTRA_LIBRARY_THREADING_CPPLIB)

#include <thread>
#include <future>

#ifdef Yield
#undef Yield
#endif

namespace Intra {

struct Thread::Data {std::thread t;};

static LinearMap<std::thread::id, uint> gIdMap;

void Thread::create_thread(Thread::Func func)
{
	ushort id;
	{
		auto locker = gThreadMutex.Locker();
		gThreadData.Add({func.ReleaseCallable(), false}, &id);
	}
	mId = id;
	mHandle = new Thread::Data{std::thread([id]() {
		{
			auto locker = gThreadMutex.Locker();
			gIdMap[std::this_thread::get_id()] = id;
		}
		{
			Unique<Utils::ICallable<void()>> func = gThreadData[id].Function;
			func->Call();
		}
		{
			auto locker = gThreadMutex.Locker();
			gThreadData[id].Function = null;
			if(gThreadData[id].IsDetached) gThreadData.Remove(id);
		}
	})};
}

void Thread::delete_thread()
{
	if(mHandle==null) return;
	Join();
	delete mHandle;
}

bool Thread::Join()
{
	mHandle->t.join();
	mJoined = true;
	return true;
}

bool Thread::Join(uint timeoutMs)
{
	if(mHandle==null || mJoined) return true;
	auto future = std::async(std::launch::async, &std::thread::join, &mHandle->t);
	if(future.wait_for(std::chrono::seconds(5)) == std::future_status::timeout)
		return false;
	mJoined = true;
	return true;
}

bool Thread::IsRunning() const
{
	if(mHandle==null || mJoined) return false;
	mJoined = gThreadData[ushort(mId)].Function == null;
	return !mJoined;
}

void Thread::Detach()
{
	if(mHandle==null) return;
	mHandle->t.detach();
	delete mHandle;
	mHandle = null;
	auto locker = gThreadMutex.Locker();
	if(gThreadData[ushort(mId)].Function==null)
		gThreadData.Remove(ushort(mId));
	else gThreadData[ushort(mId)].IsDetached = true;
}

Thread::NativeHandle Thread::GetNativeHandle() const
{
	if(mHandle==null) return null;
	return NativeHandle(mHandle->t.native_handle());
}

uint Thread::CurrentId()
{
	auto id = std::this_thread::get_id();
	auto locker = gThreadMutex.Locker();
	return gIdMap.Get(id, 0);
}

void Thread::Yield() {std::this_thread::yield();}


struct Mutex::Data {std::mutex mut;};
Mutex::Mutex(bool processPrivate) {mHandle = new Data;}
Mutex::~Mutex() {delete mHandle;}
void Mutex::Lock() {mHandle->mut.lock();}
bool Mutex::TryLock() {return mHandle->mut.try_lock();}
void Mutex::Unlock() {mHandle->mut.unlock();}

}

#elif(INTRA_LIBRARY_THREADING==INTRA_LIBRARY_THREADING_WinAPI)

#if defined(INTRA_NO_CRT) && !defined(INTRA_NO_THREAD_CRT_INIT)
#define INTRA_NO_THREAD_CRT_INIT
#endif

#ifndef INTRA_NO_THREAD_CRT_INIT
#include <process.h>
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4668)
#endif

#include <Windows.h>
#include <xmmintrin.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#ifdef Yield
#undef Yield
#endif

namespace Intra {

enum: size_t {ThreadStackSize=1048576};

#ifdef INTRA_NO_THREAD_CRT_INIT
static DWORD WINAPI ThreadProc(void* lpParam)
#else
static unsigned __stdcall ThreadProc(void* lpParam)
#endif
{
	{
		Unique<Utils::ICallable<void()>> threadFunc = static_cast<Utils::ICallable<void()>*>(lpParam);
		threadFunc->Call();
	}
#ifdef INTRA_NO_THREAD_CRT_INIT
	ExitThread(0);
#else
	_endthreadex(0);
#endif
	return 0;
}

void Thread::create_thread(Thread::Func func)
{
	if(func==null)
	{
		mHandle = null;
		return;
	}
#ifdef INTRA_NO_THREAD_CRT_INIT
	mHandle = Handle(CreateThread(null, ThreadStackSize, ThreadProc, func.ReleaseCallable(), 0, null));
#else
	mHandle = Handle(_beginthreadex(null, ThreadStackSize, ThreadProc, func.ReleaseCallable(), 0, null));
#endif
	if(Handle(mHandle) == INVALID_HANDLE_VALUE)
	{
		mHandle = null;
		return;
	}
	mId = GetThreadId(HANDLE(mHandle));
}

void Thread::delete_thread()
{
	Join();
	CloseHandle(HANDLE(mHandle));
}

bool Thread::Join(uint timeoutMs)
{
	if(mHandle == null || mJoined) return true;
	mJoined = (WaitForSingleObject(HANDLE(mHandle), timeoutMs) == WAIT_OBJECT_0);
	return mJoined;
}

bool Thread::Join() {return Join(INFINITE);}

bool Thread::IsRunning() const
{
	if(mHandle == null || mJoined) return false;
	mJoined = (WaitForSingleObject(HANDLE(mHandle), 0) != WAIT_OBJECT_0);
	return mJoined;
}

void Thread::Detach()
{
	if(mHandle==null) return;
	CloseHandle(HANDLE(mHandle));
	mHandle = null;
}

uint Thread::CurrentId() {return GetCurrentThreadId();}

void Thread::Yield() {YieldProcessor();}

Thread::NativeHandle Thread::GetNativeHandle() const {return NativeHandle(mHandle);}


struct Mutex::Data
{
	union
	{
		CRITICAL_SECTION cs;
		HANDLE mut;
	};
	bool processPrivate;
};

Mutex::Mutex(bool processPrivate)
{
	mHandle = new Data;
	mHandle->processPrivate=processPrivate;
	if(processPrivate) (void)InitializeCriticalSectionAndSpinCount(&mHandle->cs, 20);
	else mHandle->mut = CreateMutexW(null, false, null);
}

Mutex::~Mutex()
{
	if(mHandle->processPrivate) DeleteCriticalSection(&mHandle->cs);
	else CloseHandle(mHandle->mut);
	delete mHandle;
}

void Mutex::Lock()
{
	if(mHandle->processPrivate) EnterCriticalSection(&mHandle->cs);
	else WaitForSingleObject(mHandle->mut, INFINITE);
}

bool Mutex::TryLock()
{
	if(mHandle->processPrivate) return TryEnterCriticalSection(&mHandle->cs)!=0;
	return WaitForSingleObject(mHandle->mut, 0) == WAIT_OBJECT_0;
}

void Mutex::Unlock()
{
	if(mHandle->processPrivate) LeaveCriticalSection(&mHandle->cs);
	else ReleaseMutex(mHandle->mut);
}

}

#elif(INTRA_LIBRARY_THREADING==INTRA_LIBRARY_THREADING_PThread)

#include <pthread.h>
#include <sched.h>

#ifdef _MSC_VER
#pragma comment(lib, "pthread.lib")
#endif

namespace Intra {

enum: size_t {ThreadStackSize=1048576};

static void* ThreadProc(void* lpParam)
{
	ushort id = ushort(reinterpret_cast<size_t>(lpParam));
	{
		Unique<Utils::ICallable<void()>> threadFunc = Meta::Move(gThreadData[id].Function);
		threadFunc->Call();
	}
	{
		auto locker = gThreadMutex.Locker();
		gThreadData[id].Function = null;
		if(gThreadData[id].IsDetached) gThreadData.Remove(id);
	}
	return null;
}

void Thread::create_thread(Thread::Func func)
{
	pthread_attr_t attribute;
	pthread_attr_init(&attribute);
	pthread_attr_setstacksize(&attribute, ThreadStackSize);
	ushort id;
	{
		auto locker = gThreadMutex.Locker();
		gThreadData.Add({func.ReleaseCallable(), false}, &id);
	}
	mId = id;
	pthread_create(reinterpret_cast<pthread_t*>(&mHandle), &attribute, ThreadProc, reinterpret_cast<void*>(id));
}

void Thread::delete_thread() {Join();}

#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_MacOS || INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Android)
struct waitData
{
	pthread_t waitID;
	int done;
};

static void* join_timeout_helper(void* arg)
{
	waitData* data = static_cast<waitData*>(arg);
	pthread_join(data->waitID, null);
	data->done = true;
	return null;
}

bool Thread::Join(uint timeoutMs)
{
	if(mHandle == null || mJoined) return true;
	Timer tim;
	pthread_t id;
	waitData data = {wid, false};
	if(pthread_create(&id, null, join_timeout_helper, &data) != 0) return -1;
	
	while(!data.done && timer.GetTimeMs()<timeoutNs);
		Timer::Sleep(10);
	if(!data.done) pthread_cancel(id);
	pthread_join(id, null);
	if(data.done) Join();
	return mJoined;
}

#else

bool Thread::Join(uint timeoutMs)
{
	if(mHandle == null || mJoined) return true;
	timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += timeoutMs/1000;
	ts.tv_nsec += (timeoutMs-timeoutMs/1000)*1000000;
	if(ts.tv_nsec>=1000000000)
	{
		ts.tv_nsec -= 1000000000;
		ts.tv_sec++;
	}
	mJoined = (pthread_timedjoin_np(pthread_t(mHandle), null, &ts) == 0);
	return mJoined;
}

#endif

bool Thread::IsRunning() const
{
	if(mHandle == null || mJoined) return false;
	mJoined = (gThreadData[ushort(mId)].Function == null);
	return !mJoined;
}

bool Thread::Join()
{
	if(mHandle == null || mJoined) return true;
	mJoined = (pthread_join(pthread_t(mHandle), null) == 0);
	return mJoined;
}

void Thread::Detach()
{
	if(mHandle==null) return;
	pthread_detach(pthread_t(mHandle));
	mHandle = null;

	auto locker = gThreadMutex.Locker();
	if(gThreadData[ushort(mId)].Function == null)
		gThreadData.Remove(ushort(mId));
	else gThreadData[ushort(mId)].IsDetached = true;
	mId = 0;
}

void Thread::Yield() {sched_yield();}

Thread::NativeHandle Thread::GetNativeHandle() const {return NativeHandle(mHandle);}


Mutex::Mutex(bool processPrivate): mHandle(null)
{
	pthread_mutex_t* mutex = new pthread_mutex_t;
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	if(!processPrivate) pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
	pthread_mutex_init(mutex, &attr);
	mHandle = Handle(mutex);
}

Mutex::~Mutex()
{
	pthread_mutex_destroy(reinterpret_cast<pthread_mutex_t*>(mHandle));
	delete reinterpret_cast<pthread_mutex_t*>(mHandle);
}

void Mutex::Lock() {pthread_mutex_lock(reinterpret_cast<pthread_mutex_t*>(mHandle));}
bool Mutex::TryLock() {return pthread_mutex_trylock(reinterpret_cast<pthread_mutex_t*>(mHandle))!=0;}
void Mutex::Unlock() {pthread_mutex_unlock(reinterpret_cast<pthread_mutex_t*>(mHandle));}

}

#elif(INTRA_LIBRARY_THREADING==INTRA_LIBRARY_THREADING_SDL)

#error "INTRA_LIBRARY_THREADING_SDL is not implemented yet!"

#elif(INTRA_LIBRARY_THREADING==INTRA_LIBRARY_THREADING_Qt)

#error "INTRA_LIBRARY_THREADING_Qt is not implemented yet!"

#endif

INTRA_WARNING_POP
