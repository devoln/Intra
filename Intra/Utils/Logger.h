#pragma once

#include "Cpp/Features.h"
#include "StringView.h"
#include "Utils/Debug.h"
#include "Delegate.h"

namespace Intra { namespace Utils {

enum class LogLevel: byte {All, Info, Success, PerfWarning, Warning, Error, CriticalError, None};

class ILogger
{
public:
	virtual ~ILogger() {}
	virtual void Log(LogLevel, StringView msg, const Utils::SourceInfo&) = 0;

	void Info(StringView msg, const Utils::SourceInfo& srcInfo=null) {Log(LogLevel::Info, msg, srcInfo);}
	void Success(StringView msg, const Utils::SourceInfo& srcInfo=null) {Log(LogLevel::Success, msg, srcInfo);}
	void PerfWarn(StringView msg, const Utils::SourceInfo& srcInfo=null) {Log(LogLevel::PerfWarning, msg, srcInfo);}
	void Warn(StringView msg, const Utils::SourceInfo& srcInfo=null) {Log(LogLevel::Warning, msg, srcInfo);}
	void Error(StringView msg, const Utils::SourceInfo& srcInfo=null) {Log(LogLevel::Error, msg, srcInfo);}
	void CriticalError(StringView msg, const Utils::SourceInfo& srcInfo=null) {Log(LogLevel::CriticalError, msg, srcInfo);}

};

class DummyLogger final: public ILogger
{
public:
	void Log(LogLevel, StringView, const Utils::SourceInfo&) override {}
};

template<class O> class Logger final: public ILogger
{
public:
	template<typename... Args> forceinline Logger(Args&&... args): Output(Cpp::Forward<Args>(args)...) {}

	void Log(LogLevel level, StringView msg, const Utils::SourceInfo& srcInfo) override
	{
		if(level<Verbosity) return;
		if(Filter != null && !Filter(level, msg, srcInfo)) return;
		if(Formatter != null) Output << Formatter(level, msg, srcInfo);
		else Output << msg << '\n';
	}

	LogLevel Verbosity=LogLevel::All;
	Delegate<void(LogLevel, StringView, const Utils::SourceInfo&)> Formatter;
	Delegate<bool(LogLevel, StringView, const Utils::SourceInfo&)> Filter;
	O Output;
};

}
using Utils::LogLevel;
using Utils::ILogger;
using Utils::DummyLogger;
using Utils::Logger;

}

