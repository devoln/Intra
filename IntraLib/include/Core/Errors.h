#pragma once

#include "Core/FundamentalTypes.h"
#include "Range/ForwardDecls.h"

namespace Intra {

enum class Error: byte {NoError, InvalidArguments, NotFound, OutOfMemory};

namespace Errors
{
	void InitSignals();
	StringView CrashSignalName(int signum);
	StringView CrashSignalDesc(int signum);
	extern void(*CrashHandler)(int signum);
}

}

