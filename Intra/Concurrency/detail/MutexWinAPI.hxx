#include "Concurrency/Mutex.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4668)
#endif

#include <Windows.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace Intra { namespace Concurrency {

struct Mutex::Data
{
	CRITICAL_SECTION cs;
};

Mutex::Mutex()
{
	mHandle = new Data;
	(void)InitializeCriticalSectionAndSpinCount(&mHandle->cs, 20);
}

Mutex::~Mutex()
{
	DeleteCriticalSection(&mHandle->cs);
	delete mHandle;
}

void Mutex::Lock()
{
	EnterCriticalSection(&mHandle->cs);
}

bool Mutex::TryLock()
{
	return TryEnterCriticalSection(&mHandle->cs) != 0;
}

void Mutex::Unlock()
{
	LeaveCriticalSection(&mHandle->cs);
}

}}
