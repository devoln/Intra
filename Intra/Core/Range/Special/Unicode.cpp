#include "Core/Range/Special/Unicode.h"

#include "Core/Range/Mutation/Copy.h"
#include "Container/Sequential/Array.h"
#include "Container/Sequential/String.h"

INTRA_CORE_RANGE_BEGIN
// TODO: replace this functions with lazy template ranges

DString UTF8::ToUTF32(bool addNullTerminator) const
{
    DString result;
	result.SetLengthUninitialized(Text.Length() + +addNullTerminator);
	CopyTo(*this, result.AsRange());
	if(addNullTerminator) result += char32_t(0);
    return result;
}

WString UTF8::ToUTF16(bool addNullTerminator) const
{
	WString result;
	result.Reserve(Text.Length());
    for(UTF8 copy = *this; !copy.Empty(); copy.PopFirst())
    {
		char16_t codes[2];
		if(UTF32::CharToUTF16Pair(copy.First(), codes, codes+1))
			result += codes[0], result += codes[1];
		else result += codes[0];
    }
	if(addNullTerminator) result += char16_t('\0');
    return result;
}

DString UTF16::ToUTF32() const
{
	DString result;
	result.SetLengthUninitialized(Text.Length());
	CopyTo(*this, result.AsRange());
	return result;
}

String UTF16::ToUTF8() const
{
    String result;
	result.Reserve( Text.Length()*2 );
	
    for(UTF16 copy = *this; !copy.Empty(); copy.PopFirst())
    {
		char temp[5];
		UTF32::CharToUTF8Sequence(copy.First(), temp);
		result += StringView(temp, C::strlen(temp));
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
		result += StringView(temp, C::strlen(temp));
    }
    return result;
}

WString UTF32::ToUTF16(bool addNullTerminator) const
{
	WString result;
	result.Reserve(Length());
    for(auto ch: *this)
    {
    	char16_t first, second;
		if(CharToUTF16Pair(ch, &first, &second))
		{
			result += first;
			result += second;
		}
		else result += first;
    }
	if(addNullTerminator) result += char16_t('\0');
    return result;
}
INTRA_CORE_RANGE_END
