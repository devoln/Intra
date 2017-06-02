#include "System/Signal.h"

#include "Cpp/Warnings.h"
#include "Cpp/Compatibility.h"

#include "Utils/StringView.h"

INTRA_DISABLE_REDUNDANT_WARNINGS

#include <signal.h>
#include <stdlib.h>

namespace Intra {

static void INTRA_CRTDECL SignalHandler(int signum)
{
	if(System::CrashHandler != null)
		System::CrashHandler(signum);
	if(signum == SIGTERM) exit(1);
	INTRA_FATAL_ERROR(System::CrashSignalDesc(signum));
}


namespace System {

void InitSignals()
{
	signal(SIGSEGV, SignalHandler);
	signal(SIGTERM, SignalHandler);
	signal(SIGILL, SignalHandler);
	signal(SIGABRT, SignalHandler);
	signal(SIGFPE, SignalHandler);
}

StringView CrashSignalName(int signum)
{
	switch(signum)
	{
	case SIGSEGV: return "SIGSEGV";
	case SIGTERM: return "SIGTERM";
	case SIGILL: return "SIGILL";
	case SIGABRT: return "SIGABRT";
	case SIGFPE: return "SIGFPE";
	default: return "Unknown";
	}
}

StringView CrashSignalDesc(int signum)
{
	switch(signum)
	{
	case SIGSEGV: return "SEGMENTATION FAULT";
	case SIGTERM: return "PROCESS TERMINATION";
	case SIGILL: return "ILLEGAL INSTRUCTION";
	case SIGABRT: return "ABORT CALL";
	case SIGFPE: return "FLOATING POINT EXCEPTION";
	default: return "UNKNOWN SIGNAL";
	}
}

void(*CrashHandler)(int signum)=null;

}

}
