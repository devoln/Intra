#pragma once



#include "Core/Core.h"
#include "Core/Core.h"

#include "Thread.h"

#define INTRA_LIBRARY_MUTEX_None 0
#define INTRA_LIBRARY_MUTEX_WinAPI 1
#define INTRA_LIBRARY_MUTEX_Cpp11 2
#define INTRA_LIBRARY_MUTEX_PThread 3

#ifndef INTRA_LIBRARY_MUTEX

#ifdef INTRA_LIBRARY_THREAD

#define INTRA_LIBRARY_MUTEX INTRA_LIBRARY_THREAD

#elif(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)

#define INTRA_LIBRARY_MUTEX INTRA_LIBRARY_MUTEX_WinAPI

#elif(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Emscripten)

#ifdef __EMSCRIPTEN_PTHREADS__
#define INTRA_LIBRARY_MUTEX INTRA_LIBRARY_MUTEX_PThread
#else
#define INTRA_LIBRARY_MUTEX INTRA_LIBRARY_MUTEX_None
#endif

#else
#define INTRA_LIBRARY_THREAD INTRA_LIBRARY_THREAD_PThread
#endif

#endif

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

INTRA_BEGIN
namespace Concurrency {

#if(INTRA_LIBRARY_MUTEX != INTRA_LIBRARY_MUTEX_None)
class Mutex
{
	friend class SeparateCondVar;
public:
#if(INTRA_LIBRARY_MUTEX == INTRA_LIBRARY_MUTEX_Cpp11)
#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
	enum {DATA_SIZE = 48};
#else
	enum {DATA_SIZE = 80}; //TODO: узнать точный размер std::mutex на разных платформах или хотя бы максимальный
#endif
#elif(INTRA_LIBRARY_MUTEX == INTRA_LIBRARY_MUTEX_WinAPI)

#ifdef INTRA_PLATFORM_IS_64
	enum {DATA_SIZE = 40};
#else
	enum {DATA_SIZE = 24};
#endif

#elif(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Linux || INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Android)
#ifdef INTRA_PLATFORM_IS_64
	enum {DATA_SIZE = 40};
#else
	enum {DATA_SIZE = 24};
#endif
#elif(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_MacOS)
	enum {DATA_SIZE = 44};
#elif(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_iOS)
	enum {DATA_SIZE = 44};
#elif(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
	enum {DATA_SIZE = sizeof(void*)};
#else
	enum {DATA_SIZE = 80};
#endif
	static const int DataSize, ImplementationType;

	union
	{
		void* mForceAlignment;
		char mData[DATA_SIZE];
	};
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

	//! @defgroup Mutex_CPP_Interface C++-11-подобный интерфейс для Mutex
	//! Этот интерфейс предназначен для совместимости с обобщённым кодом и lock_guard из библиотеки потоков C++11\Boost.
	//! Использовать напрямую этот интерфейс не рекомендуется.
	//!@{
	forceinline void lock() {Lock();}
	forceinline bool try_lock() {return TryLock();}
	forceinline void unlock() {return Unlock();}
	//!@}

private:
	Mutex(const Mutex&) = delete;
	Mutex& operator=(const Mutex&) = delete;
};

#ifdef INTRA_DEBUG
namespace detail {
static Mutex::AbiChecker gMutexCheckPerTranslationUnit;
}
#endif

#else
class Mutex;
#endif

}
using Concurrency::Mutex;

}

INTRA_WARNING_POP
