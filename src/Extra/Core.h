#pragma once

#include "Intra/Core.h"

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

enum class OperatingSystem {Windows, Linux, Android, FreeBSD, Emscripten, IOS, MacOS, Unknown};

constexpr OperatingSystem TargetOS = OperatingSystem::
#ifdef _WIN32
	Windows;
#elif defined(__linux__)
	Linux;
#elif defined(__ANDROID__)
	Android;
#elif defined(__FreeBSD__)
	FreeBSD;
#elif defined(__EMSCRIPTEN__)
	Emscripten;
#elif defined(__APPLE__)
#ifdef TARGET_OS_IPHONE
    IOS;
#elif TARGET_IPHONE_SIMULATOR
    IOS;
#elif TARGET_OS_MAC
    MacOS;
#else
    Unknown;
#endif
#else
    Unknown;
#endif

#if defined(_MSC_VER) && !defined(__clang__)
#if _MSC_VER >= 1920 //Visual Studio 2019 has no Windows XP toolkit version
#define EXTRA_DROP_XP_SUPPORT
#endif
#endif

#define EXTRA_BEGIN namespace Extra { INTRA_PUSH_ENABLE_USEFUL_WARNINGS INTRA_DISABLE_REDUNDANT_WARNINGS
#define EXTRA_END INTRA_WARNING_POP }

namespace Extra {
using namespace Intra;
}

#ifndef EXTRA_MINEXE
//! Define EXTRA_MINEXE to:
//0) default value
//1) to disable some not very important manual optimizations (slight slowdown)
//2) to disable some important manual optimizations (moderate slowdown)
//3) to allow pessimizations to reduce exe size by all means (not recommended - this can even increase algorithmic complexity)
#define EXTRA_MINEXE 0
#endif
