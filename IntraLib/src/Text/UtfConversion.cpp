#include "Text/UtfConversion.h"
#include "Containers/Array.h"
#include "Containers/String.h"
#include "Test/UnitTest.h"

namespace Intra {

enum: ushort {HalfShift=10};
enum: uint {HalfBase=0x0010000};
enum: ushort {SurrogateHighStart=0xD800, SurrogateHighEnd=0xDBFF, SurrogateLowStart=0xDC00, SurrogateLowEnd=0xDFFF};

const char UTF8::BOM[] = "\xef\xbb\xbf";

static bool isLegalUTF8(const char* source, size_t length, size_t* bytesRead)
{
    byte a;
    const byte* srcptr = (const byte*)source+length;
	if(bytesRead!=null) *bytesRead = length;
    switch(length)
    {
    default: return false;
	// Everything else falls through when "true"...
	case 4: if((a=(*--srcptr))<0x80 || a>0xBF) goto illegal_char;
	case 3: if((a=(*--srcptr))<0x80 || a>0xBF) goto illegal_char;
	case 2: if((a=(*--srcptr))>0xBF) goto illegal_char;
		switch((byte)*source)
		{
		//no fall-through in this inner switch
		case 0xE0: if(a<0xA0) goto illegal_char; break;
		case 0xED: if(a>0x9F) goto illegal_char; break;
		case 0xF0: if(a<0x90) goto illegal_char; break;
		case 0xF4: if(a>0x8F) goto illegal_char; break;
		default:   if(a<0x80) goto illegal_char;
		}
	case 1: if((byte)*source>=0x80 && (byte)*source<0xC2) goto illegal_char;
    }
    return ((byte)*source<=0xF4);
illegal_char:
	if(bytesRead!=null) *bytesRead = size_t((const char*)srcptr-source);
	return false;
}

dchar UTF8::NextChar(size_t* bytesRead) const
{
	const byte* source = (const byte*)Text.Begin;
	uint ch=0;
	size_t bytesToRead = UTF8::SequenceBytes(*source);

	if((const char*)source+bytesToRead>Text.End)
	{
		if(bytesRead!=null)
			*bytesRead = size_t(Text.End-(const char*)source);
		return UTF32::ReplacementChar;
	}

	if(!isLegalUTF8((const char*)source, bytesToRead, bytesRead))
		return UTF32::ReplacementChar;

	static const uint offsetsFromUTF8[6] = {0, 0x00003080, 0x000E2080, 0x03C82080, 0xFA082080, 0x82082080};
	for(size_t i=bytesToRead; i>1; i--)
	{
		ch += *source++;
		ch <<= 6;
	}
	ch += (*source++) - offsetsFromUTF8[bytesToRead-1];

	if( ch>UTF32::MaxLegalChar || (ch>=SurrogateHighStart && ch<=SurrogateLowEnd) )
		ch = UTF32::ReplacementChar;

	if(bytesRead!=null) *bytesRead = bytesToRead;
	return ch;
}

dchar UTF8::ReadPrevChar()
{
	auto oldEnd = Text.End;
	if(!popBackChar()) return UTF32::ReplacementChar;
	return UTF8(Text.End, oldEnd).NextChar();
}

dchar UTF8::Last() const
{
	auto temp = *this;
	if(!temp.popBackChar()) return UTF32::ReplacementChar;
	return UTF8(temp.Text.End, Text.End).NextChar();
}

void UTF8::PopFirst()
{
	size_t bytesToRead = UTF8::SequenceBytes(*Text.Begin);
	size_t bytesRead = 0;
	if(Text.Begin+bytesToRead>Text.End) {Text.Begin = Text.End; return;}
	if(!isLegalUTF8(Text.Begin, bytesToRead, &bytesRead))
	{
		Text.Begin += bytesRead;
		return;
	}
	//ushort bytesToRead; NextChar(&bytesToRead);
	Text.Begin += bytesToRead;
}

bool UTF8::popBackChar()
{
	INTRA_ASSERT(!Empty());
	while((((*--Text.End)&0xFF) >> 6)==0x2)
		if(Text.End==Text.Begin) return false;
	return true;
}


DString UTF8::ToUTF32(bool addNullTerminator) const
{
    DString result;
	result.Reserve(Text.Length());
    for(auto ch: *this) result += ch;
	if(addNullTerminator) result += dchar(0);
    return result;
}

WString UTF8::ToUTF16(bool addNullTerminator) const
{
	WString result;
	result.Reserve(Text.Length());
    for(auto ch: *this)
    {
		wchar codes[2];
		if(UTF32::CharToUTF16Pair(ch, codes, codes+1)) result += WStringView(codes, 2);
		else result += codes[0];
    }
	if(addNullTerminator) result += wchar('\0');
    return result;
}


dchar UTF16::NextChar(size_t* wcharsRead) const
{
	INTRA_ASSERT(!Empty());
	const wchar* source = Text.Begin;
	wchar ch = *source++;

	if(wcharsRead!=null) *wcharsRead = 1;
	if(ch<SurrogateHighStart || ch>SurrogateHighEnd) return ch;
	if(source>=Text.End) return UTF32::ReplacementChar;

	if(wcharsRead!=null) *wcharsRead = 2;
	if(*source<SurrogateLowStart || *source>SurrogateLowEnd) return ch;
	return ((ch-SurrogateHighStart) << HalfShift) + (*source-SurrogateLowStart)+HalfBase;
}

void UTF16::PopFirst()
{
	INTRA_ASSERT(!Empty());
	Text.Begin += 1+(*Text.Begin>=SurrogateHighStart && *Text.Begin<=SurrogateHighEnd);
	if(Text.Begin>Text.End) Text.Begin=Text.End;
}

bool UTF16::popBackChar()
{
	INTRA_ASSERT(!Empty());
	Text.End -= 1+(*(Text.End-1)>=SurrogateHighStart && *(Text.End-1)<=SurrogateHighEnd); //Символ состоит из двух слов
	if(Text.End<Text.Begin)
	{
		Text.End=Text.Begin;
		return false;
	}
	return true;
}

dchar UTF16::ReadPrevChar()
{
	if(!popBackChar()) return UTF32::ReplacementChar;
	return NextChar();
}

dchar UTF16::Last() const
{
	auto temp = *this;
	if(!temp.popBackChar()) return UTF32::ReplacementChar;
	return UTF16(temp.Text.End, Text.End).ReadChar();
}


DString UTF16::ToUTF32() const
{
	DString result;
	result.Reserve(Text.Length());
	for(auto ch: *this) result += ch;
	return result;
}

bool UTF32::CharToUTF16Pair(dchar code, wchar* first, wchar* second)
{
	if(code<=0xFFFF)
	{
		if(code>=SurrogateHighStart && code<=SurrogateLowEnd)
			*first = UTF16::ReplacementChar;
		else
			*first = (wchar)code;
		return false;
	}
	
	if(code>UTF32::MaxLegalChar)
	{
		*first = UTF16::ReplacementChar;
		return false;
	}

	enum {halfMask=0x3FF};
	uint ch=code-HalfBase;
	*first=(wchar)((ch >> HalfShift)+SurrogateHighStart);
	*second=(wchar)((ch & halfMask)+SurrogateLowStart);
	return true;
}

size_t UTF32::CharToUTF8Sequence(dchar code, char dst[5])
{
	size_t bytesToWrite;
	if(code<0x80) bytesToWrite=1;
	else if(code<0x800) bytesToWrite=2;
	else if(code<0x10000) bytesToWrite=3;
	else if(code<=UTF32::MaxLegalChar) bytesToWrite=4;
	else bytesToWrite=3, code = UTF32::ReplacementChar;

	enum {byteMask=0xBF, byteMark=0x80};
	static const byte firstByteMark[7]={0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC};
	dst[bytesToWrite]=0;
	byte* ptr = (byte*)dst+bytesToWrite;
	for(size_t i=bytesToWrite; i>1; i--)
	{
		*--ptr = (byte)((code | byteMark) & byteMask);
		code >>= 6;
	}
	*--ptr = (byte)(code | firstByteMark[bytesToWrite]);
	return bytesToWrite;
}

String UTF16::ToUTF8() const
{
    String result;
	result.Reserve( Text.Length()*2 );
    for(auto ch: *this)
    {
		char temp[5];
		UTF32::CharToUTF8Sequence(ch, temp);
		result += StringView((const char*)temp);
    }
    return result;
}

String UTF32::ToUTF8() const
{
    String result;
	result.Reserve( Length()*2 );
    for(auto ch: *this)
    {
		char temp[5];
		CharToUTF8Sequence(ch, temp);
		result += StringView((const char*)temp);
    }
    return result;
}

WString UTF32::ToUTF16(bool addNullTerminator) const
{
	WString result;
	result.Reserve(Length());
    for(auto ch: *this)
    {
    	wchar first, second;
		if(CharToUTF16Pair(ch, &first, &second))
		{
			result += first;
			result += second;
		}
		else result += first;
    }
	if(addNullTerminator) result += wchar('\0');
    return result;
}

bool UTF8::SequenceIsLegal(const char* source, const char* sourceEnd)
{
    const size_t length = UTF8::SequenceBytes(*source);
    return source+length<=sourceEnd &&
		isLegalUTF8(source, length, null);
}

INTRA_UNITTEST("Unicode conversion test")
{
	StringView originalStr = "Тестируется текст с иероглифами ㈇㌤㈳㌛㉨, а также греческим алфавитом αβγδεζηθικλμνξο.";
	String mystr = originalStr;
	WString utf16 = UTF8(originalStr).ToUTF16(false);
	DString utf32 = UTF16(utf16()).ToUTF32();
	utf16 = UTF32(utf32()).ToUTF16(false);
	auto utf8 = UTF16(utf16.begin(), utf16.end()).ToUTF8();
	INTRA_TEST_ASSERT_EQUALS(originalStr, utf8);
};

}
