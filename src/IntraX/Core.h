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
//Visual Studio 2019 has no Windows XP toolkit version.
//Earlier versions are not supported by Intra.
#define INTRA_DROP_XP_SUPPORT
#endif

#define INTRA_BEGIN namespace IntraX { INTRA_PUSH_ENABLE_USEFUL_WARNINGS INTRA_DISABLE_REDUNDANT_WARNINGS
#define INTRA_END INTRA_WARNING_POP }

namespace IntraX {
using namespace Intra;
}
