#pragma once


#include "Core/Core.h"
#include "Core/Range/StringView.h"

INTRA_BEGIN


INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace System
{
	void InitSignals();
	StringView CrashSignalName(int signum);
	StringView CrashSignalDesc(int signum);
	extern void(*CrashHandler)(int signum);
}

INTRA_WARNING_POP

}
