#pragma once

#include "Concurrency/Thread.h"

#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)

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

#else
#include <unistd.h>
#endif

namespace Intra { namespace Concurrency {

#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)

static bool ThisThreadSleep(ulong64 milliseconds)
{
	if(milliseconds == 0)
	{
		Yield();
		return true;
	}

	while(milliseconds >= INFINITE - 1)
	{
		::Sleep(INFINITE - 1);
		milliseconds -= INFINITE - 1;
	}
	::Sleep(DWORD(milliseconds));
	return true;
}

#else

static bool ThisThreadSleep(ulong64 milliseconds)
{
	while(milliseconds / 1000 >= 0xFFFFFFFF)
	{
		sleep(0xFFFFFFFF);
		milliseconds -= 0xFFFFFFFFULL * 1000;
	}
	if(milliseconds >= 0xFFFFFFFF / 1000)
	{
		sleep(unsigned(milliseconds / 1000));
		milliseconds = milliseconds % 1000;
	}
	usleep(unsigned(milliseconds) * 1000);
	return true;
}

#endif

}}
