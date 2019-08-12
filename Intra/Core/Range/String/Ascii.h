#pragma once

#include "Core/Range/Span.h"
#include "Core/Range/StringView.h"

INTRA_CORE_RANGE_BEGIN
void StringFindAscii(StringView& str, CSpan<StringView> subStrs, size_t* oWhichIndex=null);

StringView StringReadUntilAscii(StringView& str, CSpan<StringView> stopSubStrSet, size_t* oWhichIndex=null);


size_t StringMultiReplaceAsciiLength(StringView src,
	CSpan<StringView> fromSubStrs, CSpan<StringView> toSubStrs);

StringView StringMultiReplaceAscii(StringView src, GenericStringView<char>& dstBuffer,
	CSpan<StringView> fromSubStrs, CSpan<StringView> toSubStrs);
INTRA_CORE_RANGE_END
