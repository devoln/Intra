#include "Stopwatch.h"

#include "Cpp/Warnings.h"
#include "Cpp/Compatibility.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#if(INTRA_LIBRARY_STOPWATCH == INTRA_LIBRARY_STOPWATCH_QPC)

#ifdef _MSC_VER
#pragma warning(disable: 4668)
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

namespace Intra { namespace System {

static ulong64 queryTimerPerformanceFrequency()
{
	LARGE_INTEGER li;
	QueryPerformanceFrequency(&li);
	return ulong64(li.QuadPart);
}
static const double gStopwatchPerformanceFrequency = double(queryTimerPerformanceFrequency());

Stopwatch::Stopwatch() {Reset();}

Stopwatch::~Stopwatch() = default;

void Stopwatch::Reset()
{
	LARGE_INTEGER current;
	QueryPerformanceCounter(&current);
	mData = ulong64(current.QuadPart);
}


double Stopwatch::ElapsedSeconds() const
{
	LARGE_INTEGER current;
	QueryPerformanceCounter(&current);
	return double(current.QuadPart - mData) / gStopwatchPerformanceFrequency;
}

double Stopwatch::GetElapsedSecondsAndReset()
{
	LARGE_INTEGER current;
	QueryPerformanceCounter(&current);
	const double result = double(current.QuadPart - mData) / gStopwatchPerformanceFrequency;
	mData = ulong64(current.QuadPart);
	return result;
}

}}

#elif(INTRA_LIBRARY_STOPWATCH == INTRA_LIBRARY_STOPWATCH_clock_gettime)

#include <time.h>

namespace Intra { namespace System {

static ulong64 clock_gettime_nsecs(clockid_t clkId)
{
	struct timespec ts;
	clock_gettime(clkId, &ts);
	return ulong64(ts.tv_sec*1000000000 + ts.tv_nsec);
}

Stopwatch::Stopwatch(): mData(clock_gettime_nsecs(CLOCK_PROCESS_CPUTIME_ID)) {}
Stopwatch::~Stopwatch() = default;

void Stopwatch::Reset() {mData = clock_gettime_nsecs(CLOCK_PROCESS_CPUTIME_ID);}

double Stopwatch::ElapsedSeconds() const {return double(clock_gettime_nsecs(CLOCK_PROCESS_CPUTIME_ID) - mData) / 1000000000;}

double Stopwatch::GetElapsedSecondsAndReset()
{
	const auto newData = clock_gettime_nsecs(CLOCK_PROCESS_CPUTIME_ID);
	const double result = double(newData - mData) / 1000000000;
	mData = newData;
	return result;
}

}}

#elif(INTRA_LIBRARY_STOPWATCH == INTRA_LIBRARY_STOPWATCH_gettimeofday)

#include <sys/time.h>

namespace Intra { namespace System {

static ulong64 gettime_usecs()
{
	struct timeval tv;
	gettimeofday(&tv, null);
	return tv.tv_sec*1000000 + tv.tv_usec;
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

}}

#elif(INTRA_LIBRARY_STOPWATCH == INTRA_LIBRARY_STOPWATCH_clock)

namespace Intra { namespace System {

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

}}


#elif(INTRA_LIBRARY_STOPWATCH == INTRA_LIBRARY_STOPWATCH_CPPLIB)

#if(defined(_MSC_VER) && !defined(__GNUC__) && !defined(_HAS_EXCEPTIONS))
#define _HAS_EXCEPTIONS 0
#endif

#include <chrono>
#include <thread>

namespace Intra { namespace System {

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

}}

#else
#error "INTRA_LIBRARY_STOPWATCH define is invalid!"
#endif



INTRA_WARNING_POP
