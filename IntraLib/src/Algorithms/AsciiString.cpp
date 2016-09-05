#include "Algorithms/AsciiString.h"
#include "Algorithms/RangeIteration.h"

namespace Intra { namespace Algo {

void StringFindAscii(StringView& str, ArrayRange<const StringView> stopSubStrSet, intptr* oWhichIndex)
{
	if(stopSubStrSet.Empty())
	{
		str.TailAdvance(0);
		if(oWhichIndex) *oWhichIndex = -1;
		return;
	}
	AsciiSet firstChars;
	for(size_t i=0; i<stopSubStrSet.Length(); i++)
	{
		if(stopSubStrSet[i].Empty())
		{
			if(oWhichIndex) *oWhichIndex = intptr(i);
			return;
		}
		firstChars.Set(stopSubStrSet[i].First());
	}
	for(;;)
	{
		str.FindAdvance(firstChars);
		if(str.Empty())
		{
			if(oWhichIndex) *oWhichIndex = -1;
			return;
		}
		str.PopFirst();
		for(size_t i=0; i<stopSubStrSet.Length(); i++)
		{
			if(!str.StartsWith(stopSubStrSet[i].Drop())) continue;
			if(oWhichIndex) *oWhichIndex = intptr(i);
			return;
		}
	}
}

forceinline StringView StringReadUntilAscii(StringView& str, ArrayRange<const StringView> stopSubStrSet, intptr* oWhichIndex)
{
	const char* begin = str.Data();
	StringFindAscii(str, stopSubStrSet, oWhichIndex);
	return {begin, str.Data()};
}

size_t StringMultiReplaceAsciiLength(StringView src,
	ArrayRange<const StringView> fromSubStrs, ArrayRange<const StringView> toSubStrs)
{
	INTRA_ASSERT(fromSubStrs.Length()==toSubStrs.Length());
	intptr substrIndex;
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
	intptr substrIndex;
	while(StringReadUntilAscii(src, fromSubStrs, &substrIndex).CopyToAdvance(dstBuffer), !src.Empty())
	{
		toSubStrs[substrIndex].CopyToAdvance(dstBuffer);
		src.PopFirstExactly(fromSubStrs[substrIndex].Length());
	}
	return StringView(begin, dstBuffer.Begin);
}


}}

