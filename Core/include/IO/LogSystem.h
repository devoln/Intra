#pragma once

#include "Platform/CppFeatures.h"
#include "Range/Generators/ArrayRange.h"
#include "Range/Generators/StringView.h"
#include "Container/Sequential/Array.h"
#include "IO/FormattedWriter.h"
#include "IO/CompositeFormattedWriter.h"

namespace Intra { namespace IO {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

struct DummyLogger
{
	template<typename T> forceinline DummyLogger& operator<<(const T&) {return *this;}
	forceinline void Attach(IFormattedWriter*) {}
	forceinline void Detach(IFormattedWriter*) {}
	forceinline bool operator==(null_t) const {return true;}
	forceinline bool operator!=(null_t) const {return false;}

	forceinline operator bool() const {return false;}

	forceinline void PushFont(Math::Vec3={1,1,1}, float=3, bool=false, bool=false, bool=false) {}
	forceinline void PopFont() {}
	forceinline FontDesc GetCurrentFont() const {return {{1,1,1}, 3, false, false, false, false};}

	forceinline void BeginSpoiler(StringView=null, StringView=null) {}
	forceinline void EndSpoiler() {}
	forceinline void EndAllSpoilers() {}
	forceinline void BeginCode() {}
	forceinline void EndCode() {}



	forceinline void PushStyle(StringView) {}
	forceinline void PopStyle() {}
};

typedef
#ifdef INTRA_DEBUG
CompositeFormattedWriter
#else
DummyLogger
#endif
DebugLogger;

typedef
#ifndef INTRA_NO_LOGGING
CompositeFormattedWriter
#else
DummyLogger
#endif
AppLogger;

extern AppLogger ErrorLog, WarnLog, InfoLog;



//! Это более новый и адаптированный под логирование интерфейс, но он ещё не готов и нигде не используется.

enum class LogDetail {Never, OnCriticalError, OnError, OnWarning, OnPerfWarning, OnPerfProfile, Always};

union LogChannels
{
	enum I {Assert=1, CriticalError=2, Error=4, Warning=8, PerfWarning=16, PerfProfile=32};
	struct Flags
	{
		uint Assert: 1;
		uint CriticalError: 1;
		uint Error: 1;
		uint Warning: 1;
		uint PerfWarning: 1;
		uint PerfProfile: 1;
	};
	Flags flags;
	I value;

	operator uint() const {return value;}
};

class NewLogger
{
public:
	struct Criteria
	{
		size_t channel, type;
		LogDetail verbosity;
		const char* file;
		int line;
		String message;
	};

	virtual ~NewLogger() {}
	virtual void Log(size_t channel, size_t type, LogDetail verbosity, const char* file, int line, StringView message) = 0;
};

template<class FormatPolicy, class WritePolicy, class FilterPolicy>
class LoggerImpl: public NewLogger
{
public:
	LoggerImpl(LogDetail verbosity=LogDetail::Always): Verbosity(verbosity) {}

	void Log(size_t channel, size_t type, LogDetail verbosity, const char* file, int line, StringView message) override
	{
		if(verbosity<Verbosity) return;
		Criteria criteria = {channel, type, verbosity, file, line, message};
		if(!filter.Filter(criteria)) return;
		writer.Write(formatter.Format(criteria));
	}

	LogDetail Verbosity;
	LogChannels Channels;

private:
	FormatPolicy formatter;
	WritePolicy writer;
	FilterPolicy filter;
};

namespace LogPolicies {

struct NoFilter
{
	bool Filter(const NewLogger::Criteria&) {return true;}
};

struct ChannelFilter
{
	ChannelFilter(LogChannels channels): Channels(channels) {}

	bool Filter(const NewLogger::Criteria& criteria)
	{
		return (criteria.channel & size_t(Channels.value))==criteria.channel;
	}

	LogChannels Channels;
};

struct VerbosityFilter
{
	VerbosityFilter(LogDetail verbosity): Verbosity(verbosity) {}

	bool Filter(const NewLogger::Criteria& criteria)
	{
		return criteria.verbosity>=Verbosity;
	}

	LogDetail Verbosity;
};

template<typename T1, typename T2> class CombineFilters
{
	T1 t1;
	T2 t2;
public:
	bool Filter(const NewLogger::Criteria& criteria)
	{
		return t1.Filter(criteria) && t2.Filter(criteria);
	}
};

template<typename T1, typename T2, typename T3>
using CombineFilters3 = CombineFilters<CombineFilters<T1, T2>, T3>;
template<typename T1, typename T2, typename T3, typename T4>
using CombineFilters4 = CombineFilters<CombineFilters<T1, T2>, CombineFilters<T3, T4>>;

struct PlainFormat
{
	String Format(const NewLogger::Criteria& criteria)
	{
		return criteria.message;
	}
};

struct CustomFormat
{
	CustomFormat(bool channel, bool source, bool type):
		ShowChannel(channel), ShowSource(source), ShowType(type) {}

	bool ShowChannel, ShowSource, ShowType;

	String Format(const NewLogger::Criteria& criteria)
	{
		String result;
		if(ShowSource) result = String::Format()(criteria.file)("(")(criteria.line)("): ");
		if(ShowChannel) result += "["+ToString(criteria.channel)+"] ";
		if(ShowType) result += "("+ToString(criteria.type)+") ";
		result += criteria.message;
		return result;
	}
};

struct HtmlFormat
{
	HtmlFormat(bool channel, bool source, bool type):
		ShowChannel(channel), ShowSource(source), ShowType(type) {}

	bool ShowChannel, ShowSource, ShowType;

	void Init(){}

	String Format(const NewLogger::Criteria& criteria)
	{
		String result;
		if(ShowSource) result = String::Format()(criteria.file)("(")(criteria.line)("): ");
		if(ShowChannel) result += "["+ToString(criteria.channel)+"] ";
		if(ShowType) result += "("+ToString(criteria.type)+") ";
		result+=criteria.message;
		return result;
	}
};

struct StreamWriter
{
	StreamWriter(IO::IOutputStream& stream): MyStream(stream) {}

	void Write(StringView str)
	{
		MyStream << str;
	}

	IO::IOutputStream& MyStream;

	StreamWriter& operator=(const StreamWriter&) = delete;
};
}

INTRA_WARNING_POP

}}
