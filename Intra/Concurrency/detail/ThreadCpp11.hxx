#pragma once

#include "Concurrency/Thread.h"
#include "Concurrency/CondVar.h"
#include "Concurrency/Mutex.h"

#include "BasicThreadData.hxx"

#include "Utils/Finally.h"

#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
#include "ThreadCommonWinAPI.hxx"
#endif

INTRA_PUSH_DISABLE_ALL_WARNINGS
#include <thread>
#include <future>
#include <condition_variable>
INTRA_WARNING_POP

#undef Yield

INTRA_BEGIN
namespace Concurrency {

struct Thread::Data: detail::BasicThreadData
{
	std::thread Thread;
	SeparateCondVar CV;
	size_t NumWaiters = 0;

	Data(Func func)
	{
		Function = Move(func);
		Thread = std::thread(&BasicThreadData::ThreadFunc, this);
	}

	Data(const Data&) = delete;
	Data& operator=(const Data&) = delete;

	~Data()
	{
		//Деструктор вызывается только в том случае, когда на объект не осталось внешних ссылок.
		//Владеющий объект потока удалён или отсоединён, а сам поток завершил выполнение своей функции.
		//Но могут остаться потоки, которые вызвали Join до этого и ещё не успели проснуться и выйти оттуда.
		while(NumWaiters > 0) MakeLock(StateMutex); //Ждёт завершения Join
	}

	bool Join()
	{
		if(!Thread.joinable()) return !IsRunning.GetRelaxed();
		auto lck = MakeLock(StateMutex);
		NumWaiters++;
		INTRA_FINALLY{NumWaiters--;};
		if(CV.Wait(lck, [this](){return !IsRunning.GetRelaxed();}))
		{
			if(Thread.joinable()) Thread.detach();
			return true;
		}
		return false;
	}

	bool Join(uint64 timeOutMs)
	{
		if(!Thread.joinable()) return !IsRunning.GetRelaxed();
		auto lck = MakeLock(StateMutex);
		NumWaiters++;
		INTRA_FINALLY{NumWaiters--;};
		if(CV.WaitMs(lck, timeOutMs, [this]() {return !IsRunning.GetRelaxed();}))
		{
			if(Thread.joinable()) Thread.detach();
			return true;
		}
		return false;
	}

	void Detach()
	{
		if(IsDetached) return;
		if(Thread.joinable()) Thread.detach();

		bool wasRunning = true;
		INTRA_SYNCHRONIZED(StateMutex)
		{
			if(!IsRunning.GetRelaxed()) wasRunning = false;
			else IsDetached = true;
		}
		if(!wasRunning) delete this;
	}

	void SetName()
	{
#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows && defined(_MSC_VER))
		detail::SetNativeThreadName(HANDLE(Thread.native_handle()), Name);
#endif
	}

	NativeHandle GetNativeHandle()
	{return NativeHandle(Thread.native_handle());}

	void OnFinish() final
	{
		BasicThreadData::OnFinish();
		CV.NotifyAll();
	}
};

void TThisThread::Yield() {std::this_thread::yield();}

bool TThisThread::Sleep(uint64 milliseconds)
{
	const auto hndl = Thread::Data::Current;
	if(hndl == null)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
		return true;
	}
	auto lck = MakeLock(hndl->StateMutex);
	hndl->CV.WaitMs(lck, milliseconds, []() {return false;});
	return !IsInterrupted();
}

}}
