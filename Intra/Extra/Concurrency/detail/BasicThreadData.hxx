#pragma once

#include "Concurrency/Mutex.h"
#include "Concurrency/CondVar.h"
#include "Concurrency/Lock.h"
#include "Concurrency/Atomic.h"
#include "Container/Sequential/String.h"

INTRA_BEGIN
INTRA_WARNING_DISABLE_COPY_MOVE_IMPLICITLY_DELETED
namespace detail {

struct BasicThreadData
{
	Thread::Func Function;
	bool IsDetached = false;
	AtomicBool IsRunning{true};
	AtomicBool IsInterrupted{false};
	String Name;
	Mutex StateMutex;

#ifndef INTRA_THREAD_NO_FULL_INTERRUPT
	AtomicBool ForceInterruptionDisabled{false};
#endif
#if !defined(INTRA_THREAD_NO_FULL_INTERRUPT) || defined(INTRA_DEBUG)
	SeparateCondVar* WaitedCondVar = null;
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
		Function = null;

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
thread_local Thread::Handle BasicThreadData::Current = null;

}
INTRA_END
