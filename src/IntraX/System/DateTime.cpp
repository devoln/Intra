#include "DateTime.h"

#ifdef _MSC_VER
#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS
#endif

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
#include <time.h>

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#undef Yield

#endif
INTRA_WARNING_POP

namespace Intra { INTRA_BEGIN
SystemTimestamp SystemTimestamp::Now()
{
#ifdef _WIN32
	FILETIME time;
	GetSystemTimeAsFileTime(&time);
	uint64 result = (uint64(time.dwHighDateTime) << 32) | time.dwLowDateTime;
	result -= 116444736000000000ULL; //1 jan 1601 -> 1 jan 1970
	result *= 100;
	return {int64(result)};
#elif defined(__APPLE__)
	timeval time;
	gettimeofday(&time, nullptr);
	return {time.tv_sec*1000000000LL + time.tv_usec*1000LL};
#else
	timespec time;
	clock_gettime(CLOCK_REALTIME, &time);
	return {time.tv_sec*1000000000LL + time.tv_nsec};
#endif
}

#ifdef _WIN32
INTRA_IGNORE_WARN_GLOBAL_CONSTRUCTION
static const double gNsPerQPCTick = []{
	LARGE_INTEGER li;
	QueryPerformanceFrequency(&li);
	return 1000000000.0 / li.QuadPart;
}();
#endif

MonotonicTimestamp MonotonicTimestamp::Now()
{
#ifdef _WIN32
	LARGE_INTEGER current;
	QueryPerformanceCounter(&current);
	return {int64(current.QuadPart * gNsPerQPCTick)};
#elif defined(__APPLE__)
	//TODO: use mach_continuous_time()
	return {SystemTimestamp::Now().RawValueNs};
#else
	timespec time;
#ifdef __linux__
	clock_gettime(CLOCK_BOOTTIME, &time);
#else
	clock_gettime(CLOCK_MONOTONIC, &time);
#endif
	return {time.tv_sec * 1000000000LL + time.tv_nsec};
#endif
}

TimeDelta DateTime::LocalOffset() const
{
#ifdef _WIN32
	TIME_ZONE_INFORMATION timeZone;
	GetTimeZoneInformation(&timeZone);
	return TimeDelta::Minutes(-timeZone.Bias);
#else
	const time_t epochPlus11h = 11*3600;
	const auto local = localtime(&epochPlus11h)->tm_hour;
	const auto localMins = local->tm_hour*60 + local->tm_min;
	const auto gmt = gmtime(&epochPlus11h)->tm_hour;
	const auto gmtMins = gmt->tm_hour*60 + gmt->tm_min;
	return TimeDelta::Minutes(localMins - gmtMins);
#endif
}

} INTRA_END
