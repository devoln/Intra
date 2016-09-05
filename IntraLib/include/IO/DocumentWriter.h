#pragma once

#include "Algorithms/Range.h"
#include "Containers/String.h"
#include "Stream.h"
#include "Containers/StringView.h"
#include "Containers/Array.h"

namespace Intra { namespace IO {

class IDocumentWriter: public IOutputStream
{
public:
	virtual void PushFont(Math::Vec3 color={0,0,0}, float size=3) = 0;
	virtual void PopFont() = 0;
	virtual void PushUnderline() = 0;
	virtual void PopUnderline() = 0;
	virtual void PushBold() = 0;
	virtual void PopBold() = 0;
	virtual void PushItalic() = 0;
	virtual void PopItalic() = 0;
	virtual void BeginSpoiler(StringView show="Show", StringView hide="Hide") = 0;
	virtual void EndSpoiler() = 0;
	virtual void EndAllSpoilers() = 0;
	virtual void BeginCode() = 0;
	virtual void EndCode() = 0;

	virtual void PushStyle(StringView style) = 0;
	virtual void PopStyle() = 0;

	void PrintCode(StringView code) {BeginCode(); Print(code); EndCode();}
};

class HtmlWriter: public IDocumentWriter
{
	IOutputStream* my_s;
	size_t spoiler_nesting;
public:
	HtmlWriter(IOutputStream* s): my_s(s), spoiler_nesting(0) {}
	void WriteData(const void* data, size_t bytes) override {my_s->WriteData(data, bytes);}
		
	void PushFont(Math::Vec3 color={0,0,0}, float size=3) override
	{
		const auto c = Math::USVec3(Math::Min(color*255.0f, 255.0f));
		String colstr = String::Format()((c.x<<16)|(c.y<<8)|c.z, 6, '0', 16);
		RawPrint("<font color="+colstr+" size="+ToString(size)+">");
	}
	void PopFont() override {RawPrint("</font>");}

	void PushUnderline() override {RawPrint("<ins>");}
	void PopUnderline() override {RawPrint("</ins>");}
	void PushBold() override {RawPrint("<b>");}
	void PopBold() override {RawPrint("</b>");}
	void PushItalic() override {RawPrint("<i>");}
	void PopItalic() override {RawPrint("</i>");}

	void PushStyle(StringView style) override {RawPrint(StringView("<span class='") + style + "'>");}
	void PopStyle() override {RawPrint("</span>");}
		
	void BeginSpoiler(StringView show="Show", StringView hide="Hide") override
	{
		spoiler_nesting++;
		RawPrint("\n<div class='spoiler'><input style='width:360px;height:45px;' type='checkbox'><div class='box'>\n"
			"<span class=close>"+hide+"</span><span class=open>"+show+"</span><blockquote>\n");
	}

	void EndSpoiler() override
	{
		INTRA_ASSERT(spoiler_nesting>0);
		spoiler_nesting--;
		RawPrint("\n</blockquote></div></div>\n");
	}

	void EndAllSpoilers() override {while(spoiler_nesting!=0) EndSpoiler();}

	void BeginCode() override {RawPrint("\n<pre>\n");}
	void EndCode() override {RawPrint("\n</pre>\n");}

	IOutputStream& operator<<(endl_t) override { RawPrint("\n<br />\n"); return *this; }

	void Print(StringView s) override
	{
		RawPrint(s.MultiReplace(
			{"&",      "<",     ">",   "\r\n",   "\n",     "\r"},
			{"&amp;", "&lt;", "&gt;", "<br />", "<br />", "<br />"}
		));
	}


	static const char* const CssSpoilerCode;
	static const char* const CssLoggerCode;
};

class PlainTextWriter: public IDocumentWriter
{
	IOutputStream* my_s;
	size_t spoiler_nesting;
public:
	PlainTextWriter(IOutputStream* s) {my_s=s;}
	void WriteData(const void* data, size_t bytes) override {my_s->WriteData(data, bytes);}

	void PushFont(Math::Vec3={}, float=3) override {}
	void PopFont() override {}

	void PushUnderline() override {}
	void PopUnderline() override {}
	void PushBold() override {}
	void PopBold() override {}
	void PushItalic() override {}
	void PopItalic() override {}

	void PushStyle(StringView) override {}
	void PopStyle() override {}

	void BeginSpoiler(StringView show="Show", StringView=null) override
	{
		spoiler_nesting++;
		PrintLine();
		RawPrint(show);
		PrintLine();
		RawPrint("{");
		PrintLine();
	}

	void EndSpoiler() override
	{
		INTRA_ASSERT(spoiler_nesting!=0);
		spoiler_nesting--;
		*this << endl << "}" << endl;
	}
	void EndAllSpoilers() override {while(spoiler_nesting!=0) EndSpoiler();}

	void BeginCode() override {PrintLine(endl, "___________________");}
	void EndCode() override   {PrintLine(endl, "___________________");}

	void Print(StringView s) override {RawPrint(s);}
};

class ConsoleTextWriter: public PlainTextWriter
{
	Array<Math::Vec3> colorStack;
	bool underline;
public:
	ConsoleTextWriter(IOutputStream* s): PlainTextWriter(s), underline(false) {}

	void PushFont(Math::Vec3 color={}, float size=3) override;
	void PopFont() override;
	void PushUnderline() override;
	void PopUnderline() override;
};

}}

