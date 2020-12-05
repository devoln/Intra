#include "IntraX/IO/FormattedLogger.h"

INTRA_BEGIN
void FormattedLogger::Log(LogLevel level, StringView msg, SourceInfo srcInfo)
{
	if(level < Verbosity) return;
	Vec3 color = {0.5, 0.5, 0.5};
	switch(level)
	{
	case LogLevel::Info: color = InfoColor; break;
	case LogLevel::Success: color = SuccessColor; break;
	case LogLevel::PerfWarning: color = PerfWarningColor; break;
	case LogLevel::Warning: color = WarningColor; break;
	case LogLevel::Error: color = ErrorColor; break;
	case LogLevel::CriticalError: color = CriticalErrorColor; break;
	case LogLevel::None: case LogLevel::All:;
	}
	Writer.PushFont(color);
	static const char* const names[] = {"INFO", "SUCCESS", "PERF WARNING", "WARNING", "ERROR", "CRITICAL ERROR"};
	if(srcInfo) Writer.Print(srcInfo.File, '(', srcInfo.Line, ") ");
	if(WriteLevelType) Writer.Print(names[size_t(level) - 1], ": ");
	Writer.PrintLine(msg);
	Writer.PopFont();
}
INTRA_END
