#include "Platform/Compatibility.h"
#include "Platform/Time.h"
#include "Containers/String.h"

#include <time.h>


namespace Intra {

DateTime DateTime::Now()
{
	const time_t t = time(0);
	tm* now = localtime(&t);
	return {ushort(now->tm_year+1900), byte(now->tm_mon+1), byte(now->tm_mday),
		byte(now->tm_hour), byte(now->tm_min), byte(now->tm_sec)};
}

String ToString(const DateTime& datetime)
{
	return String::Format()
		(datetime.Day, 2, '0')(".")(datetime.Month, 2, '0')(".")(datetime.Year)(" ")
		(datetime.Hour, 2, '0')(":")(datetime.Minute, 2, '0')(":")(datetime.Second, 2, '0');
}

}

#if(INTRA_LIBRARY_TIMER==INTRA_LIBRARY_TIMER_Dummy)

namespace Intra {

Timer::Timer() = default;
Timer::~Timer() = default;
void Timer::Reset() {}
double Timer::GetTime() {return 0;}
double Timer::GetTimeAndReset() {return 0;}
void Timer::Wait(uint msec) {}

}

#elif(INTRA_LIBRARY_TIMER==INTRA_LIBRARY_TIMER_QPC)

#ifdef _MSC_VER
#pragma warning(disable: 4668)
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace Intra {

static ulong64 g_timer_frequency;

Timer::Timer()
{
	QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&g_timer_frequency));
	Reset();
}

Timer::~Timer() = default;

void Timer::Reset() {GetTimeAndReset();}


double Timer::GetTime()
{
	const ulong64 oldHndl = hndl;
	const double result = GetTimeAndReset();
	hndl = oldHndl;
	return result;
}

double Timer::GetTimeAndReset()
{
	ulong64 current;
	QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&current));
	double result = double(current-hndl)/double(g_timer_frequency);
	hndl = current;
	return result;
}

}

#elif(INTRA_LIBRARY_TIMER==INTRA_LIBRARY_TIMER_CPPLIB)

#if(defined(_MSC_VER) && !defined(__GNUC__) && !defined(_HAS_EXCEPTIONS))
#define _HAS_EXCEPTIONS 0
#endif

#include <chrono>
#include <thread>

namespace Intra {

Timer::Timer():
	hndl(size_t(new std::chrono::high_resolution_clock::time_point(std::chrono::high_resolution_clock::now()))) {}

Timer::~Timer()
{
	delete reinterpret_cast<std::chrono::high_resolution_clock::time_point*>(hndl);
}


//Получить время таймера и сбросить его
double Timer::GetTimeAndReset()
{
	auto current = std::chrono::high_resolution_clock::now();
	auto& prev = *reinterpret_cast<std::chrono::high_resolution_clock::time_point*>(hndl);
	auto result = std::chrono::duration<double, std::ratio<1,1>>(current-prev).count();
	*reinterpret_cast<std::chrono::high_resolution_clock::time_point*>(hndl) = current;
	return result;
}

double Timer::GetTime()
{
	auto current = std::chrono::high_resolution_clock::now();
	auto& prev = *reinterpret_cast<std::chrono::high_resolution_clock::time_point*>(hndl);
	auto result = std::chrono::duration<double, std::ratio<1, 1>>(current-prev).count();
	return result;
}

void Timer::Reset()
{
	auto current = std::chrono::high_resolution_clock::now();
	*reinterpret_cast<std::chrono::high_resolution_clock::time_point*>(hndl) = current;
}

//void Timer::Wait(uint msec) {std::this_thread::sleep_for(std::chrono::milliseconds(msec));}

}

#elif INTRA_LIBRARY_TIMER==INTRA_LIBRARY_TIMER_Qt
#include <QtCore/QElapsedTimer>
#include <QtCore/QThread>

namespace Intra {

Timer::Timer()
{
	hndl = reinterpret_cast<size_t>(new QElapsedTimer);
	Reset();
}

Timer::~Timer() {delete reinterpret_cast<QElapsedTimer*>(hndl);}

void Timer::Reset() {reinterpret_cast<QElapsedTimer*>(hndl)->start();}

double Timer::GetTime()
{
	return double(reinterpret_cast<QElapsedTimer*>(hndl)->nsecsElapsed())/1000000000.0;
}

double Timer::GetTimeAndReset()
{
	const double result = GetTime();
	Reset();
	return result;
}

/*void Timer::Wait(uint msec)
{
	struct SleepThread: public QThread {using QThread::msleep;};
	SleepThread::msleep(msec);
}*/

}

#elif INTRA_LIBRARY_TIMER==INTRA_LIBRARY_TIMER_CLIB

namespace Intra {

Timer::Timer() {hndl = clock();}
Timer::~Timer() = default;
void Timer::Reset() {hndl = clock();}
double Timer::GetTime() {return double(clock()-hndl)/CLOCKS_PER_SEC;}

double Timer::GetTimeAndReset()
{
	auto newHndl = clock();
	const double result = double(newHndl-hndl)/CLOCKS_PER_SEC;
	hndl = newHndl;
	return result;
}

}

#else
#error "INTRA_LIBRARY_TIMER define is invalid!"
#endif

#if INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

namespace Intra {

void Timer::Wait(uint msec) {Sleep(msec);}

}

#elif defined(INTRA_PLATFORM_IS_UNIX)

#include <unistd.h>

namespace Intra {

void Timer::Wait(uint msec)
{
	if(msec < (1 << 29)/125 ) usleep(msec*1000);
	else sleep((msec+999)/1000);
}

}

#endif

