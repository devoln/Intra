#pragma once

#include "Platform/CppWarnings.h"
#include "Math/Vector.h"
#include "Container/Sequential/String.h"
#include "Range/Generators/StringView.h"
#include "Container/Sequential/Array.h"
#include "Range/Polymorphic/OutputRange.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace IO {

struct FontDesc
{
	//! Цвет шрифта. Отрицательное значение означает, что нужно использовать цвет по умолчанию.
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
		return Color==rhs.Color && Size==rhs.Size &&
			Bold==rhs.Bold && Italic==rhs.Italic &&
			Underline==rhs.Underline && Strike==rhs.Strike;
	}

	bool operator!=(const FontDesc& rhs) const {return !operator==(rhs);}
};

class FormattedWriter: public OutputStream
{
public:
	struct Interface
	{
		virtual ~Interface() {}

		virtual void BeginCode(OutputStream& s)
		{
			LineBreak(s);
			s.Print("___________________");
			LineBreak(s);
		}

		virtual void EndCode(OutputStream& s)
		{
			LineBreak(s);
			s.Print("___________________");
			LineBreak(s);
		}

		virtual void PushStyle(OutputStream& s, StringView style) {(void)s; (void)style;}
		virtual void PopStyle(OutputStream& s) {(void)s;}

		virtual void HorLine(OutputStream& s)
		{
			s.Print("__________________________________________________");
			LineBreak(s);
		}

		virtual void LineBreak(OutputStream& s)
		{
			s.Put('\r');
			s.Put('\n');
		}

		virtual void PrintPreformatted(OutputStream& s, StringView text) {s.Print(text);}

		virtual void PushFont(OutputStream& s, const FontDesc& newFont, const FontDesc& curFont) {(void)s; (void)newFont; (void)curFont;}
		virtual void PopFont(OutputStream& s, const FontDesc& curFont, const FontDesc& prevFont) {(void)s; (void)curFont; (void)prevFont;}
		
		virtual void BeginSpoiler(OutputStream& s, StringView label)
		{
			LineBreak(s);
			s.Print(label);
			LineBreak(s);
			s.Print("{");
			LineBreak(s);
		}

		virtual void EndSpoiler(OutputStream& s) {(void)s;}
	};
	Memory::UniqueRef<Interface> mInterface;
	size_t mSpoilerNesting;
	Array<FontDesc> mFontStack;
public:
	FormattedWriter(null_t=null):
		OutputStream(null), mInterface(null), mSpoilerNesting(0) {}

	explicit FormattedWriter(OutputStream stream):
		OutputStream(Meta::Move(stream)),
		mInterface(new Interface), mSpoilerNesting(0) {}

	FormattedWriter(OutputStream stream, Interface* impl):
		OutputStream(Meta::Move(stream)),
		mInterface(impl), mSpoilerNesting(0) {}

	FormattedWriter(FormattedWriter&& rhs):
		OutputStream(Meta::Move(rhs)),
		mInterface(Meta::Move(rhs.mInterface)),
		mSpoilerNesting(rhs.mSpoilerNesting), mFontStack(Meta::Move(rhs.mFontStack))
	{rhs.mSpoilerNesting = 0;}

	FormattedWriter& operator=(FormattedWriter&& rhs)
	{
		if(this == &rhs) return *this;
		EndAllSpoilers();
		OutputStream::operator=(Meta::Move(rhs));
		mInterface = Meta::Move(rhs.mInterface);
		mSpoilerNesting = rhs.mSpoilerNesting;
		mFontStack = Meta::Move(rhs.mFontStack);
		rhs.mSpoilerNesting = 0;
		return *this;
	}
	
	~FormattedWriter() {operator=(null);}

	FormattedWriter& operator=(null_t)
	{
		EndAllSpoilers();
		while(!mFontStack.Empty()) PopFont();
		OutputStream::operator=(null);
		return *this;
	}

	forceinline bool operator==(null_t) const
	{return Empty() || mInterface==null;}

	forceinline bool operator!=(null_t) const
	{return !operator==(null);}

	void PushFont(Math::Vec3 color, float size=3,
		bool bold=false, bool italic=false, bool underline=false, bool strike=false)
	{PushFont({color, size, bold, italic, underline, strike});}

	void PushFont(const FontDesc& newFont)
	{
		mInterface->PushFont(*this, newFont, GetCurrentFont());
		mFontStack.AddLast(newFont);
	}

	void PushFont() {PushFont(FontDesc::Default());}

	void PopFont()
	{
		mInterface->PopFont(*this, GetCurrentFont(), GetPrevFont());
		mFontStack.RemoveLast();
	}

	void BeginSpoiler(StringView label)
	{
		mInterface->BeginSpoiler(*this, label);
		mSpoilerNesting++;
	}

	void BeginSpoiler() {BeginSpoiler("Show");}

	void EndSpoiler()
	{
		INTRA_DEBUG_ASSERT(mSpoilerNesting != 0);
		if(mSpoilerNesting == 0) return;
		mSpoilerNesting--;
		mInterface->EndSpoiler(*this);
	}

	void EndAllSpoilers()
	{
		while(mSpoilerNesting --> 0)
			EndSpoiler();
	}

	void BeginCode() {mInterface->BeginCode(*this);}
	void EndCode() {mInterface->EndCode(*this);}

	void PushStyle(StringView style) {mInterface->PushStyle(*this, style);}
	void PopStyle() {mInterface->PopStyle(*this);}

	void HorLine() {mInterface->HorLine(*this);}

	void LineBreak(size_t count=1) {while(count --> 0) mInterface->LineBreak(*this);}

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

	void PrintCode(StringView code)
	{
		BeginCode();
		PrintPreformatted(code);
		EndCode();
	}

	void PrintPreformatted(StringView str) {mInterface->PrintPreformatted(*this, str);}

private:
	FormattedWriter(const FormattedWriter&) = delete;
	FormattedWriter& operator=(const FormattedWriter&) = delete;
};

}}

INTRA_WARNING_POP
