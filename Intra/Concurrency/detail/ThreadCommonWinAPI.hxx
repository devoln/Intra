#pragma once

#include "Container/Sequential/String.h"

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
namespace Intra { namespace Concurrency { namespace detail {
void SetNativeThreadName(HANDLE threadHandle, StringView name)
{
	const DWORD MS_VC_EXCEPTION = 0x406D1388;
	char buf[128];
	Span<char>(buf) << name.Take(127) << '\0';
#pragma pack(push, 8)
	struct {DWORD dwType; const char* szName; DWORD dwThreadID; DWORD dwFlags;}
	info = {0x1000, buf, ::GetThreadId(threadHandle), 0};
#pragma pack(pop)

#pragma warning(push)  
#pragma warning(disable: 6320 6322)  
	__try {RaiseException(MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(ULONG_PTR), reinterpret_cast<ULONG_PTR*>(&info));}
	__except(EXCEPTION_EXECUTE_HANDLER) {}
#pragma warning(pop)
}

}}}
#endif
