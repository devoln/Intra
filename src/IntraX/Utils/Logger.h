#pragma once

#include "Intra/Range/StringView.h"
#include "Intra/Assert.h"
#include "IntraX/Utils/Delegate.h"

namespace Intra { INTRA_BEGIN
enum class LogLevel: byte {All, Info, Success, PerfWarning, Warning, Error, CriticalError, None};

class ILogger
{
public:
	virtual ~ILogger() {}
	virtual void Log(LogLevel, StringView msg, SourceInfo = SourceInfo()) = 0;

	void Info(StringView msg, SourceInfo srcInfo = SourceInfo()) {Log(LogLevel::Info, msg, srcInfo);}
	void Success(StringView msg, SourceInfo srcInfo = SourceInfo()) {Log(LogLevel::Success, msg, srcInfo);}
	void PerfWarn(StringView msg, SourceInfo srcInfo = SourceInfo()) {Log(LogLevel::PerfWarning, msg, srcInfo);}
	void Warn(StringView msg, SourceInfo srcInfo = SourceInfo()) {Log(LogLevel::Warning, msg, srcInfo);}
	void Error(StringView msg, SourceInfo srcInfo = SourceInfo()) {Log(LogLevel::Error, msg, srcInfo);}
	void CriticalError(StringView msg, SourceInfo srcInfo = SourceInfo()) {Log(LogLevel::CriticalError, msg, srcInfo);}

};

class DummyLogger final: public ILogger
{
public:
	void Log(LogLevel, StringView, SourceInfo = {}) override {}
};

template<class O> class Logger final: public ILogger
{
public:
	template<typename... Args> INTRA_FORCEINLINE Logger(Args&&... args): Output(Forward<Args>(args)...) {}

	void Log(LogLevel level, StringView msg, SourceInfo srcInfo = SourceInfo()) override
	{
		if(level < Verbosity) return;
		if(Filter != nullptr && !Filter(level, msg, srcInfo)) return;
		if(Formatter != nullptr) Output << Formatter(level, msg, srcInfo);
		else Output << msg << '\n';
	}

	LogLevel Verbosity = LogLevel::All;
	Delegate<void(LogLevel, StringView, SourceInfo)> Formatter;
	Delegate<bool(LogLevel, StringView, SourceInfo)> Filter;
	O Output;
};
} INTRA_END
