#pragma once

#include "IntraX/Concurrency/CondVar.h"

#include "IntraX/System/DateTime.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

INTRA_PUSH_DISABLE_ALL_WARNINGS
#include <Windows.h>
INTRA_WARNING_POP

INTRA_BEGIN
#ifndef INTRA_DROP_XP_SUPPORT
static void(WINAPI *InitializeConditionVariable)(PCONDITION_VARIABLE cv) = null;
static BOOL(WINAPI *SleepConditionVariableCS)(PCONDITION_VARIABLE cv, PCRITICAL_SECTION cs, DWORD dwMilliseconds) = null;
static void(WINAPI *WakeConditionVariable)(PCONDITION_VARIABLE cv) = null;
static void(WINAPI *WakeAllConditionVariable)(PCONDITION_VARIABLE cv) = null;


static bool initCondVarApiFunctions()
{
	if(PVOID(InitializeConditionVariable) == PVOID(1)) return false;

	const HMODULE kernel32 = GetModuleHandleA("kernel32.dll");

	INTRA_IGNORE_WARN_CLANG("undefined-reinterpret-cast")
#define F(name) reinterpret_cast<FARPROC&>(name) = GetProcAddress(kernel32, #name)
	F(InitializeConditionVariable);
	F(SleepConditionVariableCS);
	F(WakeConditionVariable);
	F(WakeAllConditionVariable);
#undef F

	if(InitializeConditionVariable != null) return true;

	reinterpret_cast<PVOID&>(InitializeConditionVariable) = PVOID(1);
	return false;
}
#endif

INTRA_END

INTRA_BEGIN
struct BasicCondVarData
{
	BasicCondVarData()
	{
		InitializeConditionVariable(&cv);
	}

	bool Wait(Mutex& mutex)
	{
		auto cs = PCRITICAL_SECTION(&mutex);
		return SleepConditionVariableCS(&cv, cs, INFINITE) == 0;
	}

	bool Wait(Mutex& mutex, uint64 timeOutMs)
	{
		auto cs = PCRITICAL_SECTION(&mutex);
		while(timeOutMs >= INFINITE - 1)
		{
			auto result = SleepConditionVariableCS(&cv, cs, INFINITE - 1) == 0;
			if(result == 0) return true;
			if(GetLastError() != ERROR_TIMEOUT) return false;
			timeOutMs -= INFINITE - 1;
		}
		return SleepConditionVariableCS(&cv, cs, DWORD(timeOutMs)) != 0;
	}

	void Notify() {WakeConditionVariable(&cv);}

	void NotifyAll() {WakeAllConditionVariable(&cv);}

	CONDITION_VARIABLE cv;
};


SeparateCondVar::SeparateCondVar()
{
	static_assert(DATA_SIZE >= sizeof(BasicCondVarData), "Invalid DATA_SIZE in CondVar.h!");
#ifndef INTRA_DROP_XP_SUPPORT
	INTRA_ASSERT(initCondVarApiFunctions() && "Condition variables on WinXP are not supported!");
#endif
	new(Construct, mData) BasicCondVarData;
}

SeparateCondVar::~SeparateCondVar() {reinterpret_cast<BasicCondVarData*>(mData)->~BasicCondVarData();}

bool SeparateCondVar::wait(Mutex& mutex) {return reinterpret_cast<BasicCondVarData*>(mData)->Wait(mutex);}
bool SeparateCondVar::waitUntil(Mutex& mutex, uint64 absTimeMs) {return reinterpret_cast<BasicCondVarData*>(mData)->Wait(mutex, absTimeMs - DateTime::AbsTimeMs());}
void SeparateCondVar::Notify() {reinterpret_cast<BasicCondVarData*>(mData)->Notify();}
void SeparateCondVar::NotifyAll() {reinterpret_cast<BasicCondVarData*>(mData)->NotifyAll();}

void* SeparateCondVar::NativeHandle()
{
	return &reinterpret_cast<BasicCondVarData*>(mData)->cv;
}
INTRA_END
