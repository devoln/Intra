#pragma once

#include "Platform/CppWarnings.h"
#include "Platform/FundamentalTypes.h"
#include "Containers/ForwardDeclarations.h"

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

	static void Wait(uint msec);

private:
	ulong64 hndl;
};

struct DateTime
{
	static DateTime Now();

	String ToString() const;

	ushort Year;
	byte Month, Day, Hour, Minute, Second;
};

INTRA_WARNING_POP

}
