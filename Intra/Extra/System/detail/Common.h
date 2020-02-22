#pragma once

#include "System/Error.h"
#include "Container/ForwardDecls.h"

INTRA_BEGIN
namespace detail {

#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
//! Converts UTF-8 to UTF-16 null-terminated string, using MultibyteToWideChar from WinAPI.
GenericString<wchar_t> Utf8ToWStringZ(StringView str);
#endif

//! Calls error on status with description of GetLastError on Windows and errno on other systems.
void ProcessLastError(ErrorReporter err, StringView message, SourceInfo srcInfo = INTRA_DEFAULT_SOURCE_INFO);

}
INTRA_END
