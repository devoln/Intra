#include "Intra/Functional.h"
#include "Intra/Range/ForEach.h"
#include "Intra/Concurrency/Atomic.h"
#include "IntraX/System/Debug.h"
#include "IntraX/Container/Sequential/Array.h"
#include "IntraX/Container/Sequential/String.h"

#if !defined(INTRA_NO_CONCURRENCY) && INTRA_LIBRARY_ATOMIC != INTRA_LIBRARY_ATOMIC_None

#include "IntraX/Concurrency/Thread.h"
#include "IntraX/Concurrency/Mutex.h"
#include "IntraX/Concurrency/Lock.h"
#include "IntraX/Concurrency/Synchronized.h"

INTRA_BEGIN

static void TestAtomicIncrementThread(AtomicInt* counter, int count)
{
	while(count--)
		counter->IncrementRelaxed();
}

static void TestAtomicSubThread(AtomicInt* counter, int count)
{
	while(count--)
		counter->SubRelaxed(3);
}

static void TestSynchronizedAddThread(Synchronized<int>* counter, volatile int count)
{
	while(count--)
		*counter += 2;
}

INTRA_MODULE_UNITTEST
{
	Array<Thread> atomicThreads;
	AtomicInt counter{0};

	for(size_t i = 0; i < 10; i++)
		atomicThreads << Thread("AtomicIncrementer " + StringOf(i),
			Bind(Bind(TestAtomicIncrementThread, &counter), 100000));
	ForEach(atomicThreads, &Thread::Join);
	INTRA_ASSERT_EQUALS(counter.Get(), 1000000);

	for(index_t i = 0; i < atomicThreads.Length(); i++)
		atomicThreads[i] = Thread("AtomicSubtracter " + StringOf(i),
			Bind(Bind(TestAtomicSubThread, &counter), 10000));
	ForEach(atomicThreads, &Thread::Join);
	INTRA_ASSERT_EQUALS(counter.Get(), 700000);

	Synchronized<int> intCounter = counter.GetRelaxed();
	for(index_t i = 0; i < atomicThreads.Length(); i++)
		atomicThreads[i] = Thread("TestSynchronizedAdder " + StringOf(i),
			Bind(Bind(TestSynchronizedAddThread, &intCounter), 100000));
	ForEach(atomicThreads, &Thread::Join);
	INTRA_ASSERT_EQUALS(intCounter, 2700000);
}
INTRA_END
#endif
