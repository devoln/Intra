#pragma once

#include "Cpp/Fundamental.h"
#include "Cpp/PlatformDetect.h"

#define INTRA_LIBRARY_SLEEP_Dummy 0
#define INTRA_LIBRARY_SLEEP_WinAPI 1
#define INTRA_LIBRARY_SLEEP_UniStd 2
#define INTRA_LIBRARY_SLEEP_CPPLIB 3

#ifndef INTRA_LIBRARY_SLEEP

#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
#define INTRA_LIBRARY_SLEEP INTRA_LIBRARY_SLEEP_WinAPI
#elif(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Emscripten)
#define INTRA_LIBRARY_SLEEP INTRA_LIBRARY_SLEEP_Dummy
#else
#define INTRA_LIBRARY_SLEEP INTRA_LIBRARY_SLEEP_UniStd
#endif

#endif

namespace Intra { namespace System {

void SleepMs(uint milliseconds);

}}
