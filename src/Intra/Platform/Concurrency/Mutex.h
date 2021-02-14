#pragma once

#include "Thread.h"

#define INTRA_LIBRARY_MUTEX_None 0
#define INTRA_LIBRARY_MUTEX_WinAPI 1
#define INTRA_LIBRARY_MUTEX_PThread 2

#ifndef INTRA_LIBRARY_MUTEX
#ifdef INTRA_LIBRARY_THREAD
#define INTRA_LIBRARY_MUTEX INTRA_LIBRARY_THREAD
#elif defined(_WIN32)
#define INTRA_LIBRARY_MUTEX INTRA_LIBRARY_MUTEX_WinAPI
#elif defined(__EMSCRIPTEN__)
#ifdef __EMSCRIPTEN_PTHREADS__
#define INTRA_LIBRARY_MUTEX INTRA_LIBRARY_MUTEX_PThread
#else
#define INTRA_LIBRARY_MUTEX INTRA_LIBRARY_MUTEX_None
#endif
#else
#define INTRA_LIBRARY_MUTEX INTRA_LIBRARY_THREAD_PThread
#endif
#endif

namespace Intra { INTRA_BEGIN
#if(INTRA_LIBRARY_MUTEX != INTRA_LIBRARY_MUTEX_None)
class Mutex
{
	friend class SeparateCondVar;
	static constexpr size_t DATA_SIZE =
		(INTRA_LIBRARY_MUTEX == INTRA_LIBRARY_MUTEX_WinAPI || TargetOS == OperatingSystem::Linux || TargetOS == OperatingSystem::Android)?
			sizeof(void*) == 8? 40: 24:
		(TargetOS == OperatingSystem::IOS || TargetOS == OperatingSystem::MacOS)? 44:
		TargetOS == OperatingSystem::Windows? sizeof(void*): 80;
public:
	static const int DataSize, ImplementationType;

	alignas(sizeof(void*)) char mData[DATA_SIZE];
public:

#ifdef INTRA_DEBUG
	struct AbiChecker
	{
		AbiChecker()
		{
			INTRA_ASSERT(Mutex::DataSize == Mutex::DATA_SIZE && "Configurations of library and other projects must be identical!");
		}
	};
#endif

	Mutex();
	~Mutex();

	void Lock();
	bool TryLock();
	void Unlock();

	/** @defgroup Mutex_CPP_Interface Only for C++11/Boost compatibility
	*/
	///@{
	void lock() {Lock();}
	bool try_lock() {return TryLock();}
	void unlock() {return Unlock();}
	///@}

private:
	Mutex(const Mutex&) = delete;
	Mutex& operator=(const Mutex&) = delete;
};

#ifdef INTRA_DEBUG
namespace z_D {
static Mutex::AbiChecker gMutexCheckPerTranslationUnit;
}
#endif

#else
class Mutex;
#endif

} INTRA_END
