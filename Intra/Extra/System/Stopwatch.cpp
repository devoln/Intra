#include "Stopwatch.h"


#if(INTRA_LIBRARY_STOPWATCH == INTRA_LIBRARY_STOPWATCH_QPC)

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
INTRA_WARNING_DISABLE_GLOBAL_CONSTRUCTION
static const double gStopwatchPerformanceFrequency = double(queryTimerPerformanceFrequency());

Stopwatch::Stopwatch() {Reset();}

Stopwatch::~Stopwatch() = default;

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

#elif(INTRA_LIBRARY_STOPWATCH == INTRA_LIBRARY_STOPWATCH_clock_gettime)

#include <time.h>

INTRA_BEGIN
static uint64 clock_gettime_nsecs(clockid_t clkId)
{
	struct timespec ts;
	clock_gettime(clkId, &ts);
	return uint64(ts.tv_sec)*1000000000 + uint64(ts.tv_nsec);
}

Stopwatch::Stopwatch(): mData(clock_gettime_nsecs(CLOCK_REALTIME)) {}
Stopwatch::~Stopwatch() = default;

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

#elif(INTRA_LIBRARY_STOPWATCH == INTRA_LIBRARY_STOPWATCH_gettimeofday)

#include <sys/time.h>

INTRA_BEGIN
static uint64 gettime_usecs()
{
	struct timeval tv;
	gettimeofday(&tv, null);
	return uint64(tv.tv_sec)*1000000 + uint64(tv.tv_usec);
}

Stopwatch::Stopwatch(): mData(gettime_usecs()) {}
Stopwatch::~Stopwatch() = default;

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

#elif(INTRA_LIBRARY_STOPWATCH == INTRA_LIBRARY_STOPWATCH_clock)

INTRA_BEGIN
Stopwatch::Stopwatch(): mData(clock()) {}
Stopwatch::~Stopwatch() = default;
void Stopwatch::Reset() {mData = clock();}
double Stopwatch::ElapsedSeconds() const {return double(clock() - mData) / CLOCKS_PER_SEC;}

double Stopwatch::GetElapsedSecondsAndReset()
{
	const auto newData = clock();
	const double result = double(newData - mData) / CLOCKS_PER_SEC;
	Stopwatch = newData;
	return result;
}
INTRA_END


#elif(INTRA_LIBRARY_STOPWATCH == INTRA_LIBRARY_STOPWATCH_Cpp11)

#if(defined(_MSC_VER) && !defined(__GNUC__) && !defined(_HAS_EXCEPTIONS))
#define _HAS_EXCEPTIONS 0
#endif

#include <chrono>
#include <thread>

INTRA_BEGIN
Stopwatch::Stopwatch():
	mData(size_t(new std::chrono::high_resolution_clock::time_point(std::chrono::high_resolution_clock::now()))) {}

Stopwatch::~Stopwatch()
{
	delete reinterpret_cast<std::chrono::high_resolution_clock::time_point*>(mData);
}


//Получить время таймера и сбросить его
double Stopwatch::GetElapsedSecondsAndReset()
{
	const auto current = std::chrono::high_resolution_clock::now();
	auto& prev = *reinterpret_cast<std::chrono::high_resolution_clock::time_point*>(mData);
	const auto result = std::chrono::duration<double, std::ratio<1, 1>> (current - prev).count();
	prev = current;
	return result;
}

double Stopwatch::ElapsedSeconds() const
{
	const auto current = std::chrono::high_resolution_clock::now();
	auto& prev = *reinterpret_cast<std::chrono::high_resolution_clock::time_point*>(hndl);
	return std::chrono::duration<double, std::ratio<1, 1>>(current - prev).count();
}

void Stopwatch::Reset()
{
	*reinterpret_cast<std::chrono::high_resolution_clock::time_point*>(mData) = std::chrono::high_resolution_clock::now();
}
INTRA_END

#else
#error "INTRA_LIBRARY_STOPWATCH define is invalid!"
#endif
