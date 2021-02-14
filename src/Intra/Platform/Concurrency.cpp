#include "Thread.h"
#include "Intra/Concurrency/Atomic.h"
#include "IntraX/System/Runtime.h"

#if(INTRA_LIBRARY_THREAD != INTRA_LIBRARY_THREAD_None)

#ifdef _WIN32
#include "detail/ThreadCommonWinAPI.hxx"
#else
#include <pthread.h>
#endif

namespace Intra { INTRA_BEGIN
INTRA_IGNORE_WARN_COPY_MOVE_IMPLICITLY_DELETED
namespace detail {

struct BasicThreadData
{
	Thread::Func Function;
	bool IsDetached = false;
	Atomic<bool> IsRunning{true};
	Atomic<bool> IsInterrupted{false};
	String Name;
	Mutex StateMutex;

#ifndef INTRA_THREAD_NO_FULL_INTERRUPT
	Atomic<bool> ForceInterruptionDisabled{false};
#endif
#if !defined(INTRA_THREAD_NO_FULL_INTERRUPT) || defined(INTRA_DEBUG)
	SeparateCondVar* WaitedCondVar = nullptr;
#endif

	virtual ~BasicThreadData() {}

	bool InterruptableAction()
	{
		return IsInterrupted.Get();
	}

	void Interrupt()
	{
		IsInterrupted.Set(true);
#ifndef INTRA_THREAD_NO_FULL_INTERRUPT
		if(ForceInterruptionDisabled.Get()) return;
		INTRA_SYNCHRONIZED(StateMutex)
		{
			if(WaitedCondVar)
				WaitedCondVar->NotifyAll();
		}
#endif
	}

	void SetName() {}

	void ThreadFunc()
	{
		Current = Thread::Handle(this);

		Function();
		Function = nullptr;

		bool wasDetached = false;
		INTRA_SYNCHRONIZED(StateMutex)
		{
			OnFinish();
			wasDetached = IsDetached;
		}
		if(wasDetached) delete this;
	}

	virtual void OnFinish()
	{
		IsRunning.Set(false);
	}

	static thread_local Thread::Handle Current;
};
thread_local Thread::Handle BasicThreadData::Current = nullptr;

}
} INTRA_END

#if(INTRA_LIBRARY_THREAD == INTRA_LIBRARY_THREAD_WinAPI)
#include "detail/ThreadWinAPI.hxx"
#elif(INTRA_LIBRARY_THREAD == INTRA_LIBRARY_THREAD_PThread)
#include "detail/ThreadSleep.hxx"
#include "detail/ThreadPThread.hxx"
#endif



namespace Intra { INTRA_BEGIN
Thread::Thread(Func func):
	mHandle(new Data(Move(func)))
{
	if(mHandle->IsDetached)
		mHandle = nullptr;
}

Thread::Thread(decltype(nullptr)) noexcept {}

Thread::Thread(Thread&&) noexcept = default;

Thread::~Thread()
{
	if(mHandle == nullptr) return;
	Interrupt();
	if(!Join()) Detach();
}

Thread& Thread::operator=(Thread&&) = default;

bool Thread::Join()
{return mHandle == nullptr || mHandle->Join();}

bool Thread::JoinMs(uint64 timeout)
{return mHandle == nullptr || mHandle->Join(timeout);}

void Thread::Interrupt()
{
	if(mHandle == nullptr) return;
	mHandle->Interrupt();
}

bool Thread::IsInterrupted() const
{
	if(mHandle == nullptr) return false;
	return mHandle->IsInterrupted.GetRelaxed();
}

bool Thread::IsRunning() const
{return mHandle != nullptr && mHandle->IsRunning.GetRelaxed();}

void Thread::SetName(StringView name)
{
	if(mHandle == nullptr) return;
	mHandle->Name = name;
	mHandle->SetName();
}

StringView Thread::Name() const
{
	if(mHandle == nullptr) return nullptr;
	return mHandle->Name;
}

void Thread::Detach()
{
	if(mHandle == nullptr) return;
	mHandle.Release()->Detach();
}

Thread::NativeHandle Thread::GetNativeHandle() const
{
	return mHandle == nullptr? nullptr:
		mHandle->GetNativeHandle();
}

bool Thread::IsInterruptionEnabled() const
{
#ifndef INTRA_THREAD_NO_FULL_INTERRUPT
	return !mHandle->ForceInterruptionDisabled.GetRelaxed();
#else
	return false;
#endif
}

#ifndef INTRA_THREAD_NO_FULL_INTERRUPT
void Thread::allowInterruption(bool allow)
{
	mHandle->ForceInterruptionDisabled.Set(!allow);
}
#endif

#ifdef INTRA_DEBUG
Thread::Handle TThisThread::getHandle()
{
	return Thread::Data::Current;
}
#endif

Thread::NativeHandle TThisThread::NativeHandle()
{
#ifdef _WIN32
	return Thread::NativeHandle(GetCurrentThread());
#else
	thread_local pthread_t self = pthread_self();
	return Thread::NativeHandle(&self);
#endif
}

void TThisThread::Interrupt()
{
	if(Thread::Data::Current == nullptr) return;
	Thread::Data::Current->Interrupt();
}

bool TThisThread::IsInterrupted()
{
	if(Thread::Data::Current == nullptr) return false;
	return Thread::Data::Current->IsInterrupted.GetRelaxed();
}

bool TThisThread::IsInterruptionEnabled()
{
#ifndef INTRA_THREAD_NO_FULL_INTERRUPT
	if(Thread::Data::Current == nullptr) return false;
	return !Thread::Data::Current->ForceInterruptionDisabled.GetRelaxed();
#else
	return false;
#endif
}

StringView TThisThread::Name()
{
	return Thread::Data::Current == nullptr? nullptr:
		Thread::Data::Current->Name;
}

#if !defined(INTRA_THREAD_NO_FULL_INTERRUPT) || defined(INTRA_DEBUG)
void TThisThread::onWait(SeparateCondVar* cv, Mutex* mutex)
{
	auto hndl = Thread::Data::Current;
	if(hndl == nullptr) return;
#ifndef INTRA_THREAD_NO_FULL_INTERRUPT
	if(hndl->ForceInterruptionDisabled.GetRelaxed()) return;
#endif
	if(cv) hndl->InterruptableAction();
	if(mutex == &hndl->StateMutex)
	{
		hndl->WaitedCondVar = cv;
		return;
	}
	INTRA_SYNCHRONIZED(hndl->StateMutex)
		hndl->WaitedCondVar = cv;
}

#endif

#ifndef INTRA_THREAD_NO_FULL_INTERRUPT
void TThisThread::allowInterruption(bool allow)
{
	if(Thread::Data::Current == nullptr) return;
	Thread::Data::Current->ForceInterruptionDisabled.Set(!allow);
}
#endif

const TThisThread ThisThread;
} INTRA_END

#endif

