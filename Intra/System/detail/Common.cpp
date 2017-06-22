#include "Common.h"

#include "Cpp/PlatformDetect.h"
#include "Cpp/Warnings.h"

#include "Utils/StringView.h"
#include "Utils/Debug.h"
#include "Utils/ErrorStatus.h"

#include "Container/Sequential/String.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#if INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4668)
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
struct IUnknown;
#include <Windows.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#else

#include <errno.h>
#include <string.h>

#endif

namespace Intra { namespace System { namespace detail {

#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)

GenericString<wchar_t> Utf8ToWStringZ(StringView str)
{
	GenericString<wchar_t> wfn;
	wfn.SetLengthUninitialized(str.Length());
	const int wlen = MultiByteToWideChar(CP_UTF8, 0, str.Data(),
		int(str.Length()), wfn.Data(), int(wfn.Length()));
	wfn.SetLengthUninitialized(size_t(wlen + 1));
	wfn.Last() = 0;
	return wfn;
}

void ProcessLastError(ErrorStatus& status, StringView message, const Utils::SourceInfo& srcInfo)
{
	const auto le = GetLastError();
	char* s = null;
	FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
		null, le,
		DWORD(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US)),
		reinterpret_cast<char*>(&s), 0, null);
	status.Error(message + StringView(s), srcInfo);
	LocalFree(s);
}

#else

INTRA_WARNING_DISABLE_UNUSED_FUNCTION

static forceinline StringView strerrorHelper(int, const char* buf)
{return StringView(buf);}

static forceinline StringView strerrorHelper(const char* str, const char*)
{return StringView(str);}

void ProcessLastError(ErrorStatus& status, StringView message, const Utils::SourceInfo& srcInfo)
{
	char buf[128];
	status.Error(message + strerrorHelper(strerror_r(errno, buf, sizeof(buf)), buf) + "!", srcInfo);
}

#endif

}}}

INTRA_WARNING_POP
