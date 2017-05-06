#pragma once

#include "Utils/Span.h"
#include "Utils/StringView.h"

namespace Intra { namespace Range {

void StringFindAscii(StringView& str, CSpan<StringView> subStrs, size_t* oWhichIndex=null);

StringView StringReadUntilAscii(StringView& str, CSpan<StringView> stopSubStrSet, size_t* oWhichIndex=null);


size_t StringMultiReplaceAsciiLength(StringView src,
	CSpan<StringView> fromSubStrs, CSpan<StringView> toSubStrs);

StringView StringMultiReplaceAscii(StringView src, GenericStringView<char>& dstBuffer,
	CSpan<StringView> fromSubStrs, CSpan<StringView> toSubStrs);

}}
