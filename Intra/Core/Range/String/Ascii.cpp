#include "Core/Range/String/Ascii.h"
#include "Core/Range/Comparison.h"
#include "Core/Range/Search/Single.h"
#include "Core/Range/Operations.h"
#include "Core/Range/Zip.h"
#include "Core/Range/Mutation/ReplaceSubrange.h"
#include "Core/Range/Mutation/Copy.h"
#include "Core/Range/Operations.h"
#include "Core//Range/StringView.h"
#include "Utils/AsciiSet.h"

// TODO: make this a particular case of existing generic algorithms

INTRA_CORE_RANGE_BEGIN
void StringFindAscii(StringView& str, CSpan<StringView> stopSubStrSet, size_t* oWhichIndex)
{
	if(stopSubStrSet.Empty())
	{
		TailAdvance(str, 0);
		if(oWhichIndex) *oWhichIndex = 0;
		return;
	}
	AsciiSet firstChars;
	for(size_t i = 0; i<stopSubStrSet.Length(); i++)
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
	INTRA_DEBUG_ASSERT(fromSubStrs.Length() == toSubStrs.Length());
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
	size_t substrIndex = 0;
	while(WriteTo(StringReadUntilAscii(src, fromSubStrs, &substrIndex), dstBuffer), !src.Empty())
	{
		WriteTo(toSubStrs[substrIndex], dstBuffer);
		src.PopFirstExactly(fromSubStrs[substrIndex].Length());
	}
	return StringView(begin, dstBuffer.Data());
}
INTRA_CORE_RANGE_END
