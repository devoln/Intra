#include "DateTime.h"

#ifdef _MSC_VER
#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS
#endif

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
#include <time.h>

#ifdef INTRA_USE_STD_CLOCK

#include <chrono>
#include <ctime>

#elif defined(_WIN32)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#undef Yield

#endif
INTRA_WARNING_POP

INTRA_BEGIN
SystemTimestamp SystemTimestamp::Now()
{
#ifdef INTRA_USE_STD_CLOCK
	using namespace std::chrono;
	auto now = system_clock::now();
	auto ns = duration_cast<nanoseconds>(now.time_since_epoch());
	return {system_clock::to_time_t(now)*1000000000LL + ns.count()%1000000000LL};
#elif defined(_WIN32)
	FILETIME time;
	GetSystemTimeAsFileTime(&time);
	uint64 result = (uint64(time.dwHighDateTime) << 32) | time.dwLowDateTime;
	result -= 116444736000000000ULL; //1 jan 1601 -> 1 jan 1970
	result *= 100;
	return {int64(result)};
#elif defined(__APPLE__)
	timeval time;
	gettimeofday(&time, null);
	return {time.tv_sec*1000000000LL + time.tv_usec*1000LL};
#else
	timespec time;
	clock_gettime(CLOCK_REALTIME, &time);
	return {time.tv_sec*1000000000LL + time.tv_nsec};
#endif
}

#if !defined(INTRA_USE_STD_CLOCK) && defined(_WIN32)
INTRA_IGNORE_WARN_GLOBAL_CONSTRUCTION
static const double gNsPerQPCTick = []{
	LARGE_INTEGER li;
	QueryPerformanceFrequency(&li);
	return 1000000000.0 / li.QuadPart;
}();
#endif

MonotonicTimestamp MonotonicTimestamp::Now()
{
#ifdef INTRA_USE_STD_CLOCK
	using namespace std::chrono;
	auto now = steady_clock::now();
	auto ns = duration_cast<nanoseconds>(now.time_since_epoch());
	return {ns.count()};
#elif defined(_WIN32)
	LARGE_INTEGER current;
	QueryPerformanceCounter(&current);
	return {int64(current.QuadPart * gNsPerQPCTick)};
#elif defined(__APPLE__)
	//TODO: use mach_continuous_time()
	return {SystemTimestamp::Now().RawValueNs};
#else
	timespec time;
#if defined(__linux__) && defined(CLOCK_BOOTTIME)
	clock_gettime(CLOCK_BOOTTIME, &time);
#else
	clock_gettime(CLOCK_MONOTONIC, &time);
#endif
	return {time.tv_sec * 1000000000LL + time.tv_nsec};
#endif
}

DateTime DateTime::Now()
{
	const time_t t = time(null);
	tm* now = localtime(&t);
	return {uint16(now->tm_year + 1900), byte(now->tm_mon + 1), byte(now->tm_mday),
		byte(now->tm_hour), byte(now->tm_min), byte(now->tm_sec)};
}

bool DateTime::IsLeapYear() const
{
	return Year % 4 == 0 && Year / 100 % 4 == 0;
}

INTRA_END
