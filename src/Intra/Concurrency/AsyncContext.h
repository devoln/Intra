#pragma once

#include <Intra/Concurrency/Coroutine.h>
#include <Intra/Container/Queue.h>

namespace Intra { INTRA_BEGIN

class AsyncContext
{
	ConcurrentWaitQueue<Coroutine> mQueue;
public:
	void Schedule(Coroutine task) {mQueue.Push(task);}
	void RunLoop()
	{
		Coroutine coroutineToResume;
		while(mQueue.Pop(coroutineToResume))
			coroutineToResume();
	}
};

} INTRA_END
