#include "Async/Job.h"

#include "Platform/PlatformInfo.h"
#include "Math/Random.h"
#include "Container/Sequential/Array.h"
#include "Platform/Thread.h"
#include "Platform/Atomic.h"

#undef Yield

namespace Intra {

using namespace Math;

class WorkStealingQueue;
namespace
{
	Array<WorkStealingQueue*> queues;
}

#if(INTRA_LIBRARY_THREADING!=INTRA_LIBRARY_THREADING_CPPLIB)
class WorkStealingQueue
{
public:
	WorkStealingQueue(): jobs(), bottom(0), top(0), mMutex()
	{
		queues.AddLast(this);
	}

	WorkStealingQueue(const WorkStealingQueue&) = delete;
	WorkStealingQueue& operator=(const WorkStealingQueue&) = delete;

	enum: uint {NUMBER_OF_JOBS = 4096};

	void Push(Job* job)
	{
		auto locker = mMutex.Locker();
		jobs[bottom % NUMBER_OF_JOBS] = job;
		++bottom;
	}

	Job* Pop()
	{
		auto locker = mMutex.Locker();
		const int jobCount = int(bottom)-int(top);
		if(jobCount <= 0)
		{
			// no job left in the queue
			return null;
		}

		--bottom;
		return jobs[bottom % NUMBER_OF_JOBS];
	}

	Job* Steal()
	{
		auto locker = mMutex.Locker();
		const int jobCount = int(bottom)-int(top);
		if(jobCount <= 0)
		{
			// no job there to steal
			return null;
		}

		Job* job = jobs[top % NUMBER_OF_JOBS];
		++top;
		return job;
	}

private:
	Array<Job*> jobs;
	uint bottom, top;
	Mutex mMutex;
};
#else
class WorkStealingQueue
{
public:
	WorkStealingQueue()
	{
		queues.AddLast(this);
	}

	static const uint NUMBER_OF_JOBS = 4096;

	void Push(Job* job)
	{
		long b = bottom;
		jobs[b % NUMBER_OF_JOBS] = job;

		// ensure the job is written before b+1 is published to other threads.
		// on x86/64, a compiler barrier is enough.
		//On other platforms (PowerPC, ARM, …) you would need a memory fence instead.
		//Furthermore, notice that the store operation also doesn’t need to be carried out atomically in this case, because the only other operation writing to bottom is Pop(), which cannot be carried out concurrently.
		INTRA_COMPILER_BARRIER;

		bottom = b+1;
	}

	Job* Pop()
	{
		long b = bottom - 1;
		_InterlockedExchange(&bottom, b);

		long t = top;
		if(t <= b)
		{
			// non-empty queue
			Job* job = jobs[b % NUMBER_OF_JOBS];
			if(t != b)
			{
				// there's still more than one item left in the queue
				return job;
			}

			// this is the last item in the queue
			if(std::atomic_compare_exchange_strong(&top, &t, t+1))
			{
				// failed race against steal operation
				job = null;
			}

			bottom = t+1;
			return job;
		}
		else
		{
			// deque was already empty
			bottom = t;
			return null;
		}
	}

	Job* Steal()
	{
		long t = top;

		// ensure that top is always read before bottom.
		// loads will not be reordered with other loads on x86, so a compiler barrier is enough.
		INTRA_COMPILER_BARRIER;

		long b = bottom;
		if(t < b)
		{
			// non-empty queue
			Job* job = jobs[t % NUMBER_OF_JOBS];

			// the interlocked function serves as a compiler barrier, and guarantees that the read happens before the CAS.
			if(std::atomic_compare_exchange_strong(&top, &t, t+1))
			{
				// a concurrent steal or pop operation removed an element from the deque in the meantime.
				return null;
			}

			return job;
		}
		else
		{
			// empty queue
			return null;
		}
	}

private:
	Array<Job*> jobs;
	std::atomic<long> top;
	long bottom;
};
#endif




namespace
{
	/*thread_local*/ WorkStealingQueue wsqueue;
	Atomic<int> JobToDeleteCount;
	Array<Job*> JobsToDelete;
}

void WorkerMain()
{
	bool workerThreadActive = true;
	while(workerThreadActive)
	{
		Job* job = Job::Get();
		if(job!=null) job->Execute();
	}
}


enum {MaxJobCount=4096};
static /*thread_local*/ Array<Job> g_jobAllocator;
static thread_local uint g_allocatedJobs = 0u;

Job* Job::Allocate()
{
	const uint index = g_allocatedJobs++;
	return &g_jobAllocator[(index-1) % MaxJobCount];
}

bool Job::IsEmpty()
{
	return unfinishedJobs==-1;
}

Job* Job::Get()
{
	Job* job = wsqueue.Pop();
	if(job->IsEmpty())
	{
		// this is not a valid job because our own queue is empty, so try stealing from some other queue
		uint randomIndex = Random<uint>::Global(uint(queues.Count()));
		WorkStealingQueue* stealQueue = queues[randomIndex];
		if(stealQueue == &wsqueue)
		{
			// don't try to steal from ourselves
			Thread::Yield();
			return null;
		}

		Job* stolenJob = stealQueue->Steal();
		if(stolenJob->IsEmpty())
		{
			// we couldn't steal a job from the other queue either, so we just yield our time slice for now
			Thread::Yield();
			return null;
		}

		return stolenJob;
	}

	return job;
}

void Job::Finish()
{
	const int unfinished_jobs = --unfinishedJobs;
	if(unfinished_jobs==0 && parent) parent->Finish();
}

void Job::Execute()
{
	function(this, data);
	Finish();
}

Job* Job::CreateJob(Job::Function function)
{
	g_jobAllocator.SetCountUninitialized(MaxJobCount);

	Job* result = Job::Allocate();
	result->function = function;
	result->parent = null;
	result->unfinishedJobs = 1;

	return result;
}

Job* Job::CreateJobAsChild(Job* parent, Job::Function function)
{
	++parent->unfinishedJobs;

	Job* result = Job::Allocate();
	result->function = function;
	result->parent = parent;
	result->unfinishedJobs = 1;

	return result;
}



void Job::Run()
{
	wsqueue.Push(this);
}

void Job::Wait() const
{
	while(unfinishedJobs!=0)
	{
		Job* nextJob = Job::Get();
		if(nextJob!=null) nextJob->Execute();
	}
}

}

