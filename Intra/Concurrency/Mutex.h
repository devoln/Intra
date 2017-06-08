#pragma once

#include "Cpp/PlatformDetect.h"
#include "Cpp/Warnings.h"
#include "Cpp/Features.h"
#include "Cpp/Fundamental.h"

#include "Utils/Unique.h"

#undef Yield

#define INTRA_LIBRARY_MUTEX_Dummy 0
#define INTRA_LIBRARY_MUTEX_WinAPI 1
#define INTRA_LIBRARY_MUTEX_CPPLIB 2
#define INTRA_LIBRARY_MUTEX_PThread 3

#ifndef INTRA_LIBRARY_MUTEX

#ifdef INTRA_LIBRARY_THREAD

#define INTRA_LIBRARY_MUTEX INTRA_LIBRARY_THREAD

#elif(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)

#define INTRA_LIBRARY_MUTEX INTRA_LIBRARY_MUTEX_WinAPI

#elif(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Emscripten)

#ifdef __EMSCRIPTEN_PTHREADS__
#define INTRA_LIBRARY_THREAD INTRA_LIBRARY_THREAD_PThread
#else
#define INTRA_LIBRARY_THREAD INTRA_LIBRARY_THREAD_Dummy
#endif

#else

#define INTRA_LIBRARY_THREAD INTRA_LIBRARY_THREAD_PThread

#endif

#endif

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Concurrency {

class Mutex
{
#if(INTRA_LIBRARY_MUTEX != INTRA_LIBRARY_MUTEX_Dummy)
	struct Data;
	typedef Data* Handle;
	Handle mHandle;
#endif
	struct NativeData;
public:
	typedef NativeData* NativeHandle;

	Mutex();
	~Mutex();

	void Lock();
	bool TryLock();
	void Unlock();

private:
	Mutex(const Mutex&) = delete;
	Mutex& operator=(const Mutex&) = delete;
};

}
using Concurrency::Mutex;

}

INTRA_WARNING_POP
