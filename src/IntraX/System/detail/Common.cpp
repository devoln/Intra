#include "Common.h"

#include "Intra/Range/StringView.h"
#include "Intra/Assert.h"
#include "IntraX/System/Error.h"

#include "IntraX/Container/Sequential/String.h"

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

namespace Intra { INTRA_BEGIN
namespace detail {
#ifdef _WIN32

GenericString<wchar_t> Utf8ToWStringZ(StringView str)
{
	GenericString<wchar_t> wfn;
	wfn.SetLengthUninitialized(str.Length());
	const int wlen = z_D::MultiByteToWideChar(65001, 0, str.Data(), int(str.Length()), wfn.Data(), int(wfn.Length()));
	wfn.SetLengthUninitialized(wlen + 1);
	wfn.Last() = 0;
	return wfn;
}

#else

INTRA_IGNORE_WARN_UNUSED_FUNCTION

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
} INTRA_END
