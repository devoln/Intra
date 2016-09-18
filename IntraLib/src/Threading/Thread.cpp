#include "Core/Core.h"
#include "Threading/Thread.h"

#if(INTRA_LIBRARY_THREADING==INTRA_LIBRARY_THREADING_Dummy)

namespace Intra {

struct Thread::Handle {};

void Thread::create_thread(const Thread::Func& func) {func();}
void Thread::delete_thread() {}
void Thread::Join() {}
bool Thread::Joinable() const {return true;}
void Thread::Detach() {}

void Thread::Yield() {}

struct Mutex::Handle {};
Mutex::Mutex(bool processPrivate) {(void)processPrivate;}
Mutex::~Mutex() {}
void Mutex::Lock() {}
bool Mutex::TryLock() {return true;}
void Mutex::Unlock() {}

}

#elif(INTRA_LIBRARY_THREADING==INTRA_LIBRARY_THREADING_CPPLIB)

#ifdef Yield
#undef Yield
#endif

namespace Intra {

struct Thread::Handle {std::thread t;};

void Thread::create_thread(const Thread::Func& func) {handle = new Thread::Handle{std::thread(func)};}

void Thread::delete_thread()
{
	if(handle==null) return;
	INTRA_ASSERT(!Joinable());
	delete handle;
}

void Thread::Join() {handle->t.join();}
bool Thread::Joinable() const {return handle!=null && handle->t.joinable();}

void Thread::Detach()
{
	if(handle==null) return;
	handle->t.detach();
	delete handle;
	handle = null;
}

void Thread::Yield() {}


struct Mutex::Handle {std::mutex mut;};
Mutex::Mutex() {handle = new Handle;}
Mutex::~Mutex() {delete handle;}
void Mutex::Lock() {handle->mut.lock();}
bool Mutex::TryLock() {return handle->mut.try_lock();}
void Mutex::Unlock() {handle->mut.unlock();}

}

#elif(INTRA_LIBRARY_THREADING==INTRA_LIBRARY_THREADING_WinAPI)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4668)
#endif

#include <Windows.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#ifdef Yield
#undef Yield
#endif

namespace Intra {

struct ThreadData
{
	HANDLE thread;
	bool isJoinable;
	bool isDetached;
	Thread::Func myFunction;
};

struct Thread::Handle: ThreadData {};

enum: size_t {ThreadStackSize=1048576};

static DWORD WINAPI ThreadProc(void* lpParam)
{
	ThreadData* data = (ThreadData*)lpParam;
	data->myFunction();
	data->thread = null;
	if(data->isDetached) delete data;
	return 0;
}

void Thread::create_thread(const Thread::Func& func)
{
	handle = (Handle*)new ThreadData;
	handle->isJoinable = true;
	handle->isDetached = false;
	handle->myFunction = func;
	handle->thread = CreateThread(null, ThreadStackSize, ThreadProc, handle, 0, null);
}

void Thread::delete_thread()
{
	if(handle==null) return;
	INTRA_ASSERT(!Joinable());
	delete (ThreadData*)handle;
}

void Thread::Join()
{
	WaitForSingleObject(handle->thread, INFINITE);
	handle->isJoinable = false;
}

bool Thread::Joinable() const {return handle!=null && handle->isJoinable;}

void Thread::Detach()
{
	if(handle==null) return;
	handle->isDetached=true;
	handle = null;
}

void Thread::Yield() {YieldProcessor();}


struct Mutex::Handle
{
	union
	{
		CRITICAL_SECTION cs;
		HANDLE mut;
	};
	bool process_private;
};

Mutex::Mutex(bool processPrivate)
{
	handle = new Handle;
	handle->process_private=processPrivate;
	if(processPrivate) (void)InitializeCriticalSectionAndSpinCount(&handle->cs, 20);
	else handle->mut = CreateMutexW(null, false, null);
}

Mutex::~Mutex()
{
	if(handle->process_private) DeleteCriticalSection(&handle->cs);
	else CloseHandle(handle->mut);
	delete handle;
}

void Mutex::Lock()
{
	if(handle->process_private) EnterCriticalSection(&handle->cs);
	else WaitForSingleObject(handle->mut, INFINITE);
}

bool Mutex::TryLock()
{
	if(handle->process_private) return TryEnterCriticalSection(&handle->cs)!=0;
	return WaitForSingleObject(handle->mut, 0) == WAIT_OBJECT_0;
}

void Mutex::Unlock()
{
	if(handle->process_private) LeaveCriticalSection(&handle->cs);
	else ReleaseMutex(handle->mut);
}

}

#elif(INTRA_LIBRARY_THREADING==INTRA_LIBRARY_THREADING_PThread)

#include <pthread.h>

#ifdef _MSC_VER
#pragma comment(lib, "pthread.lib")
#endif

namespace Intra {

struct ThreadData
{
	pthread_t thread;
	//bool isSuspended;
	bool isJoinable;
	bool isDetached;
	Thread::Func myFunction;
};

struct Thread::Handle: ThreadData {};

enum: size_t {ThreadStackSize=1048576};

static void* ThreadProc(void* lpParam)
{
	ThreadData* data = reinterpret_cast<ThreadData*>(lpParam);
	data->myFunction();
	//data->thread.p=null;
	if(data->isDetached) delete data;
	return null;
}

void Thread::create_thread(const Thread::Func& func)
{
	handle = static_cast<Handle*>(new ThreadData);
	handle->isJoinable = true;
	handle->isDetached = false;
	handle->myFunction = func;
	pthread_attr_t attribute;
	pthread_attr_init(&attribute);
	pthread_attr_setstacksize(&attribute, ThreadStackSize);
	pthread_create(&handle->thread, &attribute, ThreadProc, handle);
}

void Thread::delete_thread()
{
	if(handle==null) return;
	INTRA_ASSERT(!Joinable());
	delete static_cast<ThreadData*>(handle);
}

void Thread::Join()
{
	pthread_join(handle->thread, null);
	handle->isJoinable = false;
}

bool Thread::Joinable() const {return handle!=null && handle->isJoinable;}

void Thread::Detach()
{
	if(handle==null) return;
	pthread_detach(handle->thread);
	handle->isDetached = true;
	handle = null;
}

void Thread::Yield() {}


Mutex::Mutex(bool processPrivate): handle(null)
{
	pthread_mutex_t* mutex = new pthread_mutex_t;
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	if(!processPrivate) pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
	pthread_mutex_init(mutex, &attr);
	handle = reinterpret_cast<Handle*>(mutex);
}

Mutex::~Mutex()
{
	pthread_mutex_destroy(reinterpret_cast<pthread_mutex_t*>(handle));
	delete reinterpret_cast<pthread_mutex_t*>(handle);
}

void Mutex::Lock() {pthread_mutex_lock(reinterpret_cast<pthread_mutex_t*>(handle));}
bool Mutex::TryLock() {return pthread_mutex_trylock(reinterpret_cast<pthread_mutex_t*>(handle))!=0;}
void Mutex::Unlock() {pthread_mutex_unlock(reinterpret_cast<pthread_mutex_t*>(handle));}

}

#elif(INTRA_LIBRARY_THREADING==INTRA_LIBRARY_THREADING_SDL)

#error "INTRA_LIBRARY_THREADING_SDL is not implemented yet!"

#elif(INTRA_LIBRARY_THREADING==INTRA_LIBRARY_THREADING_Qt)

#error "INTRA_LIBRARY_THREADING_Qt is not implemented yet!"

#endif
