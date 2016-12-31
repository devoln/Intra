#include "Algo/String/Ascii.h"
#include "Algo/Search.h"
#include "Range/Operations.h"

namespace Intra { namespace Algo {

void StringFindAscii(StringView& str, ArrayRange<const StringView> stopSubStrSet, size_t* oWhichIndex)
{
	if(stopSubStrSet.Empty())
	{
		Range::TailAdvance(str, 0);
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
			if(!str.StartsWith(stopSubStrSet[i].Drop())) continue;
			if(oWhichIndex) *oWhichIndex = i;
			return;
		}
	}
}

forceinline StringView StringReadUntilAscii(StringView& str, ArrayRange<const StringView> stopSubStrSet, size_t* oWhichIndex)
{
	const char* begin = str.Data();
	StringFindAscii(str, stopSubStrSet, oWhichIndex);
	return {begin, str.Data()};
}

size_t StringMultiReplaceAsciiLength(StringView src,
	ArrayRange<const StringView> fromSubStrs, ArrayRange<const StringView> toSubStrs)
{
	INTRA_ASSERT(fromSubStrs.Length()==toSubStrs.Length());
	size_t substrIndex = 0;
	size_t len = 0;
	while(len += StringReadUntilAscii(src, fromSubStrs, &substrIndex).Length(), !src.Empty())
	{
		len += toSubStrs[substrIndex].Length();
		src.PopFirstExactly(fromSubStrs[substrIndex].Length());
	}
	return len;
}

StringView StringMultiReplaceAscii(StringView src, ArrayRange<char>& dstBuffer,
	ArrayRange<const StringView> fromSubStrs, ArrayRange<const StringView> toSubStrs)
{
	INTRA_ASSERT(fromSubStrs.Length()==toSubStrs.Length());
	char* begin = dstBuffer.Begin;
	size_t substrIndex=0;
	while(CopyToAdvance(StringReadUntilAscii(src, fromSubStrs, &substrIndex), dstBuffer), !src.Empty())
	{
		CopyToAdvance(toSubStrs[substrIndex], dstBuffer);
		src.PopFirstExactly(fromSubStrs[substrIndex].Length());
	}
	return StringView(begin, dstBuffer.Begin);
}


}}

