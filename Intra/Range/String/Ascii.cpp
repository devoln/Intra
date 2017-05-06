#include "Range/String/Ascii.h"
#include "Range/Comparison/StartsWith.h"
#include "Range/Search/Single.h"
#include "Range/Operations.h"
#include "Utils/AsciiSet.h"
#include "Range/Compositors/Zip.h"
#include "Range/Mutation/ReplaceSubrange.h"
#include "Range/Mutation/Copy.h"
#include "Range/Operations.h"

namespace Intra { namespace Range {

void StringFindAscii(StringView& str, CSpan<StringView> stopSubStrSet, size_t* oWhichIndex)
{
	if(stopSubStrSet.Empty())
	{
		TailAdvance(str, 0);
		if(oWhichIndex) *oWhichIndex = 0;
		return;
	}
	AsciiSet firstChars;
	for(size_t i=0; i<stopSubStrSet.Length(); i++)
	{
		if(stopSubStrSet[i].Empty())
		{
			if(oWhichIndex) *oWhichIndex = i;
			return;
		}
		firstChars.Set(stopSubStrSet[i].First());
	}
	for(;;)
	{
		FindAdvance(str, firstChars);
		if(str.Empty())
		{
			if(oWhichIndex) *oWhichIndex = stopSubStrSet.Length();
			return;
		}
		str.PopFirst();
		for(size_t i=0; i<stopSubStrSet.Length(); i++)
		{
			if(!StartsWith(str, stopSubStrSet[i].Drop())) continue;
			if(oWhichIndex) *oWhichIndex = i;
			return;
		}
	}
}

forceinline StringView StringReadUntilAscii(StringView& str, CSpan<StringView> stopSubStrSet, size_t* oWhichIndex)
{
	const char* begin = str.Data();
	StringFindAscii(str, stopSubStrSet, oWhichIndex);
	return {begin, str.Data()};
}

size_t StringMultiReplaceAsciiLength(StringView src,
	CSpan<StringView> fromSubStrs, CSpan<StringView> toSubStrs)
{
	INTRA_DEBUG_ASSERT(fromSubStrs.Length()==toSubStrs.Length());
	size_t substrIndex = 0;
	size_t len = 0;
	while(len += StringReadUntilAscii(src, fromSubStrs, &substrIndex).Length(), !src.Empty())
	{
		len += toSubStrs[substrIndex].Length();
		src.PopFirstExactly(fromSubStrs[substrIndex].Length());
	}
	return len;
}

StringView StringMultiReplaceAscii(StringView src, GenericStringView<char>& dstBuffer,
	CSpan<StringView> fromSubStrs, CSpan<StringView> toSubStrs)
{
	INTRA_DEBUG_ASSERT(fromSubStrs.Length()==toSubStrs.Length());
	char* begin = dstBuffer.Data();
	size_t substrIndex=0;
	while(CopyToAdvance(StringReadUntilAscii(src, fromSubStrs, &substrIndex), dstBuffer), !src.Empty())
	{
		CopyToAdvance(toSubStrs[substrIndex], dstBuffer);
		src.PopFirstExactly(fromSubStrs[substrIndex].Length());
	}
	return StringView(begin, dstBuffer.Data());
}


}}

