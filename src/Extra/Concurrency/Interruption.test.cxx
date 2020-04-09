#include "Intra/Range/ForEach.h"
#include "Extra/System/Stopwatch.h"
#include "Extra/System/Debug.h"
#include "Extra/IO/Std.h"

#if !defined(INTRA_NO_CONCURRENCY)

#include "Extra/Concurrency/Thread.h"
#include "Intra/Concurrency/Atomic.h"
#include "Extra/Concurrency/Mutex.h"
#include "Extra/Concurrency/Lock.h"
#include "Extra/Concurrency/Synchronized.h"
#include "Extra/Concurrency/CondVar.h"

#if(INTRA_LIBRARY_THREAD != INTRA_LIBRARY_THREAD_None)

INTRA_BEGIN

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
