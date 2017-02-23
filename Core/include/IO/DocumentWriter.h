#pragma once

#include "Platform/CppWarnings.h"
#include "Math/Vector.h"
#include "Container/Sequential/String.h"
#include "Stream.h"
#include "Range/Generators/StringView.h"
#include "Container/Sequential/Array.h"

namespace Intra { namespace IO {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

struct FontDesc
{
	Math::Vec3 Color;
	float Size;
	bool Bold, Italic, Underline, Strike;

	bool operator==(const FontDesc& rhs) const
	{
		return Color==rhs.Color && Size==rhs.Size &&
			Bold==rhs.Bold && Italic==rhs.Italic &&
			Underline==rhs.Underline && Strike==rhs.Strike;
	}

	bool operator!=(const FontDesc& rhs) const {return !operator==(rhs);}
};

class IDocumentWriter: public IOutputStream
{
public:
	virtual void PushFont(Math::Vec3 color={0,0,0}, float size=3,
		bool bold=false, bool italic=false, bool underline=false) = 0;

	virtual void PopFont() = 0;

	virtual void BeginSpoiler(StringView show="Show", StringView hide="Hide") = 0;
	virtual void EndSpoiler() = 0;
	virtual void EndAllSpoilers() = 0;

	virtual void BeginCode() = 0;
	virtual void EndCode() = 0;

	virtual void PushStyle(StringView style) = 0;
	virtual void PopStyle() = 0;

	virtual FontDesc GetCurrentFont() const = 0;

	void PrintCode(StringView code)
	{BeginCode(); Print(code); EndCode();}
};

class DocumentWriterBase: public IDocumentWriter
{
protected:
	IOutputStream* mMyS;
	size_t mSpoilerNesting;
	Array<FontDesc> mFontStack;
public:
	DocumentWriterBase(IOutputStream* s):
		mMyS(s), mSpoilerNesting(0), mFontStack(null) {}

	void WriteData(const void* data, size_t bytes) override {mMyS->WriteData(data, bytes);}

	void PushFont(Math::Vec3 color={1,1,1}, float size=3,
		bool bold=false, bool italic=false, bool underline=false) override
	{mFontStack.AddLast({color, size, bold, italic, underline, false});}

	void PopFont() override
	{mFontStack.RemoveLast();}

	FontDesc GetCurrentFont() const override
	{
		if(mFontStack.Empty()) return {{0,0,0}, 0, false, false, false, false};
		return mFontStack.Last();
	}

	void PushStyle(StringView) override {}
	void PopStyle() override {}

	void BeginSpoiler(StringView show="Show", StringView hide="Hide") override
	{
		(void)show; (void)hide;
		mSpoilerNesting++;
	}

	void EndSpoiler() override
	{INTRA_ASSERT(mSpoilerNesting!=0); mSpoilerNesting--;}
	
	void EndAllSpoilers() override
	{while(mSpoilerNesting!=0) EndSpoiler();}

	void BeginCode() override {}
	void EndCode() override   {}

	void Print(StringView s) override {RawPrint(s);}

protected:
	~DocumentWriterBase() {}
	DocumentWriterBase(const DocumentWriterBase&) = delete;
	DocumentWriterBase& operator=(const DocumentWriterBase&) = delete;
};

class HtmlWriter: public DocumentWriterBase
{
public:
	HtmlWriter(IOutputStream* s): DocumentWriterBase(s) {}

	void WriteData(const void* data, size_t bytes) override {mMyS->WriteData(data, bytes);}
	
	void PushFont(Math::Vec3 color={0,0,0}, float size=3,
		bool bold=false, bool italic=false, bool underline=false) override;

	void PopFont() override;

	void PushStyle(StringView style) override {RawPrint("<span class='"+style+"'>");}
	void PopStyle() override {RawPrint("</span>");}
		
	void BeginSpoiler(StringView show="Show", StringView hide="Hide") override
	{
		mSpoilerNesting++;
		RawPrint("\n<div class='spoiler'>"
			"<input style='width:360px;height:45px;' type='checkbox'>"
			"<div class='box'>\n"
			"<span class=close>"+hide+"</span>"
			"<span class=open>"+show+"</span><blockquote>\n");
	}

	void EndSpoiler() override
	{
		DocumentWriterBase::EndSpoiler();
		RawPrint("\n</blockquote></div></div>\n");
	}

	void BeginCode() override {RawPrint("\n<pre>\n");}
	void EndCode() override {RawPrint("\n</pre>\n");}

	IOutputStream& operator<<(endl_t) override
	{ RawPrint("\n<br />\n"); return *this; }

	void Print(StringView s) override
	{
		RawPrint(String::MultiReplace(s,
			{"&",      "<",     ">",   "\r\n",   "\n",     "\r"},
			{"&amp;", "&lt;", "&gt;", "<br />", "<br />", "<br />"}
		));
	}


	static const char* const CssSpoilerCode;
	static const char* const CssLoggerCode;

private:
	HtmlWriter(const HtmlWriter&) = delete;
	HtmlWriter& operator=(const HtmlWriter&) = delete;
};

class PlainTextWriter: public DocumentWriterBase
{
public:
	PlainTextWriter(IOutputStream* s): DocumentWriterBase(s) {}

	void BeginSpoiler(StringView show="Show", StringView=null) override
	{
		DocumentWriterBase::BeginSpoiler(show, null);
		PrintLine();
		RawPrint(show);
		PrintLine();
		RawPrint("{");
		PrintLine();
	}

	void EndSpoiler() override
	{
		DocumentWriterBase::EndSpoiler();
		*this << endl << "}" << endl;
	}

	void BeginCode() override {PrintLine(endl, "___________________");}
	void EndCode() override   {PrintLine(endl, "___________________");}

	void Print(StringView s) override {RawPrint(s);}

private:
	PlainTextWriter(const PlainTextWriter&) = delete;
	PlainTextWriter& operator=(const PlainTextWriter&) = delete;
};

class ConsoleTextWriter: public PlainTextWriter
{
public:
	ConsoleTextWriter(IOutputStream* s): PlainTextWriter(s) {}

	void PushFont(Math::Vec3 color={1,1,1}, float size=3,
		bool bold=false, bool italic=false, bool underline=false) override;

	void PopFont() override;
	
	FontDesc GetCurrentFont() const override
	{
		if(mFontStack.Empty()) return {Math::Vec3(-1), 3, false, false, false, false};
		return mFontStack.Last();
	}
	
private:
	ConsoleTextWriter(const ConsoleTextWriter&) = delete;
	ConsoleTextWriter& operator=(const ConsoleTextWriter&) = delete;
};

INTRA_WARNING_POP

}}
