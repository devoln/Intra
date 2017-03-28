#pragma once

#include "Platform/CppWarnings.h"
#include "Platform/FundamentalTypes.h"
#include "Container/ForwardDecls.h"

namespace Intra {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

class Timer
{
public:
	Timer();
	~Timer();

	void Reset();
	double GetTime() const;
	double GetTimeAndReset();

	uint GetTimeUs() {return uint(GetTime()*1000000);}
	uint GetTimeUsAndReset() {return uint(GetTimeAndReset()*1000000);}

	uint GetTimeMs() {return uint(GetTime()*1000);}
	uint GetTimeMsAndReset() {return uint(GetTimeAndReset()*1000);}

	static void SleepMs(uint msec);

private:
	ulong64 hndl;
};

struct DateTime
{
	static DateTime Now();

	String ToString() const;
	ulong64 TimeBasedRandomValue() const;

	ushort Year;
	byte Month, Day, Hour, Minute, Second;

	//! Константа, монотонно зависящая от времени запуска программы.
	//! Его можно использовать как seed в генераторах псевдослучайных чисел.
	static ulong64 StartupTimeBasedSeed();
};

INTRA_WARNING_POP

}
