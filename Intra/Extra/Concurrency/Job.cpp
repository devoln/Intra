

#include "Container/Sequential/Array.h"

#include "Thread.h"
#include "Mutex.h"
#include "Atomic.h"
#include "Lock.h"

#if(INTRA_LIBRARY_THREAD != INTRA_LIBRARY_THREAD_None)

#include "Concurrency/Job.h"

#undef Yield

INTRA_BEGIN
INTRA_WARNING_DISABLE_GLOBAL_CONSTRUCTION

class WorkStealingQueue;
static Array<WorkStealingQueue*> queues;

#if(INTRA_LIBRARY_ATOMIC == INTRA_LIBRARY_ATOMIC_None)
class WorkStealingQueue
{
public:
	WorkStealingQueue(): mJobs(), mBottom(0), mTop(0), mMutex()
	{
		queues.AddLast(this);
	}

	WorkStealingQueue(const WorkStealingQueue&) = delete;
	WorkStealingQueue& operator=(const WorkStealingQueue&) = delete;

	enum: uint {NUMBER_OF_JOBS = 4096};

	void Push(Job* job)
	{
		INTRA_SYNCHRONIZED(mMutex)
		{
			jobs[mBottom++ % NUMBER_OF_JOBS] = job;
		}
	}

	Job* Pop()
	{
		auto locker = MakeLock(mMutex);
		const int jobCount = int(mBottom) - int(mTop);
		if(jobCount <= 0)
		{
			// no job left in the queue
			return null;
		}
		return jobs[--mBottom % NUMBER_OF_JOBS];
	}

	Job* Steal()
	{
		auto locker = MakeLock(mMutex);
		const int jobCount = int(mBottom) - int(mTop);
		if(jobCount <= 0) return null;
		return mJobs[mTop++ % NUMBER_OF_JOBS];
	}

private:
	Array<Job*> mJobs;
	uint mBottom, mTop;
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

	WorkStealingQueue(const WorkStealingQueue&) = delete;
	WorkStealingQueue& operator=(const WorkStealingQueue&) = delete;

	enum {NUMBER_OF_JOBS = 4096};

	void Push(Job* job)
	{
		int b = mBottom.GetRelaxed();
		mJobs[size_t(b % NUMBER_OF_JOBS)] = job;

		mBottom.Set(b + 1);
	}

	Job* Pop()
	{
		const int b = mBottom.GetRelaxed() - 1;
		mBottom.Set(b);

		int t = mTop.Get();
		if(t <= b)
		{
			// non-empty queue
			Job* job = mJobs[size_t(b % NUMBER_OF_JOBS)];
			if(t != b)
			{
				// there's still more than one item left in the queue
				return job;
			}

			// this is the last item in the queue
			if(mTop.CompareSet(t, t + 1))
			{
				// failed race against steal operation
				job = null;
			}

			mBottom.SetRelaxed(t + 1);
			return job;
		}
		else
		{
			// deque was already empty
			mBottom.SetRelaxed(t);
			return null;
		}
	}

	Job* Steal()
	{
		int t = mTop.Get();

		int b = mBottom.GetRelaxed();
		if(t < b)
		{
			// non-empty queue
			Job* job = mJobs[size_t(t % NUMBER_OF_JOBS)];

			// serves as a compiler barrier, and guarantees that the read happens before the CAS.
			if(mTop.CompareSet(t, t + 1))
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
	Array<Job*> mJobs;
	AtomicInt mTop, mBottom;
};
#endif



INTRA_WARNING_DISABLE_GLOBAL_CONSTRUCTION
namespace
{
	/*thread_local*/ WorkStealingQueue wsqueue;
	AtomicInt JobToDeleteCount;
	Array<Job*> JobsToDelete;
}

void WorkerMain()
{
	bool workerThreadActive = true;
	while(workerThreadActive)
	{
		Job* job = Job::Get();
		if(job != null) job->Execute();
	}
}


enum {MaxJobCount = 4096};
static thread_local Array<Job> g_jobAllocator;
static thread_local uint g_allocatedJobs = 0u;

Job* Job::Allocate()
{
	const uint index = g_allocatedJobs++;
	return &g_jobAllocator[(index - 1) % MaxJobCount];
}

bool Job::IsEmpty()
{
	return mUnfinishedJobCount.GetRelaxed() == -1;
}

Job* Job::Get()
{
	Job* const job = wsqueue.Pop();
	if(job->IsEmpty())
	{
		// this is not a valid job because our own queue is empty, so try stealing from some other queue
		const size_t randomIndex = (reinterpret_cast<size_t>(job) >> 6) % queues.Count();
		WorkStealingQueue* const stealQueue = queues[randomIndex];
		if(stealQueue == &wsqueue)
		{
			// don't try to steal from ourselves
			ThisThread.Yield();
			return null;
		}

		Job* stolenJob = stealQueue->Steal();
		if(stolenJob->IsEmpty())
		{
			// we couldn't steal a job from the other queue either, so we just yield our time slice for now
			ThisThread.Yield();
			return null;
		}

		return stolenJob;
	}

	return job;
}

void Job::Finish()
{
	const int unfinishedJobs = mUnfinishedJobCount.Increment();
	if(unfinishedJobs == 0 && mParent != null) mParent->Finish();
}

void Job::Execute()
{
	mFunction(this, data);
	Finish();
}

Job* Job::CreateJob(Job::Function function)
{
	g_jobAllocator.SetCountUninitialized(MaxJobCount);

	Job* result = Job::Allocate();
	result->mFunction = function;
	result->mParent = null;
	result->mUnfinishedJobCount.Set(1);

	return result;
}

Job* Job::CreateJobAsChild(Job* parent, Job::Function function)
{
	parent->mUnfinishedJobCount.Increment();

	Job* result = Job::Allocate();
	result->mFunction = function;
	result->mParent = parent;
	result->mUnfinishedJobCount.Set(1);

	return result;
}



void Job::Run()
{
	wsqueue.Push(this);
}

void Job::Wait() const
{
	while(mUnfinishedJobCount.Get() != 0)
	{
		Job* nextJob = Job::Get();
		if(nextJob != null) nextJob->Execute();
	}
}
INTRA_END

#endif

