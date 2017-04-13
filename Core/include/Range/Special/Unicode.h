#pragma once

#include "Platform/CppWarnings.h"
#include "Platform/CppFeatures.h"
#include "Range/Generators/Span.h"
#include "Range/Generators/StringView.h"
#include "Container/ForwardDecls.h"

namespace Intra {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

struct UTF8
{
	StringView Text;

	UTF8(StringView str=null): Text(str) {}

	DString ToUTF32(bool addNullTerminator) const;
	WString ToUTF16(bool addNullTerminator) const;

	static constexpr size_t SequenceBytes(byte i) {return size_t(1+(i>=192)+(i>=224)+(i>=240)+(i>=248)+(i>=252));}
	static constexpr size_t SequenceBytes(char i) {return SequenceBytes(byte(i));}
	static bool SequenceIsLegal(const char* start, const char* end);

	dchar ReadChar()
	{
		size_t read;
		auto res = NextChar(&read);
		Text.PopFirstExactly(read);
		return res;
	}

	dchar NextChar(size_t* bytesRead=null) const;
	forceinline dchar First() const {return NextChar();}
	void PopFirst();

	dchar ReadPrevChar();
	dchar Last() const;
	void PopLast() {popBackChar();}

	bool Empty() const {return Text.Empty();}

	bool operator==(const UTF8& rhs) const {return Text==rhs.Text;}

	static const char BOM[];

private:
	bool popBackChar();
};

struct UTF16
{
	WStringView Text;

	UTF16(WStringView str=null): Text(str) {}

	DString ToUTF32() const;
	String ToUTF8() const;

	dchar ReadChar()
	{
		size_t read;
		auto res = NextChar(&read);
		Text.PopFirstExactly(read);
		return res;
	}

	dchar NextChar(size_t* wcharsRead=null) const;
	forceinline dchar First() const {return NextChar();}
	void PopFirst();

	dchar ReadPrevChar();
	dchar Last() const;
	void PopLast() {popBackChar();}

	bool Empty() const {return Text.Empty();}

	bool operator==(const UTF16& rhs) const {return Text==rhs.Text;}

	enum: wchar_t {ReplacementChar=0xFFFD};

private:
	bool popBackChar();
};


struct UTF32: public DStringView
{
	UTF32(DStringView range=null): DStringView(range) {}

	WString ToUTF16(bool addNullTerminator) const;
	String ToUTF8() const;

	enum: uint {ReplacementChar=0xFFFD, MaxLegalChar=0x10FFFF};
	static bool CharToUTF16Pair(dchar c, wchar* first, wchar* second);
	static size_t CharToUTF8Sequence(dchar c, char dst[5]);
};

INTRA_WARNING_POP

}

