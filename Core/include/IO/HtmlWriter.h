#pragma once

#include "FormattedWriter.h"
#include "Algo/Hash/ToHash.h"
#include "Math/Random.h"

namespace Intra { namespace IO {

class HtmlWriter: public FormattedWriterBase
{
public:
	HtmlWriter(IOutputStream* s): FormattedWriterBase(s) {}

	void WriteData(const void* data, size_t bytes) override {mMyS->WriteData(data, bytes);}
	
	void PushFont(Math::Vec3 color={0,0,0}, float size=3,
		bool bold=false, bool italic=false, bool underline=false) override;

	void PopFont() override;

	void PushStyle(StringView style) override {RawPrint("<span class='"+style+"'>");}
	void PopStyle() override {RawPrint("</span>");}
		
	void BeginSpoiler(StringView show) override
	{
		mSpoilerNesting++;
		auto id = Math::Random<ulong64>::Global()^Algo::ToHash(show);
		RawPrint(*String::Format(SpoilerBeginCode)(id)(id)(show));
	}

	void EndSpoiler() override
	{
		FormattedWriterBase::EndSpoiler();
		RawPrint(SpoilerEndCode);
	}

	void BeginCode() override {RawPrint("\n<pre>\n");}
	void EndCode() override {RawPrint("\n</pre>\n");}

	void HorLine() override {RawPrint("<hr>");}

	IOutputStream& operator<<(endl_t) override
	{
		RawPrint("\n<br />\n");
		return *this;
	}

	void Print(StringView s) override
	{
		RawPrint(String::MultiReplace(s,
			{"&",      "<",     ">",   "\r\n",   "\n",     "\r"},
			{"&amp;", "&lt;", "&gt;", "<br />", "<br />", "<br />"}
		));
	}


	static const StringView CssSpoilerCode;
	static const StringView CssLoggerCode;
	static const StringView SpoilerBeginCode, SpoilerEndCode;

private:
	HtmlWriter(const HtmlWriter&) = delete;
	HtmlWriter& operator=(const HtmlWriter&) = delete;
};

}}
