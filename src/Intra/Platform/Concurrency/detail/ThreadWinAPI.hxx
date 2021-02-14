#include "IntraX/Concurrency/Thread.h"
#include "IntraX/Concurrency/Lock.h"
#include "IntraX/Concurrency/Mutex.h"

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

namespace Intra { INTRA_BEGIN
INTRA_IGNORE_WARN_COPY_MOVE_IMPLICITLY_DELETED
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
	HANDLE InterruptEvent = nullptr;
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
		if(Current != nullptr && !Current->ForceInterruptionDisabled.GetRelaxed())
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
			return WaitForMultipleObjects(2, objects, false, uint32(timeOutMs)) == WAIT_OBJECT_0;
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
#if defined(_WIN32) && defined(_MSC_VER) && defined(INTRA_DEBUG_ABI)
		// Set name of thread that can be seen in MSVC debugger.
		const uint32 id = ::GetThreadId(threadHandle);

		char buf[128];
		Span<char>(buf) << name.Take(127) << '\0';
		struct {uint32 dwType; const char* szName; uint32 dwThreadID; uint32 dwFlags;} info = {0x1000, buf, id, 0};

		INTRA_IGNORE_WARNS_MSVC(6320 6322 4201)
		__try {RaiseException(0x406D1388, 0, sizeof(info)/sizeof(ULONG_PTR), reinterpret_cast<ULONG_PTR*>(&info));}
		__except(EXCEPTION_EXECUTE_HANDLER) {}
#endif
	}

	Data(Func func)
	{
		Function = Move(func);
#ifndef INTRA_THREAD_NO_FULL_INTERRUPT
		InterruptEvent = CreateEventW(nullptr, false, false, nullptr);
#endif
#ifdef INTRA_NO_THREAD_CRT_INIT
		Thread = CreateThread(nullptr, ThreadStackSize, ThreadProc, this, 0, nullptr);
#else
		const auto threadInt = _beginthreadex(nullptr, ThreadStackSize, ThreadProc, this, 0, nullptr);
		Thread = threadInt != 0? HANDLE(threadInt): INVALID_HANDLE_VALUE;
#endif
		if(Thread == INVALID_HANDLE_VALUE)
		{
			IsDetached = true;
			return;
		}
#ifdef INTRA_DEBUG
		Id = ::GetThreadId(Thread);
		INTRA_SYNCHRONIZED(ThreadHashTableMutex)
			ThreadHashTable[Id % HASH_LEN] = this;
#endif
	}

	~Data()
	{
		if(Thread != INVALID_HANDLE_VALUE) CloseHandle(Thread);
#ifndef INTRA_THREAD_NO_FULL_INTERRUPT
		if(InterruptEvent != nullptr) CloseHandle(InterruptEvent);
#endif
#ifdef INTRA_DEBUG
		INTRA_SYNCHRONIZED(ThreadHashTableMutex)
		{
			if(ThreadHashTable[Id % HASH_LEN] == this)
				ThreadHashTable[Id % HASH_LEN] = nullptr;
		}
#endif
	}

	void Detach()
	{
		bool wasRunning = true;
		INTRA_SYNCHRONIZED(StateMutex)
		{
			if(!IsRunning.Get<MemoryOrder::Relaxed>()) wasRunning = false;
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
	if(handle != nullptr && !handle->ForceInterruptionDisabled.GetRelaxed())
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
	::Sleep(uint32(milliseconds));
	return true;
}
} INTRA_END
