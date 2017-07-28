#include "Concurrency.h"

#if !defined(INTRA_NO_CONCURRENCY)

#include "Concurrency/Thread.h"
#include "Concurrency/Atomic.h"
#include "Concurrency/Mutex.h"
#include "Concurrency/Lock.h"
#include "Concurrency/Synchronized.h"
#include "Concurrency/CondVar.h"

#if(INTRA_LIBRARY_THREAD != INTRA_LIBRARY_THREAD_None)

#include "Range/ForEach.h"

#include "System/Stopwatch.h"

using namespace Intra;

void TestInterruption(FormattedWriter& output)
{
	AtomicBool flag;
	Thread thr("Sleeper", [&]() {
		if(ThisThread.Sleep(500)) flag.Set(true);
	});
	Stopwatch clock;
	ThisThread.Sleep(1);
	auto elapsedUs = clock.ElapsedUs();
	output.PrintLine("ThisThread.Sleep(1) time: ", elapsedUs, u8" μs.");
	thr.Interrupt();

	clock.Reset();
	thr.Join();
	elapsedUs = clock.ElapsedUs();
	output.PrintLine(thr.Name(), " joining time: ", elapsedUs, u8" μs.");
#ifndef INTRA_THREAD_NO_FULL_INTERRUPT
	INTRA_ASSERT(!flag.Get());
#endif

	CondVar cv;
	thr = Thread("CVWaiter", [&]() {
		INTRA_SYNCHRONIZED(cv)
			cv.WaitMs(500, []() {return false;});
	});
	ThisThread.Sleep(1);
	thr.Interrupt();

	clock.Reset();
	thr.Join();
	elapsedUs = clock.ElapsedUs();
	output.PrintLine(thr.Name(), " joining time: ", elapsedUs, u8" μs.");
#ifndef INTRA_THREAD_NO_FULL_INTERRUPT
	INTRA_ASSERT1(elapsedUs < 50000 || Utils::IsDebuggerAttached(), elapsedUs);
#endif

	thr = Thread("NonInterruptibleSleeper", [&]() {
		ThisThread.DisableInterruption();
		ThisThread.Sleep(500);
	});
	ThisThread.Sleep(1);
	thr.Interrupt();

	clock.Reset();
	thr.Join();
	elapsedUs = clock.ElapsedUs();
	output.PrintLine(thr.Name(), " joining time: ", elapsedUs, u8" μs.");
	INTRA_ASSERT1(elapsedUs >= 480000 || Utils::IsDebuggerAttached(), elapsedUs);
}

#endif
#endif
