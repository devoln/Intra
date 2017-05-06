#include "IO/FormattedLogger.h"
#include "Cpp/Warnings.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace IO {

void FormattedLogger::Log(LogLevel level, StringView msg, SourceInfo srcInfo)
{
	if(level<Verbosity) return;
	Math::Vec3 color;
	switch(level)
	{
	case LogLevel::Info: color = InfoColor; break;
	case LogLevel::PerfWarning: color = PerfWarningColor; break;
	case LogLevel::Warning: color = WarningColor; break;
	case LogLevel::Error: color = ErrorColor; break;
	case LogLevel::CriticalError: color = CriticalErrorColor; break;
	default: color = {0.5, 0.5, 0.5};
	}
	Writer.PushFont(color);
	INTRA_DEBUG_ASSERT(level > All && level < None);
	static const char* const names[] = {"info", "perf warning", "warning", "error", "critical error"};
	Writer.PrintLine(srcInfo.File, '(', srcInfo.Line, ") ", names[size_t(level)-1], ": ", msg);
	Writer.PopFont();
}

}}

INTRA_WARNING_POP
