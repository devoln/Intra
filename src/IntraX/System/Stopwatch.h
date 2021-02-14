#pragma once

#include "IntraX/Core.h"

#define INTRA_LIBRARY_STOPWATCH_QPC 1
#define INTRA_LIBRARY_STOPWATCH_clock_gettime 2
#define INTRA_LIBRARY_STOPWATCH_clock 3
#define INTRA_LIBRARY_STOPWATCH_gettimeofday 4
#define INTRA_LIBRARY_STOPWATCH_Cpp11 5

namespace Intra { INTRA_BEGIN
class Stopwatch
{
public:
	Stopwatch();

	void Reset();
	double ElapsedSeconds() const;
	double GetElapsedSecondsAndReset();

	unsigned ElapsedUs() {return unsigned(ElapsedSeconds()*1000000);}
	unsigned GetElapsedUsAndReset() {return unsigned(GetElapsedSecondsAndReset()*1000000);}

	unsigned ElapsedMs() {return unsigned(ElapsedSeconds()*1000);}
	unsigned GetElapsedMsAndReset() {return unsigned(GetElapsedSecondsAndReset()*1000);}

private:
	uint64 mData;
};
} INTRA_END
