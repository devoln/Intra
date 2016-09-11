#pragma once

#include "Containers/ForwardDeclarations.h"
#include "Algorithms/Range.h"
#include "Containers/StringView.h"
#include "Meta/Type.h"

namespace Intra {

struct UTF8: Range::RangeMixin<UTF8, dchar, Range::TypeEnum::Bidirectional, true>
{
	typedef dchar value_type;
	typedef dchar return_value_type;

	ArrayRange<const char> Text;

	constexpr UTF8(null_t=null): Text(null) {}
	UTF8(const char* start, const char* end): Text(start, end) {}
	constexpr UTF8(const char* start, size_t lengthInBytes): Text(start, lengthInBytes) {}
	constexpr UTF8(StringView str): Text(str.AsRange()) {}
	constexpr UTF8(const ArrayRange<char> str): Text(str) {}
	constexpr UTF8(const ArrayRange<const char> str): Text(str) {}

	DString ToUTF32(bool addNullTerminator) const;
	WString ToUTF16(bool addNullTerminator) const;

	static constexpr size_t SequenceBytes(byte i) {return size_t(1+(i>=192)+(i>=224)+(i>=240)+(i>=248)+(i>=252));}
	static constexpr size_t SequenceBytes(char i) {return SequenceBytes((byte)i);}
	static bool SequenceIsLegal(const char* start, const char* end);

	dchar ReadChar()
	{
		size_t read;
		auto res = NextChar(&read);
		Text.Begin += read;
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

struct UTF16: Range::RangeMixin<UTF16, dchar, Range::TypeEnum::Bidirectional, true>
{
	typedef dchar value_type;
	typedef dchar return_value_type;

	ArrayRange<const wchar> Text;

	constexpr UTF16(null_t=null) {}
	UTF16(const wchar* start, const wchar* end): Text(start, end) {}
	constexpr UTF16(const wchar* start, size_t lengthInWords): Text(start, lengthInWords) {}
	constexpr UTF16(WStringView str): Text(str.AsRange()) {}
	constexpr UTF16(const ArrayRange<wchar> str): Text(str) {}
	constexpr UTF16(const ArrayRange<const wchar> str): Text(str) {}

	DString ToUTF32() const;
	String ToUTF8() const;

	dchar ReadChar()
	{
		size_t read;
		auto res = NextChar(&read);
		Text.Begin += read;
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


struct UTF32: public ArrayRange<const dchar>
{
	typedef dchar value_type;
	typedef dchar return_value_type;

	constexpr UTF32(null_t=null) {}
	UTF32(const dchar* start, const dchar* end): ArrayRange<const dchar>(start, end) {}
	constexpr UTF32(const dchar* start, size_t lengthInChars): ArrayRange<const dchar>(start, lengthInChars) {}
	constexpr UTF32(DStringView range): ArrayRange<const dchar>(range.AsRange()) {}
	constexpr UTF32(const ArrayRange<dchar> range): ArrayRange<const dchar>(range) {}
	constexpr UTF32(const ArrayRange<const dchar> range): ArrayRange<const dchar>(range) {}

	WString ToUTF16(bool addNullTerminator) const;
	String ToUTF8() const;

	enum: uint {ReplacementChar=0xFFFD, MaxLegalChar=0x10FFFF};
	static bool CharToUTF16Pair(dchar c, wchar* first, wchar* second);
	static size_t CharToUTF8Sequence(dchar c, char dst[5]);
};

}
