#pragma once

#include "Range/Polymorphic/OutputRange.h"
#include "Utils/StringView.h"
#include "Container/Sequential/Array.h"
#include "Math/Vector3.h"

namespace Intra { namespace IO {

struct FontDesc
{
	//! ÷вет шрифта. ќтрицательное значение означает, что нужно использовать цвет по умолчанию.
	Math::Vec3 Color;
	float Size;
	bool Bold, Italic, Underline, Strike;

	static const FontDesc& Default()
	{
		static const FontDesc result = {{-1, -1, -1}, 3, false, false, false, false};
		return result;
	}

	bool operator==(const FontDesc& rhs) const
	{
		return Color == rhs.Color &&
			Size == rhs.Size &&
			Bold == rhs.Bold &&
			Italic == rhs.Italic &&
			Underline == rhs.Underline &&
			Strike == rhs.Strike;
	}

	forceinline bool operator!=(const FontDesc& rhs) const {return !operator==(rhs);}
};

class IFormatter
{
public:
	virtual ~IFormatter() {}

	virtual void CleanUp(IOutputStream& s) = 0;
	virtual void BeginCode(IOutputStream& s) = 0;
	virtual void EndCode(IOutputStream& s) = 0;
	virtual void PushStyle(IOutputStream& s, StringView style) = 0;
	virtual void PopStyle(IOutputStream& s) = 0;
	virtual void HorLine(IOutputStream& s) = 0;
	virtual void LineBreak(IOutputStream& s) = 0;
	virtual void PrintPreformatted(IOutputStream& s, StringView text) = 0;
	virtual void BeginSpoiler(IOutputStream& s, StringView label) = 0;
	virtual void EndSpoiler(IOutputStream& s) = 0;

	virtual void PushFont(IOutputStream& s, const FontDesc& newFont) = 0;
	virtual void PopFont(IOutputStream& s) = 0;
};

class BasicFormatter: public IFormatter
{
public:
	void CleanUp(IOutputStream& s) override
	{
		while(mSpoilerNesting != 0)
			EndSpoiler(s);
		while(!mFontStack.Empty()) PopFont(s);
	}

	void BeginCode(IOutputStream& s) override
	{
		LineBreak(s);
		s << "___________________";
		LineBreak(s);
	}

	void EndCode(IOutputStream& s) override
	{
		LineBreak(s);
		s << "___________________";
		LineBreak(s);
	}

	void PushStyle(IOutputStream& s, StringView style) override {(void)s; (void)style;}
	void PopStyle(IOutputStream& s) override {(void)s;}

	void HorLine(IOutputStream& s) override
	{
		s << "__________________________________________________";
		LineBreak(s);
	}

	void LineBreak(IOutputStream& s) override
	{
		s << "\r\n";
	}

	void PrintPreformatted(IOutputStream& s, StringView text) override {s << text;}

	virtual void PushFont(IOutputStream& s, const FontDesc& newFont, const FontDesc& curFont)
	{(void)s; (void)newFont; (void)curFont;}

	virtual void PopFont(IOutputStream& s, const FontDesc& curFont, const FontDesc& prevFont)
	{(void)s; (void)curFont; (void)prevFont;}

	void PushFont(IOutputStream& s, const FontDesc& newFont) override
	{
		PushFont(s, newFont, GetCurrentFont());
		mFontStack.AddLast(newFont);
	}

	void PopFont(IOutputStream& s) override
	{
		PopFont(s, GetCurrentFont(), GetPrevFont());
		mFontStack.RemoveLast();
	}

	void BeginSpoiler(IOutputStream& s, StringView label) final
	{
		beginSpoiler(s, label);
		mSpoilerNesting++;
	}

	void EndSpoiler(IOutputStream& s) final
	{
		INTRA_DEBUG_ASSERT(mSpoilerNesting != 0);
		if(mSpoilerNesting == 0) return;
		endSpoiler(s);
		mSpoilerNesting--;
	}

	virtual void beginSpoiler(IOutputStream& s, StringView label)
	{
		LineBreak(s);
		s << label;
		LineBreak(s);
		s << "{";
		LineBreak(s);
	}

	virtual void endSpoiler(IOutputStream& s)
	{
		s << "}";
		LineBreak(s);
	}

	const FontDesc& GetCurrentFont() const
	{
		if(mFontStack.Empty())
			return FontDesc::Default();
		return mFontStack.Last();
	}

	const FontDesc& GetPrevFont() const
	{
		if(mFontStack.Length()<2)
			return FontDesc::Default();
		return mFontStack[mFontStack.Length()-2];
	}

	size_t mSpoilerNesting = 0;
	Array<FontDesc> mFontStack;
};

}}
