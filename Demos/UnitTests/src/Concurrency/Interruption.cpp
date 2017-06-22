#include "Concurrency.h"

#include "Concurrency/Thread.h"
#include "Concurrency/Atomic.h"
#include "Concurrency/Mutex.h"
#include "Concurrency/Lock.h"
#include "Concurrency/Synchronized.h"
#include "Concurrency/CondVar.h"
#include "Utils/Bind.h"
#include "Range/ForEach.h"
#include "System/Stopwatch.h"

using namespace Intra;

void TestInterruption(FormattedWriter& output)
{
	AtomicBool flag;
	Thread thr("Sleeper", [&]() {
		if(ThisThread.Sleep(500)) flag.Set(true);
	});
	ThisThread.Sleep(1);
	thr.Interrupt();

	Stopwatch clock;
	thr.Join();
	auto elapsedUs = clock.ElapsedUs();
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
	INTRA_ASSERT1(elapsedUs >= 485000 || Utils::IsDebuggerAttached(), elapsedUs);
}
