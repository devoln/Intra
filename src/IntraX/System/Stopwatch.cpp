#include "Stopwatch.h"

#ifdef _WIN32

INTRA_PUSH_DISABLE_ALL_WARNINGS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
INTRA_WARNING_POP

INTRA_BEGIN
static uint64 queryTimerPerformanceFrequency()
{
	LARGE_INTEGER li;
	QueryPerformanceFrequency(&li);
	return uint64(li.QuadPart);
}
INTRA_IGNORE_WARN_GLOBAL_CONSTRUCTION
static const double gStopwatchPerformanceFrequency = double(queryTimerPerformanceFrequency());

Stopwatch::Stopwatch() {Reset();}

void Stopwatch::Reset()
{
	LARGE_INTEGER current;
	QueryPerformanceCounter(&current);
	mData = uint64(current.QuadPart);
}


double Stopwatch::ElapsedSeconds() const
{
	LARGE_INTEGER current;
	QueryPerformanceCounter(&current);
	return double(uint64(current.QuadPart) - mData) / gStopwatchPerformanceFrequency;
}

double Stopwatch::GetElapsedSecondsAndReset()
{
	LARGE_INTEGER current;
	QueryPerformanceCounter(&current);
	const double result = double(uint64(current.QuadPart) - mData) / gStopwatchPerformanceFrequency;
	mData = uint64(current.QuadPart);
	return result;
}
INTRA_END
#elif defined(__APPLE__)
#include <sys/time.h>
INTRA_BEGIN
static uint64 gettime_usecs()
{
	struct timeval tv;
	gettimeofday(&tv, null);
	return uint64(tv.tv_sec)*1000000 + uint64(tv.tv_usec);
}

Stopwatch::Stopwatch(): mData(gettime_usecs()) {}

void Stopwatch::Reset() {mData = gettime_usecs();}

double Stopwatch::ElapsedSeconds() const {return double(gettime_usecs() - mData) / 1000000;}

double Stopwatch::GetElapsedSecondsAndReset()
{
	const auto newData = gettime_usecs();
	const double result = double(newData - mData) / 1000000;
	mData = newData;
	return result;
}
INTRA_END
#else
#include <time.h>
INTRA_BEGIN
static uint64 clock_gettime_nsecs(clockid_t clkId)
{
	struct timespec ts;
	clock_gettime(clkId, &ts);
	return uint64(ts.tv_sec)*1000000000 + uint64(ts.tv_nsec);
}

Stopwatch::Stopwatch(): mData(clock_gettime_nsecs(CLOCK_REALTIME)) {}

void Stopwatch::Reset() {mData = clock_gettime_nsecs(CLOCK_REALTIME);}

double Stopwatch::ElapsedSeconds() const {return double(clock_gettime_nsecs(CLOCK_REALTIME) - mData) / 1000000000;}

double Stopwatch::GetElapsedSecondsAndReset()
{
	const auto newData = clock_gettime_nsecs(CLOCK_REALTIME);
	const double result = double(newData - mData) / 1000000000;
	mData = newData;
	return result;
}

#if 0

CpuStopwatch::CpuStopwatch(): mData(clock_gettime_nsecs(CLOCK_PROCESS_CPUTIME_ID)) {}
CpuStopwatch::~CpuStopwatch() = default;

void CpuStopwatch::Reset() {mData = clock_gettime_nsecs(CLOCK_PROCESS_CPUTIME_ID);}

double CpuStopwatch::ElapsedSeconds() const {return double(clock_gettime_nsecs(CLOCK_PROCESS_CPUTIME_ID) - mData) / 1000000000;}

double CpuStopwatch::GetElapsedSecondsAndReset()
{
	const auto newData = clock_gettime_nsecs(CLOCK_PROCESS_CPUTIME_ID);
	const double result = double(newData - mData) / 1000000000;
	mData = newData;
	return result;
}

#endif

INTRA_END
#endif
