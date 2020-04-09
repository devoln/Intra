#pragma once

#include "Intra/Range/Span.h"
#include "Intra/Range/StringView.h"

INTRA_BEGIN
void StringFindAscii(StringView& str, CSpan<StringView> subStrs, size_t* oWhichIndex=null);

StringView StringReadUntilAscii(StringView& str, CSpan<StringView> stopSubStrSet, size_t* oWhichIndex=null);


size_t StringMultiReplaceAsciiLength(StringView src,
	CSpan<StringView> fromSubStrs, CSpan<StringView> toSubStrs);

StringView StringMultiReplaceAscii(StringView src, GenericStringView<char>& dstBuffer,
	CSpan<StringView> fromSubStrs, CSpan<StringView> toSubStrs);
INTRA_END
