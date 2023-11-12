#pragma once

#include <Intra/Core.h>

namespace Intra { INTRA_BEGIN
enum class LogLevel {
	None,
	Fatal, Error, Warn, PerfWarn, Success, Info, Debug, Trace,
	EnumLength
};

template<typename TLogger> struct LogProxy
{
	TLogger& Logger;
	SourceInfo CallerInfo;

    template<typename... Args> void Log(LogLevel level, Args&&... args)
    {
        Logger.Log(level, CallerInfo, Logger.Format(INTRA_FWD(args)...));
    }

    template<typename... Args> void Trace(Args&&... args) {Log(LogLevel::Trace, INTRA_FWD(args)...);}
    template<typename... Args> void Debug(Args&&... args) {Log(LogLevel::Debug, INTRA_FWD(args)...);}
	template<typename... Args> void Info(Args&&... args) {Log(LogLevel::Info, INTRA_FWD(args)...);}
    template<typename... Args> void Success(Args&&... args) {Log(LogLevel::Success, INTRA_FWD(args)...);}
    template<typename... Args> void PerfWarn(Args&&... args) {Log(LogLevel::PerfWarn, INTRA_FWD(args)...);}
    template<typename... Args> void Warn(Args&&... args) {Log(LogLevel::Warn, INTRA_FWD(args)...);}
    template<typename... Args> void Error(Args&&... args) {Log(LogLevel::Error, INTRA_FWD(args)...);}
    template<typename... Args> void Fatal(Args&&... args) {Log(LogLevel::Fatal, INTRA_FWD(args)...);}
};

template<LogLevel Verbosity> constexpr auto LogStaticVerbosityFilter =
    [](LogLevel level, const SourceInfo&) {return level <= Verbosity;};

constexpr auto LogFunction = []<typename TPrint, typename TFilter = decltype(LogStaticVerbosityFilter<LogLevel::Info>)>(
    TPrint&& print, const TFilter& filter = LogStaticVerbosityFilter<LogLevel::Info>)
{
    return [=](LogLevel level, const SourceInfo& callerInfo, auto&& msg)
	{
        if(!filter(level, callerInfo)) return;
        print(level, std::forward<decltype(msg)>(msg), callerInfo);
    };
};

template<class... Fs> struct CombineFunctions: Fs...
{
    template<typename... Args> constexpr void operator()(Args&&... args) const
    {(Fs::operator()(INTRA_FWD(args)...), ...);}
};
template<class... Fs> CombineFunctions(Fs...) -> CombineFunctions<Fs...>;

template<class TFormatter, class TLogFunc> struct Logger
{
    TFormatter Format;
    TLogFunc Log;
	constexpr LogProxy<const Logger> operator()(const SourceInfo& callerInfo = SourceInfo::Current()) const {return {*this, callerInfo};}
};
template<class TFormatter, class TLogFunc> Logger(TFormatter, TLogFunc) -> Logger<TFormatter, TLogFunc>;

constexpr auto LogPrinter = [](auto&& format, auto&& output)
{
	return [=](LogLevel level, const auto& msg, const SourceInfo& srcInfo) {
		output(format(level, msg, srcInfo));
	};
};

constexpr auto LogFormatter = [](auto&& format)
{
    return [format = String(INTRA_FWD(format))](LogLevel level, auto&& msg, const SourceInfo& srcInfo)
    {
        static constexpr const char* logLevelToStringTable[int(LogLevel::EnumLength)] = {
            "", "[FATAL]", "[ERROR]", " [WARN]", " [PERF]", "   [OK]",
            " [INFO]", "[DEBUG]", "[TRACE]"
        };
        static constexpr const char* logLevelColors[int(LogLevel::EnumLength)] = {
            "", "\x1B[1;31m", "\x1B[1;31m", "\x1B[1;33m", "\x1B[1;33m", "\x1B[1;32m",
            "\x1B[1;37m", "\x1B[1;37m", "\x1B[1;37m"
        };
        return StringSprintf(format,
            logLevelColors[int(level)], srcInfo.file_name(), srcInfo.line(),
            logLevelToStringTable[int(level)], srcInfo.function_name(), msg.c_str());
    };
};

} INTRA_END
