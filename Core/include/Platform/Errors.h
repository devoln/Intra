#pragma once

#include "Platform/CppWarnings.h"
#include "Platform/FundamentalTypes.h"
#include "Range/ForwardDecls.h"

namespace Intra {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

enum class Error: byte {NoError, InvalidArguments, NotFound, OutOfMemory};

namespace Errors
{
	void InitSignals();
	StringView CrashSignalName(int signum);
	StringView CrashSignalDesc(int signum);
	extern void(*CrashHandler)(int signum);
}

INTRA_WARNING_POP

}
