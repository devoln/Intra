#pragma once

#include "Range/Generators/Span.h"
#include "Range/Generators/StringView.h"

namespace Intra { namespace Algo {

void StringFindAscii(StringView& str, CSpan<StringView> subStrs, size_t* oWhichIndex=null);

StringView StringReadUntilAscii(StringView& str, CSpan<StringView> stopSubStrSet, size_t* oWhichIndex=null);


size_t StringMultiReplaceAsciiLength(StringView src,
	CSpan<StringView> fromSubStrs, CSpan<StringView> toSubStrs);

StringView StringMultiReplaceAscii(StringView src, GenericStringView<char>& dstBuffer,
	CSpan<StringView> fromSubStrs, CSpan<StringView> toSubStrs);

}}
