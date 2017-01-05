#pragma once

#include "Platform/CppFeatures.h"
#include "Range/ArrayRange.h"
#include "Range/StringView.h"
#include "Containers/Array.h"
#include "IO/DocumentWriter.h"

namespace Intra { namespace IO {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

class Logger: public IDocumentWriter
{
public:
	Logger(): attached(null) {}
	Logger(ArrayRange<IDocumentWriter* const> streams): attached(streams) {}

	void Attach(IDocumentWriter* stream) {attached.AddLast(stream);}
	void Detach(IDocumentWriter* stream) {attached.FindAndRemoveUnordered(stream);}

	bool operator==(null_t) const {return attached==null;}
	bool operator!=(null_t) const {return attached!=null;}
	operator bool() const {return *this!=null;}

	void PushFont(Math::Vec3 color={0,0,0}, float size=3, bool bold=false, bool italic=false, bool underline=false) override
	{
		for(auto stream: attached) stream->PushFont(color, size, bold, italic, underline);
	}

	void PopFont() override {for(auto stream: attached) stream->PopFont();}

	virtual FontDesc GetCurrentFont() const override
	{
		if(attached.Empty()) return {{1,1,1}, 3, false, false, false, false};
		return attached.First()->GetCurrentFont();
	}

	void PushStyle(StringView style) override {for(auto stream: attached) stream->PushStyle(style);}
	void PopStyle() override {for(auto stream: attached) stream->PopStyle();}

	void BeginSpoiler(StringView show="Show", StringView hide="Hide") override {for(auto stream: attached) stream->BeginSpoiler(show, hide);}
	void EndSpoiler() override {for(auto stream: attached) stream->EndSpoiler();}
	void EndAllSpoilers() override {for(auto stream: attached) stream->EndAllSpoilers();}
	void BeginCode() override {for(auto stream: attached) stream->BeginCode();}
	void EndCode() override {for(auto stream: attached) stream->EndCode();}

	Logger& operator<<(endl_t) override {for(auto stream: attached) *stream << endl; return *this;}
	void Print(StringView s) override {for(auto stream: attached) stream->Print(s);}

private:
	virtual void WriteData(const void* data, size_t bytes) override
	{
		for(auto stream: attached)
			stream->WriteData(data, bytes);
	}
	Array<IDocumentWriter*> attached;
};





struct DummyLogger
{
	template<typename T> forceinline DummyLogger& operator<<(const T&) {return *this;}
	forceinline void Attach(IDocumentWriter*) {}
	forceinline void Detach(IDocumentWriter*) {}
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
Logger
#else
DummyLogger
#endif
DebugLogger;

typedef
#ifndef INTRA_NO_LOGGING
Logger
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
