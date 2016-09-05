#include "Core/Time.h"
#include "Containers/String.h"
#include <time.h>

namespace Intra {

DateTime DateTime::Now()
{
	const time_t t = time(0);
	tm* now = localtime(&t);
	return {ushort(now->tm_year+1900), byte(now->tm_mon+1), (byte)now->tm_mday,
		(byte)now->tm_hour, (byte)now->tm_min, (byte)now->tm_sec};
}

String ToString(const DateTime& datetime)
{
	return String::Format()
		(datetime.Day, 2, '0')(".")(datetime.Month, 2, '0')(".")(datetime.Year)(" ")
		(datetime.Hour, 2, '0')(":")(datetime.Minute, 2, '0')(":")(datetime.Second, 2, '0');
}

#if(INTRA_LIBRARY_TIMER==INTRA_LIBRARY_TIMER_Dummy)

Timer::Timer() = default;
Timer::~Timer() = default;
void Timer::Reset() {}
double Timer::GetTime() {return 0;}
double Timer::GetTimeAndReset() {return 0;}
void Timer::Wait(uint msec) {}

#elif(INTRA_LIBRARY_TIMER==INTRA_LIBRARY_TIMER_QPC)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

static ulong64 g_timer_frequency;

Timer::Timer()
{
	QueryPerformanceFrequency((LARGE_INTEGER*)&g_timer_frequency);
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
	QueryPerformanceCounter((LARGE_INTEGER*)&current);
	double result = double(current-hndl)/g_timer_frequency;
	hndl = current;
	return result;
}

#elif(INTRA_LIBRARY_TIMER==INTRA_LIBRARY_TIMER_CPPLIB)
#include <chrono>
#include <thread>

Timer::Timer()
{
	hndl = (ulong64)new std::chrono::high_resolution_clock::time_point(std::chrono::high_resolution_clock::now());
}

Timer::~Timer()
{
	delete (std::chrono::high_resolution_clock::time_point*)hndl;
}


//Получить время таймера и сбросить его
double Timer::GetTimeAndReset()
{
	auto current = std::chrono::high_resolution_clock::now();
	auto& prev = *(std::chrono::high_resolution_clock::time_point*)hndl;
	auto result = std::chrono::duration<double, std::ratio<1,1>>(current-prev).count();
	*(std::chrono::high_resolution_clock::time_point*)hndl = current;
	return result;
}

double Timer::GetTime()
{
	auto current = std::chrono::high_resolution_clock::now();
	auto& prev = *(std::chrono::high_resolution_clock::time_point*)hndl;
	auto result = std::chrono::duration<double, std::ratio<1, 1>>(current-prev).count();
	return result;
}

void Timer::Reset()
{
	auto current = std::chrono::high_resolution_clock::now();
	*(std::chrono::high_resolution_clock::time_point*)hndl = current;
}

//void Timer::Wait(uint msec) {std::this_thread::sleep_for(std::chrono::milliseconds(msec));}


#elif INTRA_LIBRARY_TIMER==INTRA_LIBRARY_TIMER_Qt
#include <QtCore/QElapsedTimer>
#include <QtCore/QThread>

Timer::Timer()
{
	hndl = (ulong64)new QElapsedTimer;
	Reset();
}

Timer::~Timer() {delete (QElapsedTimer*)hndl;}

void Timer::Reset() {((QElapsedTimer*)hndl)->start();}

double Timer::GetTime()
{
	const double result = (double)((QElapsedTimer*)hndl)->nsecsElapsed()/1000000000;
	return result;
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

#elif INTRA_LIBRARY_TIMER==INTRA_LIBRARY_TIMER_CLIB

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

#else
#error "INTRA_LIBRARY_TIMER define is invalid!"
#endif

#if INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
void Timer::Wait(uint msec) {Sleep(msec);}

#elif defined(INTRA_PLATFORM_IS_POSIX)

#include <unistd.h>
void Timer::Wait(uint msec)
{
	if(msec < (1 << 29)/125 ) usleep(msec*1000);
	else sleep((msec+999)/1000);
}

#endif

}

