#include "DateTime.h"

#include "Cpp/Warnings.h"
#include "Cpp/PlatformDetect.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#ifdef _MSC_VER
#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <time.h>

#ifdef INTRA_LIBRARY_USE_STD_CLOCK

#include <chrono>
#include <ctime>

#elif(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4668)
#endif

#include <Windows.h>
#undef Yield

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif

namespace Intra { namespace System {

#ifdef INTRA_USE_STD_CLOCK
ulong64 DateTime::AbsTimeMs()
{
	auto now = std::chrono::system_clock::now();
	auto microsecs = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch());
	return ulong64(std::chrono::system_clock::to_time_t(now))*1000 + (microsecs.count() + 500) / 1000 % 1000;
}
#elif(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
ulong64 DateTime::AbsTimeMs()
{
	FILETIME time;
	GetSystemTimeAsFileTime(&time);
	ulong64 result = (ulong64(time.dwHighDateTime) << 32) | time.dwLowDateTime;
	result /= 10000;
	result -= 11644473600000ULL; //1 jan 1601 -> 1 jan 1970
	return result;
}

#elif(INTRA_PLATFORM_OS == INTRA_PLATFORM_MacOS)
ulong64 DateTime::AbsTimeMs()
{
	timeval time;
	gettimeofday(&time, null);
	return ulong64(time.tv_sec)*1000 + ulong64((time.tv_usec + 500) / 1000);
}
#else
ulong64 DateTime::AbsTimeMs()
{
	timespec time;
	clock_gettime(CLOCK_REALTIME, &time);
	return ulong64(time.tv_sec)*1000 + ulong64((time.tv_nsec + 500000) / 1000000);
}
#endif

DateTime DateTime::Now()
{
	const time_t t = time(0);
	tm* now = localtime(&t);
	return {ushort(now->tm_year + 1900), byte(now->tm_mon + 1), byte(now->tm_mday),
		byte(now->tm_hour), byte(now->tm_min), byte(now->tm_sec)};
}

ulong64 DateTime::TimeBasedSeed() const
{
	return Second + Minute*60ull + Hour*3600ull +
		Day*86400ull + Month*31ull*86400ull + Year*366ull*86400ull;
}

ulong64 DateTime::StartupTimeBasedSeed()
{
	static const ulong64 result = Now().TimeBasedSeed();
	return result;
}

}}

INTRA_WARNING_POP
