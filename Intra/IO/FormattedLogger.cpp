#include "IO/FormattedLogger.h"


INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

INTRA_BEGIN
namespace IO {

void FormattedLogger::Log(LogLevel level, StringView msg, const Utils::SourceInfo& srcInfo)
{
	if(level < Verbosity) return;
	Math::Vec3 color;
	switch(level)
	{
	case LogLevel::Info: color = InfoColor; break;
	case LogLevel::Success: color = SuccessColor; break;
	case LogLevel::PerfWarning: color = PerfWarningColor; break;
	case LogLevel::Warning: color = WarningColor; break;
	case LogLevel::Error: color = ErrorColor; break;
	case LogLevel::CriticalError: color = CriticalErrorColor; break;
	default: color = {0.5, 0.5, 0.5};
	}
	Writer.PushFont(color);
	static const char* const names[] = {"INFO", "SUCCESS", "PERF WARNING", "WARNING", "ERROR", "CRITICAL ERROR"};
	if(srcInfo != null) Writer.Print(srcInfo.File, '(', srcInfo.Line, ") ");
	if(WriteLevelType) Writer.Print(names[size_t(level) - 1], ": ");
	Writer.PrintLine(msg);
	Writer.PopFont();
}

}}

INTRA_WARNING_POP
