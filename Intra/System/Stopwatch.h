#pragma once


#include "Core/Core.h"


#define INTRA_LIBRARY_STOPWATCH_QPC 1
#define INTRA_LIBRARY_STOPWATCH_clock_gettime 2
#define INTRA_LIBRARY_STOPWATCH_clock 3
#define INTRA_LIBRARY_STOPWATCH_gettimeofday 4
#define INTRA_LIBRARY_STOPWATCH_Cpp11 5


#ifndef INTRA_LIBRARY_STOPWATCH

#ifdef INTRA_LIBRARY_USE_STD_CLOCK
#define INTRA_LIBRARY_STOPWATCH INTRA_LIBRARY_STOPWATCH_Cpp11
#elif(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
#define INTRA_LIBRARY_STOPWATCH INTRA_LIBRARY_STOPWATCH_QPC
#elif(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_MacOS || INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_iOS)
#define INTRA_LIBRARY_STOPWATCH INTRA_LIBRARY_STOPWATCH_gettimeofday
#else
#define INTRA_LIBRARY_STOPWATCH INTRA_LIBRARY_STOPWATCH_clock_gettime
#endif

#endif

INTRA_BEGIN
namespace System {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

class Stopwatch
{
public:
	Stopwatch();
	~Stopwatch();

	void Reset();
	double ElapsedSeconds() const;
	double GetElapsedSecondsAndReset();

	uint ElapsedUs() {return uint(ElapsedSeconds()*1000000);}
	uint GetElapsedUsAndReset() {return uint(GetElapsedSecondsAndReset()*1000000);}

	uint ElapsedMs() {return uint(ElapsedSeconds()*1000);}
	uint GetElapsedMsAndReset() {return uint(GetElapsedSecondsAndReset()*1000);}

private:
	uint64 mData;
};

INTRA_WARNING_POP

}
using System::Stopwatch;

}
