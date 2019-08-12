#pragma once

#include "Core/Range/Span.h"
#include "Core/Range/StringView.h"
#include "Container/ForwardDecls.h"

INTRA_CORE_RANGE_BEGIN
// TODO: turn UTF8, UTF16 and UTF32 into template classes RUtf8ToUtf16<R>, RUtf8ToUtf32<R>, ..., taking underlying range as a parameter

struct UTF8
{
private:
	enum: ushort {SurrogateHighStart = 0xD800, SurrogateLowEnd = 0xDFFF};
	enum: uint32 {ReplacementCharCode = 0xFFFD, MaxLegalCharCode = 0x10FFFF};

	static INTRA_CONSTEXPR2 bool isLegalUTF8(const char* source, index_t length, index_t& oBytesRead)
	{
		oBytesRead = length;
		byte a = 0;
		switch(length)
		{
		default: return false;
			// Everything else falls through when "true"...
		case 4: a = byte(source[--oBytesRead]); if(a < 0x80 || a > 0xBF) return false;
		case 3: a = byte(source[--oBytesRead]); if(a < 0x80 || a > 0xBF) return false;
		case 2: a = byte(source[--oBytesRead]); if(a > 0xBF) return false;
			switch(byte(source[0]))
			{
				//no fall-through in this inner switch
			case 0xE0: if(a < 0xA0) return false; break;
			case 0xED: if(a > 0x9F) return false; break;
			case 0xF0: if(a < 0x90) return false; break;
			case 0xF4: if(a > 0x8F) return false; break;
			default:   if(a < 0x80) return false;
			}
		case 1: if(byte(source[0]) >= 0x80 && byte(source[0]) < 0xC2) return false;
		}
		oBytesRead = length;
		return (byte(*source) <= 0xF4);
	}
	static INTRA_NODISCARD constexpr index_t sequenceBytes(byte i) {return index_t(1 + (i >= 192) + (i >= 224) + (i >= 240) + (i >= 248) + (i >= 252));}
public:
	StringView Text;

	UTF8() = default;
	constexpr forceinline UTF8(StringView str): Text(str) {}

	DString ToUTF32(bool addNullTerminator) const;
	WString ToUTF16(bool addNullTerminator) const;

	static INTRA_NODISCARD constexpr forceinline index_t SequenceBytes(char i) {return sequenceBytes(byte(i));}

	static INTRA_NODISCARD INTRA_CONSTEXPR2 bool SequenceIsLegal(const char* start, const char* end)
	{
		const index_t length = UTF8::SequenceBytes(*start);
		index_t unused = 0;
		return start + length <= end &&
			isLegalUTF8(start, length, unused);
	}

	INTRA_CONSTEXPR2 char32_t ReadChar()
	{
		index_t read = 0;
		auto res = NextChar(OptRef(read));
		Text.PopFirstExactly(read);
		return res;
	}

	INTRA_CONSTEXPR2 char32_t NextChar(Optional<index_t&> oBytesRead = null) const
	{
		auto src = Text;
		uint32 ch = 0;
		const index_t bytesToRead = UTF8::SequenceBytes(src.First());

		if(bytesToRead > src.Length())
		{
			if(oBytesRead) oBytesRead.Unwrap() = src.Length();
			return char32_t(ReplacementCharCode);
		}

		index_t bytesReadIgnore = 0;
		if(!oBytesRead) oBytesRead = OptRef(bytesReadIgnore);
		if(!isLegalUTF8(src.Data(), bytesToRead, oBytesRead.Unwrap()))
			return char32_t(ReplacementCharCode);

		constexpr const uint32 offsetsFromUTF8[6] = {0, 0x00003080, 0x000E2080, 0x03C82080, 0xFA082080, 0x82082080};
		for(index_t i = bytesToRead; i > 1; i--)
		{
			ch += byte(src.Next());
			ch <<= 6;
		}
		ch += byte(src.Next()) - offsetsFromUTF8[bytesToRead - 1];

		if(ch > MaxLegalCharCode || (ch >= SurrogateHighStart && ch <= SurrogateLowEnd))
			ch = char32_t(ReplacementCharCode);

		oBytesRead.Unwrap() = bytesToRead;
		return ch;
	}

	INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline char32_t First() const {return NextChar();}

	INTRA_CONSTEXPR2 void PopFirst()
	{
		const index_t bytesToRead = UTF8::SequenceBytes(Text.First());
		index_t bytesRead = 0;
		if(bytesToRead > Text.Length()) {Text = null; return;}
		if(!isLegalUTF8(Text.Data(), bytesToRead, bytesRead))
		{
			Text.PopFirstExactly(bytesRead);
			return;
		}
		//ushort bytesToRead; NextChar(OptRef(bytesToRead));
		Text.PopFirstExactly(bytesToRead);
	}

	INTRA_CONSTEXPR2 char32_t ReadPrevChar()
	{
		const auto oldText = Text;
		if(!popBackChar()) return char32_t(ReplacementCharCode);
		return UTF8(oldText.Tail(oldText.Length() - Text.Length())).NextChar();
	}

	INTRA_NODISCARD INTRA_CONSTEXPR2 char32_t Last() const
	{
		auto temp = *this;
		if(!temp.popBackChar()) return char32_t(ReplacementCharCode);
		return UTF8({temp.Text.end(), Text.end()}).NextChar();
	}

	INTRA_CONSTEXPR2 void PopLast() {popBackChar();}

	INTRA_NODISCARD constexpr forceinline bool Empty() const {return Text.Empty();}

	INTRA_NODISCARD constexpr forceinline bool operator==(const UTF8& rhs) const {return Text == rhs.Text;}

	static constexpr const char BOM[] = "\xef\xbb\xbf";

private:
	INTRA_CONSTEXPR2 bool popBackChar()
	{
		INTRA_PRECONDITION(!Empty());
		while(((Text.Last() & 0xFF) >> 6) == 0x2)
		{
			Text.PopLast();
			if(Text.Empty()) return false;
		}
		return true;
	}
};

struct UTF16
{
private:
	enum: ushort {HalfShift = 10};
	enum: uint32 {HalfBase = 0x0010000, ReplacementCharCode = 0xFFFD};
	enum: ushort {SurrogateHighStart = 0xD800, SurrogateHighEnd = 0xDBFF, SurrogateLowStart = 0xDC00, SurrogateLowEnd = 0xDFFF};
public:
	GenericStringView<const char16_t> Text;

	UTF16() = default;
	constexpr forceinline UTF16(GenericStringView<const char16_t> str): Text(str) {}

	DString ToUTF32() const;
	String ToUTF8() const;

	INTRA_NODISCARD INTRA_CONSTEXPR2 char32_t ReadChar()
	{
		index_t read = 0;
		auto res = NextChar(OptRef(read));
		Text.PopFirstExactly(read);
		return res;
	}

	INTRA_NODISCARD INTRA_CONSTEXPR2 char32_t NextChar(Optional<index_t&> oWcharsRead = null) const
	{
		INTRA_PRECONDITION(!Empty());
		auto src = Text;
		char16_t ch = src.First();
		src.PopFirst();

		if(oWcharsRead) oWcharsRead.Unwrap() = 1;
		if(ch < SurrogateHighStart || ch > SurrogateHighEnd) return ch;
		if(src.Empty()) return char32_t(ReplacementCharCode);

		if(oWcharsRead) oWcharsRead.Unwrap() = 2;
		if(src.First() < SurrogateLowStart || src.First() > SurrogateLowEnd) return ch;
		return (uint(ch - SurrogateHighStart) << HalfShift) + uint(src.First() - SurrogateLowStart) + HalfBase;
	}

	INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline char32_t First() const {return NextChar();}

