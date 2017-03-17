#pragma once

#include "Platform/CppWarnings.h"
#include "Math/Vector.h"
#include "Container/Sequential/String.h"
#include "Range/Generators/StringView.h"
#include "Container/Sequential/Array.h"
#include "Range/Polymorphic/OutputRange.h"

namespace Intra { namespace IO {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

struct FontDesc
{
	//! Цвет шрифта. Отрицательное значение означает, что нужно использовать цвет по умолчанию.
	Math::Vec3 Color;
	float Size;
	bool Bold, Italic, Underline, Strike;

	static const FontDesc& Default()
	{
		const FontDesc result = {{-1, -1, -1}, 3, false, false, false, false};
		return result;
	}

	bool operator==(const FontDesc& rhs) const
	{
		return Color==rhs.Color && Size==rhs.Size &&
			Bold==rhs.Bold && Italic==rhs.Italic &&
			Underline==rhs.Underline && Strike==rhs.Strike;
	}

	bool operator!=(const FontDesc& rhs) const {return !operator==(rhs);}
};

class AFormattedWriter
{
public:
	void PushFont(Math::Vec3 color, float size=3,
		bool bold=false, bool italic=false, bool underline=false, bool strike=false)
	{
		PushFont({color, size, bold, italic, underline, strike});
	}

	void PushFont(const FontDesc& fontDesc)
	{
		pushFont(fontDesc);
		mFontStack.AddLast(fontDesc);
	}

	void PushFont() {PushFont(FontDesc::Default());}

	void PopFont()
	{
		popFont();
		mFontStack.RemoveLast();
	}

	void BeginSpoiler(StringView label)
	{
		beginSpoiler(label);
		mSpoilerNesting++;
	}

	void BeginSpoiler() {BeginSpoiler("Show");}

	void EndSpoiler()
	{
		INTRA_DEBUG_ASSERT(mSpoilerNesting != 0);
		if(mSpoilerNesting == 0) return;
		mSpoilerNesting--;
		endSpoiler();
	}

	void EndAllSpoilers()
	{
		while(mSpoilerNesting --> 0)
			EndSpoiler();
	}

	virtual void BeginCode() {}
	virtual void EndCode() {}

	virtual void PushStyle(StringView style) {}
	virtual void PopStyle() {}

	virtual void HorLine()
	{
		PrintRaw("__________________________________________________");
		LineBreak();
	}

	virtual void LineBreak() {PrintRaw("\r\n");}

	const FontDesc& GetCurrentFont() const
	{
		if(mFontStack.Empty())
			return FontDesc::Default();
		return mFontStack.Last();
	}

	void PrintCode(StringView code)
	{
		BeginCode();
		PrintPreformatted(code);
		EndCode();
	}

	virtual void PrintPreformatted(StringView str) {PrintRaw(str);}
	virtual void PrintRaw(StringView str) = 0;

protected:
	virtual void pushFont(const FontDesc& fontDesc) {(void)fontDesc;}
	virtual void popFont() {}
	virtual void beginSpoiler(StringView label) {(void)label;}
	virtual void endSpoiler() {}

	size_t mSpoilerNesting;
	Array<FontDesc> mFontStack;
};


class APlainTextFormattedWriter: public AFormattedWriter
{
public:
	void BeginCode() override
	{
		LineBreak();
		PrintRaw("___________________");
		LineBreak();
	}

	void EndCode() override
	{
		LineBreak();
		PrintRaw("___________________");
		LineBreak();
	}

	void PrintPreformatted(StringView str) {PrintRaw(str);}

protected:
	void beginSpoiler(StringView show) override
	{
		LineBreak();
		PrintRaw(show);
		LineBreak();
		PrintRaw("{");
		LineBreak();
	}

	void endSpoiler() override
	{
		LineBreak();
		PrintRaw("}");
		LineBreak();
	}
};

INTRA_WARNING_POP

}}
