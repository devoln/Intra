#include "Common.h"

#include "Intra/Range/StringView.h"
#include "Intra/Assert.h"
#include "Extra/System/Error.h"

#include "Extra/Container/Sequential/String.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
#ifdef _WIN32

#ifdef _MSC_VER
#pragma warning(disable: 4668)
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
struct IUnknown;
#include <Windows.h>

#else

#include <errno.h>
#include <string.h>

#endif
INTRA_WARNING_POP

EXTRA_BEGIN
namespace detail {
#ifdef _WIN32

GenericString<wchar_t> Utf8ToWStringZ(StringView str)
{
	GenericString<wchar_t> wfn;
	wfn.SetLengthUninitialized(str.Length());
	const int wlen = MultiByteToWideChar(CP_UTF8, 0, str.Data(),
		int(str.Length()), wfn.Data(), int(wfn.Length()));
	wfn.SetLengthUninitialized(wlen + 1);
	wfn.Last() = 0;
	return wfn;
}

void ProcessLastError(ErrorReporter err, StringView message, SourceInfo srcInfo)
{
	const auto le = GetLastError();
	char* s = null;
	FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
		null, le,
		DWORD(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US)),
		reinterpret_cast<char*>(&s), 0, null);
	err.Error({uint32(HRESULT_FROM_WIN32(le))}, message + StringView(s), srcInfo);
	LocalFree(s);
}

#else

INTRA_IGNORE_WARNING_UNUSED_FUNCTION

inline StringView strerrorHelper(int, const char* buf)
{return StringView(buf);}

inline StringView strerrorHelper(const char* str, const char*)
{return StringView(str);}

void ProcessLastError(ErrorStatus& status, StringView message, const Utils::SourceInfo& srcInfo)
{
	char buf[128];
	status.Error(message + strerrorHelper(strerror_r(errno, buf, sizeof(buf)), buf) + "!", srcInfo);
}

#endif
}
EXTRA_END
