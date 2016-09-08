#pragma once

#include "Algorithms/Range.h"
#include "Containers/StringView.h"

namespace Intra { namespace Algo {

void StringFindAscii(StringView& str, ArrayRange<const StringView> subStrs, size_t* oWhichIndex=null);

StringView StringReadUntilAscii(StringView& str, ArrayRange<const StringView> stopSubStrSet, size_t* oWhichIndex=null);


size_t StringMultiReplaceAsciiLength(StringView src,
	ArrayRange<const StringView> fromSubStrs, ArrayRange<const StringView> toSubStrs);

StringView StringMultiReplaceAscii(StringView src, ArrayRange<char>& dstBuffer,
	ArrayRange<const StringView> fromSubStrs, ArrayRange<const StringView> toSubStrs);

}}
