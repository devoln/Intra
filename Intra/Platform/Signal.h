#pragma once

#include "Cpp/Warnings.h"
#include "Cpp/Fundamental.h"
#include "Utils/StringView.h"

namespace Intra {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Errors
{
	void InitSignals();
	StringView CrashSignalName(int signum);
	StringView CrashSignalDesc(int signum);
	extern void(*CrashHandler)(int signum);
}

INTRA_WARNING_POP

}
