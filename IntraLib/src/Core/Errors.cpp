#include "Core/Errors.h"
#include "Range/StringView.h"

#include <signal.h>
#include <stdlib.h>

namespace Intra {

static void INTRA_CRTDECL on_crash(int signum)
{
	if(Errors::CrashHandler!=null)
		Errors::CrashHandler(signum);
	INTRA_INTERNAL_ERROR(Errors::CrashSignalDesc(signum));
}


namespace Errors {

void InitSignals()
{
	signal(SIGSEGV, on_crash);
	signal(SIGTERM, on_crash);
	signal(SIGILL, on_crash);
	//signal(SIGABRT, on_crash);
	signal(SIGFPE, on_crash);
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
