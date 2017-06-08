#include "Concurrency/Thread.h"
#include "Concurrency/Lock.h"
#include "Concurrency/Mutex.h"

#include "ThreadCommonWinAPI.hxx"
#include "BasicThreadData.hxx"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_MOVE_IMPLICITLY_DELETED

#if defined(INTRA_NO_CRT) && !defined(INTRA_NO_THREAD_CRT_INIT)
#define INTRA_NO_THREAD_CRT_INIT
#endif

#ifndef INTRA_NO_THREAD_CRT_INIT
#include <process.h>
#endif

#include <xmmintrin.h>

#undef Yield

namespace Intra { namespace Concurrency {

struct Thread::Data: detail::BasicThreadData
{
#ifdef INTRA_NO_THREAD_CRT_INIT
	static DWORD WINAPI ThreadProc(void* lpParam)
#else
	static unsigned __stdcall ThreadProc(void* lpParam)
#endif
	{
		Handle(lpParam)->ThreadFunc();
		return 0;
	}

	enum: size_t {ThreadStackSize = 1048576};

	HANDLE Thread;
	HANDLE InterruptEvent = null;

	bool InterruptableAction()
	{
#ifndef INTRA_THREAD_NO_FULL_INTERRUPT
		if(!IsInterrupted && InterruptEvent == null)
			InterruptEvent = CreateEventW(null, false, IsInterrupted, null);
#endif
		return IsInterrupted;
	}

	void Interrupt()
	{
		BasicThreadData::Interrupt();
		if(InterruptEvent != null)
			SetEvent(InterruptEvent);
	}

	bool Join(uint timeOutMs = INFINITE)
	{
#ifndef INTRA_THREAD_NO_FULL_INTERRUPT
		if(InterruptEvent != null)
		{
			InterruptableAction();
			const HANDLE objects[] = {Thread, InterruptEvent};
			return WaitForMultipleObjects(2, objects, false, timeOutMs) == WAIT_OBJECT_0;
		}
#endif
		return WaitForSingleObject(Thread, timeOutMs) == WAIT_OBJECT_0;
	}

	void SetName()
	{
#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows && defined(_MSC_VER))
		detail::SetNativeThreadName(Thread, Name);
#endif
	}

	Data(Func func)
	{
		Function = Cpp::Move(func);
#ifdef INTRA_NO_THREAD_CRT_INIT
		Thread = CreateThread(null, ThreadStackSize, ThreadProc, mHandle, 0, null);
#else
		Thread = HANDLE(_beginthreadex(null, ThreadStackSize, ThreadProc, this, 0, null));
#endif
		if(Thread == INVALID_HANDLE_VALUE)
			IsDetached = true;
	}

	~Data()
	{
		if(!IsDetached)
		{
			Interrupt();
			Join();
		}
		if(Thread != INVALID_HANDLE_VALUE)
			CloseHandle(Thread);
		if(InterruptEvent != null)
			CloseHandle(InterruptEvent);
	}

	void Detach()
	{
		INTRA_SYNCHRONIZED_BLOCK(GlobalThreadMutex)
		{
			if(!IsRunning) delete this;
			else IsDetached = true;
		}
	}

	NativeHandle GetNativeHandle()
	{return NativeHandle(Thread);}
};

void ThisThread::Yield() {YieldProcessor();}

bool ThisThread::Sleep(ulong64 milliseconds)
{
#ifndef INTRA_THREAD_NO_FULL_INTERRUPT
	auto handle = Thread::Data::Current;
	if(handle != null)
	{
		handle->InterruptableAction();

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

	while(milliseconds >= INFINITE-1)
	{
		::Sleep(INFINITE-1);
		milliseconds -= INFINITE-1;
	}
	::Sleep(DWORD(milliseconds));
	return true;
}

}}

INTRA_WARNING_POP
