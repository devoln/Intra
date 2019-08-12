#pragma once

#include "Container/Sequential/String.h"
#include "Utils/AnyPtr.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4668)
#endif

#include <Windows.h>
#undef Yield

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#ifdef _MSC_VER
INTRA_BEGIN
namespace Concurrency { namespace detail {

static DWORD GetThreadId(HANDLE thread)
{
#ifdef INTRA_DROP_XP_SUPPORT
	return ::GetThreadId(thread);
#else
	static struct Kernel32Dll
	{
		HMODULE lib;
		DWORD(WINAPI *GetThreadId)(HANDLE thread);

		Kernel32Dll():
			lib(LoadLibraryA("kernel32.dll")),
			GetThreadId(AnyPtr(GetProcAddress(lib, "GetThreadId"))) {}

		~Kernel32Dll() {FreeLibrary(lib);}
	} dll;
	if(dll.GetThreadId == null) return 0;
	return dll.GetThreadId(thread);
#endif
}

// Sets name of thread to see it in MSVC debugger.
// Doesn't work on XP, but MSVC 2015 also doesn't work on it.
static void SetNativeThreadName(HANDLE threadHandle, StringView name)
{
	const DWORD id = GetThreadId(threadHandle);
	if(id == 0) return;

	const DWORD MS_VC_EXCEPTION = 0x406D1388;
	char buf[128];
	Span<char>(buf) << name.Take(127) << '\0';
#pragma pack(push, 8)
	struct {DWORD dwType; const char* szName; DWORD dwThreadID; DWORD dwFlags;}
	info = {0x1000, buf, id, 0};
#pragma pack(pop)

#pragma warning(push)  
#pragma warning(disable: 6320 6322)  
	__try {RaiseException(MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(ULONG_PTR), reinterpret_cast<ULONG_PTR*>(&info));}
	__except(EXCEPTION_EXECUTE_HANDLER) {}
#pragma warning(pop)
}

}}}
#endif
