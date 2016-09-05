#pragma once

#include "Core/Core.h"
#include "Containers/ForwardDeclarations.h"

namespace Intra {

class Timer
{
public:
	Timer();
	~Timer();

	void Reset();
	double GetTime();
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

	ushort Year;
	byte Month, Day, Hour, Minute, Second;
};

String ToString(const DateTime& datetime);

}
