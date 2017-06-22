#pragma once

#include "Cpp/PlatformDetect.h"
#include "Utils/ErrorStatus.h"
#include "Container/ForwardDecls.h"

namespace Intra { namespace System { namespace detail {

#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
//! Converts UTF-8 to UTF-16 null-terminated string, using MultibyteToWideChar from WinAPI.
GenericString<wchar_t> Utf8ToWStringZ(StringView str);
#endif

//! Calls error on status with description of GetLastError on Windows and errno on other systems.
void ProcessLastError(ErrorStatus& status, StringView message, const Utils::SourceInfo& srcInfo);

}}}
