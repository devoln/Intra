#pragma once

#include "Core/Range/StringView.h"
#include "Core/Assert.h"
#include "Utils/Delegate.h"

INTRA_BEGIN
inline namespace Utils {

enum class LogLevel: byte {All, Info, Success, PerfWarning, Warning, Error, CriticalError, None};

class ILogger
{
public:
	virtual ~ILogger() {}
	virtual void Log(LogLevel, StringView msg, const SourceInfo& = INTRA_DEFAULT_SOURCE_INFO) = 0;

	void Info(StringView msg, const SourceInfo& srcInfo = INTRA_DEFAULT_SOURCE_INFO) {Log(LogLevel::Info, msg, srcInfo);}
	void Success(StringView msg, const SourceInfo& srcInfo = INTRA_DEFAULT_SOURCE_INFO) {Log(LogLevel::Success, msg, srcInfo);}
	void PerfWarn(StringView msg, const SourceInfo& srcInfo = INTRA_DEFAULT_SOURCE_INFO) {Log(LogLevel::PerfWarning, msg, srcInfo);}
	void Warn(StringView msg, const SourceInfo& srcInfo = INTRA_DEFAULT_SOURCE_INFO) {Log(LogLevel::Warning, msg, srcInfo);}
	void Error(StringView msg, const SourceInfo& srcInfo = INTRA_DEFAULT_SOURCE_INFO) {Log(LogLevel::Error, msg, srcInfo);}
	void CriticalError(StringView msg, const SourceInfo& srcInfo = INTRA_DEFAULT_SOURCE_INFO) {Log(LogLevel::CriticalError, msg, srcInfo);}

};

class DummyLogger final: public ILogger
{
public:
	void Log(LogLevel, StringView, const SourceInfo& = {}) override {}
};

template<class O> class Logger final: public ILogger
{
public:
	template<typename... Args> forceinline Logger(Args&&... args): Output(Forward<Args>(args)...) {}

	void Log(LogLevel level, StringView msg, const SourceInfo& srcInfo = INTRA_DEFAULT_SOURCE_INFO) override
	{
		if(level < Verbosity) return;
		if(Filter != null && !Filter(level, msg, srcInfo)) return;
		if(Formatter != null) Output << Formatter(level, msg, srcInfo);
		else Output << msg << '\n';
	}

	LogLevel Verbosity = LogLevel::All;
	Delegate<void(LogLevel, StringView, const SourceInfo&)> Formatter;
	Delegate<bool(LogLevel, StringView, const SourceInfo&)> Filter;
	O Output;
};

}
INTRA_END
