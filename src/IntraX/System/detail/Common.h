#pragma once

#include "Intra/Assert.h"
#include "Intra/Range/StringView.h"
#include "IntraX/System/Error.h"
#include "IntraX/Container/ForwardDecls.h"

namespace Intra { INTRA_BEGIN
namespace detail {

#ifdef _WIN32
/// Converts UTF-8 to UTF-16 nullptr-terminated string, using MultibyteToWideChar from WinAPI.
GenericString<wchar_t> Utf8ToWStringZ(StringView str);
#endif

/// Calls error on status with description of GetLastError on Windows and errno on other systems.
void ProcessLastError(ErrorReporter err, StringView message, SourceInfo srcInfo = SourceInfo::Current());

}
} INTRA_END
