#include "Thread.h"
#include "Atomic.h"



#include "Core/Runtime.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#if(INTRA_LIBRARY_THREAD != INTRA_LIBRARY_THREAD_None)

#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
#include "detail/ThreadCommonWinAPI.hxx"
#else
#include <pthread.h>
#endif

#include "detail/BasicThreadData.hxx"

#if(INTRA_LIBRARY_THREAD == INTRA_LIBRARY_THREAD_Cpp11)

#include "detail/ThreadCpp11.hxx"

#elif(INTRA_LIBRARY_THREAD == INTRA_LIBRARY_THREAD_WinAPI)

#include "detail/ThreadWinAPI.hxx"

#elif(INTRA_LIBRARY_THREAD == INTRA_LIBRARY_THREAD_PThread)

#include "detail/ThreadSleep.hxx"
#include "detail/ThreadPThread.hxx"

#endif



INTRA_BEGIN
namespace Concurrency {

Thread::Thread(Func func):
	mHandle(new Data(Move(func)))
{
	if(mHandle->IsDetached)
		mHandle = null;
}

Thread::Thread(null_t) noexcept {}

Thread::Thread(Thread&&) noexcept = default;

Thread::~Thread()
{
	if(mHandle == null) return;
	Interrupt();
	if(!Join()) Detach();
}

Thread& Thread::operator=(Thread&&) = default;

bool Thread::Join()
{return mHandle == null || mHandle->Join();}

bool Thread::JoinMs(uint64 timeout)
{return mHandle == null || mHandle->Join(timeout);}

void Thread::Interrupt()
{
	if(mHandle == null) return;
	mHandle->Interrupt();
}

bool Thread::IsInterrupted() const
{
	if(mHandle == null) return false;
	return mHandle->IsInterrupted.GetRelaxed();
}

bool Thread::IsRunning() const
{return mHandle != null && mHandle->IsRunning.GetRelaxed();}

void Thread::SetName(StringView name)
{
	if(mHandle == null) return;
	mHandle->Name = name;
	mHandle->SetName();
}

StringView Thread::Name() const
{
	if(mHandle == null) return null;
	return mHandle->Name;
}

void Thread::Detach()
{
	if(mHandle == null) return;
	mHandle.Release()->Detach();
}

Thread::NativeHandle Thread::GetNativeHandle() const
{
	return mHandle == null? null:
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
#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
	return Thread::NativeHandle(GetCurrentThread());
#else
	thread_local pthread_t self = pthread_self();
	return Thread::NativeHandle(&self);
#endif
}

void TThisThread::Interrupt()
{
	if(Thread::Data::Current == null) return;
	Thread::Data::Current->Interrupt();
}

bool TThisThread::IsInterrupted()
{
	if(Thread::Data::Current == null) return false;
	return Thread::Data::Current->IsInterrupted.GetRelaxed();
}

bool TThisThread::IsInterruptionEnabled()
{
#ifndef INTRA_THREAD_NO_FULL_INTERRUPT
	if(Thread::Data::Current == null) return false;
	return !Thread::Data::Current->ForceInterruptionDisabled.GetRelaxed();
#else
	return false;
#endif
}

StringView TThisThread::Name()
{
	return Thread::Data::Current == null? null:
		Thread::Data::Current->Name;
}

#if !defined(INTRA_THREAD_NO_FULL_INTERRUPT) || defined(INTRA_DEBUG)
void TThisThread::onWait(SeparateCondVar* cv, Mutex* mutex)
{
	auto hndl = Thread::Data::Current;
	if(hndl == null) return;
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
	if(Thread::Data::Current == null) return;
	Thread::Data::Current->ForceInterruptionDisabled.Set(!allow);
}
#endif

const TThisThread ThisThread;

}}

#endif

INTRA_WARNING_POP
