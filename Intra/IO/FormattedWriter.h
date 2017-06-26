#pragma once

#include "Cpp/Warnings.h"
#include "Math/Vector3.h"
#include "Container/Sequential/String.h"
#include "Utils/StringView.h"
#include "Container/Sequential/Array.h"
#include "Range/Polymorphic/OutputRange.h"
#include "Utils/Unique.h"

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
	class Interface
	{
	public:
		virtual ~Interface() {}

		virtual void CleanUp(OutputStream& s) = 0;
		virtual void BeginCode(OutputStream& s) = 0;
		virtual void EndCode(OutputStream& s) = 0;
		virtual void PushStyle(OutputStream& s, StringView style) = 0;
		virtual void PopStyle(OutputStream& s) = 0;
		virtual void HorLine(OutputStream& s) = 0;
		virtual void LineBreak(OutputStream& s) = 0;
		virtual void PrintPreformatted(OutputStream& s, StringView text) = 0;
		virtual void BeginSpoiler(OutputStream& s, StringView label) = 0;
		virtual void EndSpoiler(OutputStream& s) = 0;

		virtual void PushFont(OutputStream& s, const FontDesc& newFont) = 0;
		virtual void PopFont(OutputStream& s) = 0;
	};

	class BasicImpl: public Interface
	{
	public:
		void CleanUp(OutputStream& s) override
		{
			while(mSpoilerNesting != 0)
				EndSpoiler(s);
			while(!mFontStack.Empty()) PopFont(s);
		}

		void BeginCode(OutputStream& s) override
		{
			LineBreak(s);
			s.Print("___________________");
			LineBreak(s);
		}

		void EndCode(OutputStream& s) override
		{
			LineBreak(s);
			s.Print("___________________");
			LineBreak(s);
		}

		void PushStyle(OutputStream& s, StringView style) override {(void)s; (void)style;}
		void PopStyle(OutputStream& s) override {(void)s;}

		void HorLine(OutputStream& s) override
		{
			s.Print("__________________________________________________");
			LineBreak(s);
		}

		void LineBreak(OutputStream& s) override
		{
			s.Put('\r');
			s.Put('\n');
		}

		void PrintPreformatted(OutputStream& s, StringView text) override {s.Print(text);}

		virtual void PushFont(OutputStream& s, const FontDesc& newFont, const FontDesc& curFont)
		{(void)s; (void)newFont; (void)curFont;}

		virtual void PopFont(OutputStream& s, const FontDesc& curFont, const FontDesc& prevFont)
		{(void)s; (void)curFont; (void)prevFont;}
		
		void PushFont(OutputStream& s, const FontDesc& newFont) override
		{
			PushFont(s, newFont, GetCurrentFont());
			mFontStack.AddLast(newFont);
		}

		void PopFont(OutputStream& s) override
		{
			PopFont(s, GetCurrentFont(), GetPrevFont());
			mFontStack.RemoveLast();
		}

		void BeginSpoiler(OutputStream& s, StringView label) final
		{
			beginSpoiler(s, label);
			mSpoilerNesting++;
		}

		void EndSpoiler(OutputStream& s) final
		{
			INTRA_DEBUG_ASSERT(mSpoilerNesting != 0);
			if(mSpoilerNesting == 0) return;
			endSpoiler(s);
			mSpoilerNesting--;
		}

		virtual void beginSpoiler(OutputStream& s, StringView label)
		{
			LineBreak(s);
			s.Print(label);
			LineBreak(s);
			s.Print("{");
			LineBreak(s);
		}

		virtual void endSpoiler(OutputStream& s)
		{
			s.Print("}");
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

	Unique<Interface> mInterface;
	size_t mRefCount = 0;
public:
	FormattedWriter(null_t=null):
		OutputStream(null), mInterface(null) {}

	explicit FormattedWriter(OutputStream stream):
		OutputStream(Cpp::Move(stream)),
		mInterface(new BasicImpl) {}

	FormattedWriter(OutputStream stream, Interface* impl):
		OutputStream(Cpp::Move(stream)),
		mInterface(Cpp::Move(impl)) {}

	FormattedWriter(FormattedWriter&& rhs):
		OutputStream(Cpp::Move(rhs)),
		mInterface(Cpp::Move(rhs.mInterface))
	{}

	~FormattedWriter()
	{
		INTRA_DEBUG_ASSERT(mRefCount == 0);
		operator=(null);
	}

	FormattedWriter& operator=(FormattedWriter&& rhs)
	{
		if(this == &rhs) return *this;
		OutputStream::operator=(Cpp::Move(rhs));
		if(mInterface!=null) mInterface->CleanUp(*this);
		mInterface = Cpp::Move(rhs.mInterface);
		return *this;
	}

	FormattedWriter& operator=(null_t)
	{
		if(mInterface != null) mInterface->CleanUp(*this);
		OutputStream::operator=(null);
		mInterface = null;
		return *this;
	}

	forceinline bool operator==(null_t) const
	{return Full() || mInterface==null;}

	forceinline bool operator!=(null_t) const
	{return !operator==(null);}

	FormattedWriter& PushFont(Math::Vec3 color, float size=3,
		bool bold=false, bool italic=false, bool underline=false, bool strike=false)
	{return PushFont({color, size, bold, italic, underline, strike});}

	FormattedWriter& PushFont(const FontDesc& newFont)
	{
		mInterface->PushFont(*this, newFont);
		return *this;
	}

	FormattedWriter& PopFont()
	{
		mInterface->PopFont(*this);
		return *this;
	}

	FormattedWriter& BeginSpoiler(StringView label)
	{
		mInterface->BeginSpoiler(*this, label);
		return *this;
	}

	FormattedWriter& BeginSpoiler() {return BeginSpoiler("Show");}

	FormattedWriter& EndSpoiler()
	{
		mInterface->EndSpoiler(*this);
		return *this;
	}

	FormattedWriter& BeginCode()
	{
		mInterface->BeginCode(*this);
		return *this;
	}

	FormattedWriter& EndCode()
	{
		mInterface->EndCode(*this);
		return *this;
	}

	FormattedWriter& PushStyle(StringView style)
	{
		mInterface->PushStyle(*this, style);
		return *this;
	}

	FormattedWriter& PopStyle()
	{
		mInterface->PopStyle(*this);
		return *this;
	}

	FormattedWriter& HorLine()
	{
		mInterface->HorLine(*this);
		return *this;
	}

	FormattedWriter& LineBreak(size_t count=1)
	{
		while(count --> 0) mInterface->LineBreak(*this);
		return *this;
	}

	template<typename Arg0, typename... Args>
	FormattedWriter& Print(Arg0&& arg0, Args&&... args)
	{
		OutputStream::Print(Cpp::Forward<Arg0>(arg0), Cpp::Forward<Args>(args)...);
		return *this;
	}

	template<typename Arg0, typename... Args>
	FormattedWriter& PrintLine(Arg0&& arg0, Args&&... args)
	{
		OutputStream::Print(Cpp::Forward<Arg0>(arg0), Cpp::Forward<Args>(args)...);
		LineBreak();
		return *this;
	}

	FormattedWriter& PrintCode(StringView code)
	{
		BeginCode();
		PrintPreformatted(code);
		EndCode();
		return *this;
	}

	FormattedWriter& PrintPreformatted(StringView str)
	{
		mInterface->PrintPreformatted(*this, str);
		return *this;
	}

private:
	FormattedWriter(const FormattedWriter&) = delete;
	FormattedWriter& operator=(const FormattedWriter&) = delete;
};

}}

INTRA_WARNING_POP
