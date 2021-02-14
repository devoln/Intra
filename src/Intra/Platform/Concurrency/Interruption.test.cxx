#include "Intra/Range/ForEach.h"
#include "IntraX/System/Stopwatch.h"
#include "IntraX/System/Debug.h"
#include "IntraX/IO/Std.h"

#if !defined(INTRA_NO_CONCURRENCY)

#include "IntraX/Concurrency/Thread.h"
#include "Intra/Concurrency/Atomic.h"
#include "IntraX/Concurrency/Mutex.h"
#include "IntraX/Concurrency/Lock.h"
#include "IntraX/Concurrency/Synchronized.h"
#include "IntraX/Concurrency/CondVar.h"

#if(INTRA_LIBRARY_THREAD != INTRA_LIBRARY_THREAD_None)

namespace Intra { INTRA_BEGIN

INTRA_MODULE_UNITTEST
{
	AtomicBool flag;
	Thread thr("Sleeper", [&]() {
		if(ThisThread.Sleep(500)) flag.Set(true);
	});
	Stopwatch clock;
	ThisThread.Sleep(1);
	auto elapsedUs = clock.ElapsedUs();
	Std.PrintLine("ThisThread.Sleep(1) time: ", elapsedUs, " μs.");
	thr.Interrupt();

	clock.Reset();
	thr.Join();
	elapsedUs = clock.ElapsedUs();
#ifndef INTRA_THREAD_NO_FULL_INTERRUPT
	INTRA_ASSERT(!flag.Get());
#endif

	CondVar cv;
	thr = Thread("CVWaiter", [&]() {
		INTRA_SYNCHRONIZED(cv)
			cv.WaitMs(500, AlwaysFalse);
	});
	ThisThread.Sleep(1);
	thr.Interrupt();

	clock.Reset();
	thr.Join();
	elapsedUs = clock.ElapsedUs();
#ifndef INTRA_THREAD_NO_FULL_INTERRUPT
	INTRA_ASSERT1(elapsedUs < 50100 || IsDebuggerAttached(), elapsedUs);
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
	INTRA_ASSERT1(elapsedUs >= 480000 || IsDebuggerAttached(), elapsedUs);
}

#endif
#endif
