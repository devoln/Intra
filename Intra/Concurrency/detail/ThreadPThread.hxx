#include "Concurrency/Thread.h"
#include "BasicThreadData.hxx"

#include "System/Stopwatch.h"

#include "Utils/Finally.h"

INTRA_PUSH_DISABLE_ALL_WARNINGS
#include <pthread.h>

#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
#include "ThreadCommonWinAPI.hxx"
#else
#include <sched.h>
#endif

#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_FreeBSD)
#include <pthread_np.h>
#endif

#if(INTRA_PLATFORM_OS != INTRA_PLATFORM_OS_Windows)
#include <unistd.h>
#endif
INTRA_WARNING_POP

#ifdef _MSC_VER
#pragma comment(lib, "pthread.lib")
#endif

#include "ThreadSleep.hxx"

INTRA_BEGIN
namespace Concurrency {

struct Thread::Data: detail::BasicThreadData
{
	enum: size_t {ThreadStackSize = 1048576};

	pthread_t Thread;
	SeparateCondVar CV;
	size_t NumWaiters = 0;

	Data(Func func)
	{
		Function = Move(func);

		pthread_attr_t attribute;
		pthread_attr_init(&attribute);
		pthread_attr_setstacksize(&attribute, ThreadStackSize);
		if(pthread_create(&Thread, &attribute, ThreadProc, this) != 0)
			IsDetached = true;
	}

	static void* ThreadProc(void* lpParam)
	{
		Handle(lpParam)->ThreadFunc();
		return null;
	}

	~Data()
	{
		pthread_detach(Thread);

		//Деструктор вызывается только в том случае, когда на объект не осталось внешних ссылок.
		//Владеющий объект потока удалён или отсоединён, а сам поток завершил выполнение своей функции.
		//Но могут остаться потоки, которые вызвали Join до этого и ещё не успели проснуться и выйти оттуда.
		while(NumWaiters > 0) MakeLock(StateMutex); //Ждёт завершения Join
	}

	bool Join()
	{
		auto lck = MakeLock(StateMutex);
		NumWaiters++;
		INTRA_FINALLY{NumWaiters--;};
		return CV.Wait(lck, [this]() {return !IsRunning.GetRelaxed();});
	}

	bool Join(uint64 timeOutMs)
	{
		auto lck = MakeLock(StateMutex);
		NumWaiters++;
		INTRA_FINALLY{NumWaiters--;};
		return CV.WaitMs(lck, timeOutMs, [this]() {return !IsRunning.GetRelaxed();});
	}

	void Detach()
	{
		bool wasRunning = true;
		INTRA_SYNCHRONIZED(StateMutex)
		{
			if(IsDetached) return;
			if(!IsRunning.GetRelaxed()) wasRunning = false;
			else IsDetached = true;
		}
		if(!wasRunning) delete this;
	}

	void SetName()
	{
#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Linux || INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Android)
		pthread_setname_np(Thread, String(Name.Take(15)).CStr());
#elif(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_FreeBSD)
		pthread_set_name_np(Thread, String(Name.Take(15)).CStr());
#elif(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows && defined(_MSC_VER))
		detail::SetNativeThreadName(HANDLE(GetNativeHandle()), Name);
#endif
	}

	NativeHandle GetNativeHandle()
	{
#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
		return NativeHandle(pthread_getw32threadhandle_np(Thread));
#else
		return NativeHandle(&Thread);
#endif
	}

	void OnFinish() final
	{
		BasicThreadData::OnFinish();
		CV.NotifyAll();
	}

	Data(const Data&) = delete;
	Data& operator=(const Data&) = delete;
};

void TThisThread::Yield()
{
#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
	YieldProcessor();
#else
	sched_yield();
#endif
}

bool TThisThread::Sleep(uint64 milliseconds)
{
	const auto hndl = Thread::Data::Current;
	if(hndl == null || !ThisThread.IsInterruptionEnabled())
	{
		ThisThreadSleep(milliseconds);
		return true;
	}
	auto lck = MakeLock(hndl->StateMutex);
	hndl->CV.WaitMs(lck, milliseconds, []() {return false;});
	return !IsInterrupted();
}

}}
