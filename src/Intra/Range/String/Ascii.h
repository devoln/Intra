#pragma once

#include "Intra/Range/Span.h"
#include "Intra/Range/StringView.h"

namespace Intra { INTRA_BEGIN
void StringFindAscii(StringView& str, Span<const StringView> subStrs, size_t* oWhichIndex=nullptr);

StringView StringReadUntilAscii(StringView& str, Span<const StringView> stopSubStrSet, size_t* oWhichIndex=nullptr);


size_t StringMultiReplaceAsciiLength(StringView src,
	Span<const StringView> fromSubStrs, Span<const StringView> toSubStrs);

StringView StringMultiReplaceAscii(StringView src, GenericStringView<char>& dstBuffer,
	Span<const StringView> fromSubStrs, Span<const StringView> toSubStrs);
} INTRA_END
