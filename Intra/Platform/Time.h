#pragma once

#include "Cpp/Warnings.h"
#include "Cpp/Fundamental.h"
#include "Container/ForwardDecls.h"

#include "Cpp/PlatformDetect.h"

//! Используемый метод для отсчёта времени
#define INTRA_LIBRARY_TIMER_Dummy 0
#define INTRA_LIBRARY_TIMER_QPC 1
#define INTRA_LIBRARY_TIMER_CPPLIB 2
#define INTRA_LIBRARY_TIMER_Qt 3
#define INTRA_LIBRARY_TIMER_CLIB 4


#ifndef INTRA_LIBRARY_TIMER

#if INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows
#define INTRA_LIBRARY_TIMER INTRA_LIBRARY_TIMER_QPC
#else
#define INTRA_LIBRARY_TIMER INTRA_LIBRARY_TIMER_CPPLIB
#endif

#endif

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

	ulong64 TimeBasedSeed() const;

	ushort Year;
	byte Month, Day, Hour, Minute, Second;

	//! Константа, монотонно зависящая от времени запуска программы.
	//! Его можно использовать как seed в генераторах псевдослучайных чисел.
	static ulong64 StartupTimeBasedSeed();
};

template<class R> void ToString(R&& dst, const DateTime& dt)
{
	dst.Put('0' + dt.Day / 10);
	dst.Put('0' + dt.Day % 10);
	dst.Put('.');
	dst.Put('0' + dt.Month / 10);
	dst.Put('0' + dt.Month % 10);
	dst.Put('.');
	if(dt.Year>10000) dst.Put('0' + dt.Year / 10000);
	if(dt.Year>1000) dst.Put('0' + dt.Year / 1000 % 10);
	if(dt.Year>100) dst.Put('0' + dt.Year / 100 % 10);
	if(dt.Year>10) dst.Put('0' + dt.Year / 10 % 10);
	dst.Put(' ');
	dst.Put('0' + dt.Hour / 10);
	dst.Put('0' + dt.Hour % 10);
	dst.Put(':');
	dst.Put('0' + dt.Minute / 10);
	dst.Put('0' + dt.Minute % 10);
	dst.Put(':');
	dst.Put('0' + dt.Second / 10);
	dst.Put('0' + dt.Second % 10);
}

INTRA_WARNING_POP

}
