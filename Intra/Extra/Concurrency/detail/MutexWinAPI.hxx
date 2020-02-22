#include "Concurrency/Mutex.h"
#include "Concurrency/Thread.h"

#include "Core/Assert.h"
#include "Container/Sequential/String.h"

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

INTRA_BEGIN
Mutex::Mutex()
{
	static_assert(sizeof(mData) >= sizeof(CRITICAL_SECTION), "Invalid DATA_SIZE in Mutex.h!");
	(void)InitializeCriticalSectionAndSpinCount(new(Construct, mData) CRITICAL_SECTION, 20);
}

Mutex::~Mutex() {DeleteCriticalSection(PCRITICAL_SECTION(mData));}

void Mutex::Lock()
{
	EnterCriticalSection(PCRITICAL_SECTION(mData));
	INTRA_ASSERT1(PCRITICAL_SECTION(mData)->RecursionCount == 1, ThisThread.Name());
}

bool Mutex::TryLock()
{
	return TryEnterCriticalSection(PCRITICAL_SECTION(mData)) != 0;
}

void Mutex::Unlock()
{
	INTRA_ASSERT1(PCRITICAL_SECTION(mData)->RecursionCount == 1, ThisThread.Name());
	LeaveCriticalSection(PCRITICAL_SECTION(mData));
}
INTRA_END
