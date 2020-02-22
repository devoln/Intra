#include "Concurrency.h"

#if !defined(INTRA_NO_CONCURRENCY) && INTRA_LIBRARY_ATOMIC != INTRA_LIBRARY_ATOMIC_None

#include "Concurrency/Thread.h"
#include "Concurrency/Atomic.h"
#include "Concurrency/Mutex.h"
#include "Concurrency/Lock.h"
#include "Concurrency/Synchronized.h"
#include "Core/Functional.h"
#include "Core/Range/ForEach.h"

using namespace Intra;

static void TestAtomicIncrementThread(AtomicInt* counter, int count)
{
	while(count --> 0)
		counter->IncrementRelaxed();
}

static void TestAtomicSubThread(AtomicInt* counter, int count)
{
	while(count --> 0)
		counter->SubRelaxed(3);
}

static void TestSynchronizedAddThread(Synchronized<int>* counter, volatile int count)
{
	while(count --> 0)
		*counter += 2;
}

void TestAtomics(FormattedWriter& output)
{
	Array<Thread> atomicThreads;
	AtomicInt counter{0};

	for(size_t i = 0; i < 10; i++)
		atomicThreads << Thread("AtomicIncrementer " + StringOf(i),
			Bind(Bind(TestAtomicIncrementThread, &counter), 100000));
	ForEach(atomicThreads, &Thread::Join);

	output.PrintLine("Atomic counter value GetRelaxed: ", counter.GetRelaxed());
	output.PrintLine("Atomic counter value Get: ", counter.Get());
	INTRA_ASSERT_EQUALS(counter.Get(), 1000000);

	for(size_t i = 0; i < atomicThreads.Length(); i++)
		atomicThreads[i] = Thread("AtomicSubtracter " + StringOf(i),
			Bind(Bind(TestAtomicSubThread, &counter), 10000));
	ForEach(atomicThreads, &Thread::Join);

	output.PrintLine("Atomic counter value GetRelaxed: ", counter.GetRelaxed());
	output.PrintLine("Atomic counter value Get: ", counter.Get());
	INTRA_ASSERT_EQUALS(counter.Get(), 700000);

	Synchronized<int> intCounter = counter.GetRelaxed();
	for(size_t i = 0; i < atomicThreads.Length(); i++)
		atomicThreads[i] = Thread("TestSynchronizedAdder " + StringOf(i),
			Bind(Bind(TestSynchronizedAddThread, &intCounter), 100000));
	ForEach(atomicThreads, &Thread::Join);

	output.PrintLine("Synchronized counter value: ", intCounter);
	INTRA_ASSERT_EQUALS(intCounter, 2700000);
}

#endif