	INTRA_CONSTEXPR2 void PopFirst()
	{
		INTRA_PRECONDITION(!Empty());
		bool twoElements = (Text.First() >= SurrogateHighStart && Text.First() <= SurrogateHighEnd);
		Text.PopFirst();
		if(twoElements && !Text.Empty()) Text.PopFirst();
	}

	INTRA_NODISCARD INTRA_CONSTEXPR2 char32_t ReadPrevChar()
	{
		if(!popBackChar()) return char32_t(ReplacementCharCode);
		return NextChar();
	}

	INTRA_NODISCARD INTRA_CONSTEXPR2 char32_t Last() const
	{
		auto temp = *this;
		if(!temp.popBackChar()) return char32_t(ReplacementCharCode);
		return UTF16({temp.Text.end(), Text.end()}).ReadChar();
	}

	INTRA_CONSTEXPR2 void PopLast() {popBackChar();}

	constexpr forceinline bool Empty() const {return Text.Empty();}

	constexpr forceinline bool operator==(const UTF16& rhs) const {return Text == rhs.Text;}

private:
	INTRA_CONSTEXPR2 bool popBackChar()
	{
		INTRA_PRECONDITION(!Empty());
		const bool twoElements = (Text.Last() >= SurrogateHighStart && Text.Last() <= SurrogateHighEnd);
		Text.PopLast();
		if(twoElements)
		{
			if(Text.Empty()) return false;
			Text.PopLast();
		}
		return true;
	}
};


struct UTF32: public GenericStringView<const char32_t>
{
private:
	enum: ushort {HalfShift = 10};
	enum: uint {HalfBase = 0x0010000};
	enum: ushort {SurrogateHighStart = 0xD800, SurrogateLowStart = 0xDC00, SurrogateLowEnd = 0xDFFF};
public:
	UTF32() = default;
	constexpr forceinline UTF32(GenericStringView<const char32_t> range): GenericStringView<const char32_t>(range) {}

	WString ToUTF16(bool addNullTerminator) const;
	String ToUTF8() const;

	enum: uint32 {ReplacementCharCode = 0xFFFD, MaxLegalCharCode = 0x10FFFF};
	static INTRA_CONSTEXPR2 bool CharToUTF16Pair(char32_t code, char16_t* first, char16_t* second)
	{
		if(code <= 0xFFFF)
		{
			if(code >= SurrogateHighStart && code <= SurrogateLowEnd)
				*first = char16_t(ReplacementCharCode);
			else
				*first = char16_t(code);
			return false;
		}

		if(code > MaxLegalCharCode)
		{
			*first = char16_t(ReplacementCharCode);
			return false;
		}

		enum {halfMask = 0x3FF};
		uint ch = code - HalfBase;
		*first = char16_t((ch >> HalfShift) + SurrogateHighStart);
		*second = char16_t((ch & halfMask) + SurrogateLowStart);
		return true;
	}

	static INTRA_CONSTEXPR2 index_t CharToUTF8Sequence(char32_t code, char dst[5])
	{
		index_t bytesToWrite = 0;
		if(code < 0x80) bytesToWrite = 1;
		else if(code < 0x800) bytesToWrite = 2;
		else if(code < 0x10000) bytesToWrite = 3;
		else if(code <= MaxLegalCharCode) bytesToWrite = 4;
		else bytesToWrite = 3, code = char32_t(ReplacementCharCode);

		enum {byteMask = 0xBF, byteMark = 0x80};
		constexpr const char firstByteMark[] = "\0\0\xC0\xE0\xF0\xF8\xFC";
		dst[bytesToWrite] = 0;
		char* ptr = dst + bytesToWrite;
		for(index_t i = bytesToWrite; i > 1; i--)
		{
			*--ptr = char(byte((code | byteMark) & byteMask));
			code >>= 6;
		}
		*--ptr = char(byte(code | firstByteMark[bytesToWrite]));
		return bytesToWrite;
	}
};
INTRA_CORE_RANGE_END
