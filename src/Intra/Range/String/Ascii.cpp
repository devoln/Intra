#include "Intra/Range/String/Ascii.h"
#include "Intra/Range/Comparison.h"
#include "Intra/Range/Search/Single.h"
#include "Intra/Range/Operations.h"
#include "Intra/Range/Zip.h"
#include "Intra/Range/Mutation/ReplaceSubrange.h"
#include "Intra/Range/Mutation/Copy.h"
#include "Intra/Range/Operations.h"
#include "Intra//Range/StringView.h"
#include "IntraX/Utils/AsciiSet.h"

// TODO: make this a particular case of existing generic algorithms

namespace Intra { INTRA_BEGIN
void StringFindAscii(StringView& str, Span<const StringView> stopSubStrSet, size_t* oWhichIndex)
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

INTRA_FORCEINLINE StringView StringReadUntilAscii(StringView& str, Span<const StringView> stopSubStrSet, size_t* oWhichIndex)
{
	const char* begin = str.Data();
	StringFindAscii(str, stopSubStrSet, oWhichIndex);
	return StringView::FromPointerRange(begin, str.Data());
}

size_t StringMultiReplaceAsciiLength(StringView src,
	Span<const StringView> fromSubStrs, Span<const StringView> toSubStrs)
{
	INTRA_PRECONDITION(fromSubStrs.Length() == toSubStrs.Length());
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
	Span<const StringView> fromSubStrs, Span<const StringView> toSubStrs)
{
	INTRA_PRECONDITION(fromSubStrs.Length() == toSubStrs.Length());
	char* begin = dstBuffer.Data();
	size_t substrIndex = 0;
	while(WriteTo(StringReadUntilAscii(src, fromSubStrs, &substrIndex), dstBuffer), !src.Empty())
	{
		WriteTo(toSubStrs[substrIndex], dstBuffer);
		src.PopFirstExactly(fromSubStrs[substrIndex].Length());
	}
	return StringView::FromPointerRange(begin, dstBuffer.Data());
}
} INTRA_END
