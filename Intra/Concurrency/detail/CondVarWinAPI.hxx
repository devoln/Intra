#pragma once

#include "Concurrency/CondVar.h"

#include "System/DateTime.h"

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

#include "Utils/AnyPtr.h"

namespace Intra { namespace Concurrency {

#ifndef INTRA_DROP_XP_SUPPORT
static void(WINAPI *InitializeConditionVariable)(PCONDITION_VARIABLE cv) = null;
static BOOL(WINAPI *SleepConditionVariableCS)(PCONDITION_VARIABLE cv, PCRITICAL_SECTION cs, DWORD dwMilliseconds) = null;
static void(WINAPI *WakeConditionVariable)(PCONDITION_VARIABLE cv) = null;
static void(WINAPI *WakeAllConditionVariable)(PCONDITION_VARIABLE cv) = null;


static bool initCondVarApiFunctions()
{
	if(PVOID(InitializeConditionVariable) == PVOID(1)) return false;

	const HMODULE kernel32 = GetModuleHandleA("kernel32.dll");

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

struct BasicCondVarData
{
	virtual ~BasicCondVarData() {}
	virtual bool Wait(Mutex& mutex) = 0;
	virtual bool Wait(Mutex& mutex, ulong64 timeOutMs) = 0;
	virtual void Notify() = 0;
	virtual void NotifyAll() = 0;
};
#endif

}}

#ifndef INTRA_DROP_XP_SUPPORT
#include "CondVarWinXP.hxx"
#endif

namespace Intra { namespace Concurrency {

#ifdef INTRA_DROP_XP_SUPPORT
struct BasicCondVarData
#else
struct VistaCondVar: BasicCondVarData
#endif
{
#ifdef INTRA_DROP_XP_SUPPORT
	BasicCondVarData()
#else
	VistaCondVar()
#endif
	{
		InitializeConditionVariable(&cv);
	}

	bool Wait(Mutex& mutex)
	{
		auto cs = PCRITICAL_SECTION(&mutex);
		return SleepConditionVariableCS(&cv, cs, INFINITE) == 0;
	}

	bool Wait(Mutex& mutex, ulong64 timeOutMs)
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
#ifdef INTRA_DROP_XP_SUPPORT
	static_assert(DATA_SIZE >= sizeof(BasicCondVarData), "Invalid DATA_SIZE in CondVar.h!");
	new(mData) BasicCondVarData;
#else
	static_assert(DATA_SIZE >= sizeof(XPCondVar) && DATA_SIZE >= sizeof(VistaCondVar), "Invalid DATA_SIZE in CondVar.h!");
	if(initCondVarApiFunctions()) new(mData) VistaCondVar;
	else new(mData) XPCondVar;
#endif
}

SeparateCondVar::~SeparateCondVar() {reinterpret_cast<BasicCondVarData*>(mData)->~BasicCondVarData();}

bool SeparateCondVar::wait(Mutex& mutex) {return reinterpret_cast<BasicCondVarData*>(mData)->Wait(mutex);}
bool SeparateCondVar::waitUntil(Mutex& mutex, ulong64 absTimeMs) {return reinterpret_cast<BasicCondVarData*>(mData)->Wait(mutex, absTimeMs - DateTime::AbsTimeMs());}
void SeparateCondVar::Notify() {reinterpret_cast<BasicCondVarData*>(mData)->Notify();}
void SeparateCondVar::NotifyAll() {reinterpret_cast<BasicCondVarData*>(mData)->NotifyAll();}

AnyPtr SeparateCondVar::NativeHandle()
{
#ifndef INTRA_DROP_XP_SUPPORT
	if(!initCondVarApiFunctions()) return null;
	return &reinterpret_cast<VistaCondVar*>(mData)->cv;
#else
	return &reinterpret_cast<BasicCondVarData*>(mData)->cv;
#endif
}

}}
