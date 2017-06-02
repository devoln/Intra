#include "Sleep.h"

#include "Cpp/Warnings.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#if(INTRA_LIBRARY_SLEEP == INTRA_LIBRARY_SLEEP_Dummy)
namespace Intra { namespace System {

void SleepMs(uint milliseconds) {(void)milliseconds;}

}}
#elif(INTRA_LIBRARY_SLEEP == INTRA_LIBRARY_SLEEP_WinAPI)

#ifdef _MSC_VER
#pragma warning(disable: 4668)
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

namespace Intra { namespace System {

void SleepMs(uint milliseconds) {::Sleep(milliseconds);}

}}

#elif(INTRA_LIBRARY_SLEEP == INTRA_LIBRARY_SLEEP_UniStd)

#include <unistd.h>

namespace Intra { namespace System {

void SleepMs(uint milliseconds)
{
	if(milliseconds < (1 << 29) / 125 ) usleep(milliseconds * 1000);
	else sleep((milliseconds + 999) / 1000);
}

}}

#elif(INTRA_LIBRARY_SLEEP == INTRA_LIBRARY_SLEEP_CPPLIB)
void SleepMs(uint milliseconds)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}
#else
#error "Invalid INTRA_LIBRARY_SLEEP define!"
#endif

INTRA_WARNING_POP
