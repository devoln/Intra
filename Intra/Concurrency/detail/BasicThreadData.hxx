#pragma once

#include "Concurrency/Mutex.h"
#include "Concurrency/Lock.h"
#include "Concurrency/Atomic.h"
#include "Container/Sequential/String.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_MOVE_IMPLICITLY_DELETED

namespace Intra { namespace Concurrency { namespace detail {

struct BasicThreadData
{
	Thread::Func Function;
	bool IsDetached = false;
	Atomic<bool> IsRunning{true};
	Atomic<bool> IsInterrupted{false};
	String Name;

	void InterruptableAction() {}

	void Interrupt() {IsInterrupted = true;}
	void SetName() {}

	void ThreadFunc()
	{
		Current = Thread::Handle(this);

		Function();
		Function = null;

		INTRA_SYNCHRONIZED_BLOCK(GlobalThreadMutex)
		{
			IsRunning = false;
			if(IsDetached)
				delete this;
		}
	}

	static thread_local Thread::Handle Current;
	static Mutex GlobalThreadMutex;
};
thread_local Thread::Handle BasicThreadData::Current = null;
Mutex BasicThreadData::GlobalThreadMutex;

}}}

INTRA_WARNING_POP
