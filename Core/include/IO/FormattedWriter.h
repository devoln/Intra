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

class IFormattedWriter: public IOutputStream
{
public:
	virtual void PushFont(Math::Vec3 color={0,0,0}, float size=3,
		bool bold=false, bool italic=false, bool underline=false) = 0;

	virtual void PopFont() = 0;

	virtual void BeginSpoiler(StringView show) = 0;
	void BeginSpoiler() {BeginSpoiler("Show");}
	virtual void EndSpoiler() = 0;
	virtual void EndAllSpoilers() = 0;

	virtual void BeginCode() = 0;
	virtual void EndCode() = 0;

	virtual void PushStyle(StringView style) = 0;
	virtual void PopStyle() = 0;

	virtual void HorLine() {PrintLine("__________________________________________________");}

	virtual FontDesc GetCurrentFont() const = 0;

	void PrintCode(StringView code)
	{BeginCode(); Print(code); EndCode();}
};

class FormattedWriterBase: public IFormattedWriter
{
protected:
	IOutputStream* mMyS;
	size_t mSpoilerNesting;
	Array<FontDesc> mFontStack;
public:
	FormattedWriterBase(IOutputStream* s):
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

	void BeginSpoiler(StringView show) override
	{
		(void)show;
		mSpoilerNesting++;
	}

	void EndSpoiler() override
	{INTRA_DEBUG_ASSERT(mSpoilerNesting!=0); mSpoilerNesting--;}
	
	void EndAllSpoilers() override
	{while(mSpoilerNesting!=0) EndSpoiler();}

	void BeginCode() override {}
	void EndCode() override   {}

	void Print(StringView s) override {RawPrint(s);}

protected:
	~FormattedWriterBase() {}
	FormattedWriterBase(const FormattedWriterBase&) = delete;
	FormattedWriterBase& operator=(const FormattedWriterBase&) = delete;
};



class PlainTextWriter: public FormattedWriterBase
{
public:
	PlainTextWriter(IOutputStream* s): FormattedWriterBase(s) {}

	void BeginSpoiler(StringView show) override
	{
		FormattedWriterBase::BeginSpoiler(show);
		PrintLine();
		RawPrint(show);
		PrintLine();
		RawPrint("{");
		PrintLine();
	}

	void EndSpoiler() override
	{
		FormattedWriterBase::EndSpoiler();
		PrintLine();
		PrintLine("}");
	}

	void BeginCode() override {PrintLine(endl, "___________________");}
	void EndCode() override   {PrintLine(endl, "___________________");}

	void Print(StringView s) override {RawPrint(s);}

private:
	PlainTextWriter(const PlainTextWriter&) = delete;
	PlainTextWriter& operator=(const PlainTextWriter&) = delete;
};

INTRA_WARNING_POP

}}
