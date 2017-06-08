#pragma once

#include "Concurrency/Thread.h"
#include "BasicThreadData.hxx"

#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
#include "ThreadCommonWinAPI.hxx"
#endif

#include <thread>
#include <future>

#undef Yield

namespace Intra { namespace Concurrency {

struct Thread::Data: detail::BasicThreadData
{
	std::thread Thread;

	Data(Func func)
	{
		Function = Cpp::Move(func);
		Thread = std::thread(&BasicThreadData::ThreadFunc, this);
	}

	bool Join()
	{
		Thread.join();
		return true;
	}

	bool Join(uint timeOutMs)
	{
		auto future = std::async(std::launch::async, &std::thread::join, &Thread);
		return future.wait_for(std::chrono::milliseconds(timeOutMs)) != std::future_status::timeout;
	}

	void Detach()
	{
		Thread.detach();

		INTRA_SYNCHRONIZED_BLOCK(GlobalThreadMutex)
		{
			if(!IsRunning) delete this;
			else IsDetached = true;
		}
	}

	void SetName()
	{
#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows && defined(_MSC_VER))
		detail::SetNativeThreadName(HANDLE(Thread.native_handle()), Name);
#endif
	}

	NativeHandle GetNativeHandle()
	{return NativeHandle(Thread.native_handle());}

	~Data()
	{
		if(IsDetached) return;
		Interrupt();
		if(!Join()) Detach();
	}
};

void ThisThread::Yield() {std::this_thread::yield();}

bool ThisThread::Sleep(ulong64 milliseconds)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
	return true;
}

}}
