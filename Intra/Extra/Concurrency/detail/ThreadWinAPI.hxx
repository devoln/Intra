#include "Concurrency/Thread.h"
#include "Concurrency/Lock.h"
#include "Concurrency/Mutex.h"

#include "ThreadCommonWinAPI.hxx"
#include "BasicThreadData.hxx"


#if defined(INTRA_NO_CRT) && !defined(INTRA_NO_THREAD_CRT_INIT)
#define INTRA_NO_THREAD_CRT_INIT
#endif

#ifndef INTRA_NO_THREAD_CRT_INIT
#include <process.h>
#endif

#include <xmmintrin.h>

#undef Yield

INTRA_BEGIN
INTRA_WARNING_DISABLE_COPY_MOVE_IMPLICITLY_DELETED
struct Thread::Data: detail::BasicThreadData
{
#ifdef INTRA_NO_THREAD_CRT_INIT
	static DWORD WINAPI ThreadProc(void* lpParam)
#else
	static unsigned __stdcall ThreadProc(void* lpParam)
#endif
	{
		const auto thread = Thread::Handle(lpParam);
		thread->ThreadFunc();
		return 0;
	}

	enum: size_t {ThreadStackSize = 1048576};

	HANDLE Thread;
#ifndef INTRA_THREAD_NO_FULL_INTERRUPT
	HANDLE InterruptEvent = null;
#endif

#ifdef INTRA_DEBUG
	enum {HASH_LEN = 127};
	DWORD Id = 0;
	static Mutex ThreadHashTableMutex;
	static Handle ThreadHashTable[HASH_LEN];
#endif

	void Interrupt()
	{
		BasicThreadData::Interrupt();
#ifndef INTRA_THREAD_NO_FULL_INTERRUPT
		if(!ForceInterruptionDisabled.GetRelaxed()) SetEvent(InterruptEvent);
#endif
	}

	bool Join(uint64 timeOutMs)
	{
#ifndef INTRA_THREAD_NO_FULL_INTERRUPT
		if(Current != null && !Current->ForceInterruptionDisabled.GetRelaxed())
		{
			if(Current->InterruptableAction()) return false;
			const HANDLE objects[] = {Thread, InterruptEvent};
			while(timeOutMs >= INFINITE - 1)
			{
				auto result = WaitForMultipleObjects(2, objects, false, INFINITE - 1);
				if(result == WAIT_OBJECT_0) return true;
				if(result != WAIT_TIMEOUT) return false;
				timeOutMs -= INFINITE - 1;
			}
			return WaitForMultipleObjects(2, objects, false, DWORD(timeOutMs)) == WAIT_OBJECT_0;
		}
#endif
		while(timeOutMs >= INFINITE - 1)
		{
			if(WaitForSingleObject(Thread, INFINITE - 1) == WAIT_OBJECT_0) return true;
			timeOutMs -= INFINITE - 1;
		}
		return WaitForSingleObject(Thread, DWORD(timeOutMs)) == WAIT_OBJECT_0;
	}

	bool Join()
	{
#ifndef INTRA_THREAD_NO_FULL_INTERRUPT
		if(Current && !Current->ForceInterruptionDisabled.GetRelaxed())
		{
			if(Current->InterruptableAction()) return false;
			const HANDLE objects[] = {Thread, InterruptEvent};
			return WaitForMultipleObjects(2, objects, false, INFINITE) == WAIT_OBJECT_0;
		}
#endif
		return WaitForSingleObject(Thread, INFINITE) == WAIT_OBJECT_0;
	}

	void SetName()
	{
#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows && defined(_MSC_VER))
		detail::SetNativeThreadName(Thread, Name);
#endif
	}

	Data(Func func)
	{
		Function = Move(func);
#ifndef INTRA_THREAD_NO_FULL_INTERRUPT
		InterruptEvent = CreateEventW(null, false, false, null);
#endif
#ifdef INTRA_NO_THREAD_CRT_INIT
		Thread = CreateThread(null, ThreadStackSize, ThreadProc, mHandle, 0, null);
#else
		Thread = HANDLE(_beginthreadex(null, ThreadStackSize, ThreadProc, this, 0, null));
#endif
		if(Thread == INVALID_HANDLE_VALUE)
		{
			IsDetached = true;
			return;
		}
#ifdef INTRA_DEBUG
		Id = detail::GetThreadId(Thread);
		INTRA_SYNCHRONIZED(ThreadHashTableMutex)
			ThreadHashTable[Id % HASH_LEN] = this;
#endif
	}

	~Data()
	{
		if(Thread != INVALID_HANDLE_VALUE)
			CloseHandle(Thread);
#ifndef INTRA_THREAD_NO_FULL_INTERRUPT
		if(InterruptEvent != null)
			CloseHandle(InterruptEvent);
#endif
#ifdef INTRA_DEBUG
		INTRA_SYNCHRONIZED(ThreadHashTableMutex)
		{
			if(ThreadHashTable[Id % HASH_LEN] == this)
				ThreadHashTable[Id % HASH_LEN] = null;
		}
#endif
	}

	void Detach()
	{
		bool wasRunning = true;
		INTRA_SYNCHRONIZED(StateMutex)
		{
			if(!IsRunning.GetRelaxed()) wasRunning = false;
			else IsDetached = true;
		}
		if(!wasRunning) delete this;
	}

	Thread::NativeHandle GetNativeHandle()
	{return Thread::NativeHandle(Thread);}
};

#ifdef INTRA_DEBUG
Mutex Thread::Data::ThreadHashTableMutex;
// Для получения всей информации о потоке по его ID в отладчике.
// При коллизии отладчик не сможет получить эту информацию.
Thread::Data* Thread::Data::ThreadHashTable[HASH_LEN]{};
#endif

void TThisThread::Yield() {YieldProcessor();}

bool TThisThread::Sleep(uint64 milliseconds)
{
#ifndef INTRA_THREAD_NO_FULL_INTERRUPT
	auto handle = Thread::Data::Current;
	if(handle != null && !handle->ForceInterruptionDisabled.GetRelaxed())
	{
		if(handle->InterruptableAction()) return false;

		while(milliseconds >= INFINITE - 1)
		{
			if(WaitForSingleObject(handle->InterruptEvent, INFINITE - 1) == WAIT_OBJECT_0) return false;
			milliseconds -= INFINITE - 1;
		}
		if(WaitForSingleObject(handle->InterruptEvent, DWORD(milliseconds)) == WAIT_OBJECT_0) return false;
		return true;
	}
#endif

	if(milliseconds == 0)
	{
		Yield();
		return true;
	}

	while(milliseconds >= INFINITE - 1)
	{
		::Sleep(INFINITE - 1);
		milliseconds -= INFINITE - 1;
	}
	::Sleep(DWORD(milliseconds));
	return true;
}
INTRA_END
