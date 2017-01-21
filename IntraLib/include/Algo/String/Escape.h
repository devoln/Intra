#pragma once

#include "Range/Concepts.h"
#include "Range/ForwardDecls.h"
#include "Algo/Search.h"
#include "Range/Iteration/Transversal.h"
#include "Containers/AsciiSet.h"

namespace Intra { namespace Algo {

template<typename R, typename RangeOfRanges, typename CharsToEscapeRange> Meta::EnableIf<
	Range::IsCharRange<R>::_ &&
	Range::IsFiniteInputRange<RangeOfRanges>::_ &&
	Range::IsFiniteForwardRange<Range::ValueTypeOf<RangeOfRanges>>::_ &&
	Range::ValueTypeIsConvertible<Range::ValueTypeOf<RangeOfRanges>, Range::ValueTypeOf<R>>::_ &&
	Range::IsFiniteForwardRange<CharsToEscapeRange>::_ &&
	Range::ValueTypeIsConvertible<CharsToEscapeRange, Range::ValueTypeOf<R>>::_,
size_t> StringEscapeLength(const R& src, const RangeOfRanges& escapeSequences, const CharsToEscapeRange& charsToEscape)
{
	R range = src;
	size_t len = 0;
	//AsciiSet escapeCharset = AsciiSet(charsToEscape);
	while(len += Algo::CountUntilAdvanceAny(range, charsToEscape), !range.Empty())
	{
		size_t index = 0;
		Find(charsToEscape, range.First(), &index);
		len += 1+Range::Count(Range::AtIndex(escapeSequences, index));
	}
	return len;
}

template<typename DstRange, typename SrcRange, typename RangeOfRanges, typename CharsToEscapeRange> Meta::EnableIf<
	Range::IsCharRange<SrcRange>::_ &&
	Range::IsOutputCharRange<DstRange>::_ &&
	Range::IsFiniteInputRange<RangeOfRanges>::_ &&
	Range::IsFiniteForwardRange<Range::ValueTypeOf<RangeOfRanges>>::_ &&
	Range::ValueTypeIsConvertible<Range::ValueTypeOf<RangeOfRanges>, Range::ValueTypeOf<DstRange>>::_ &&
	Range::IsFiniteForwardRange<CharsToEscapeRange>::_ &&
	Range::ValueTypeIsConvertible<CharsToEscapeRange, Range::ValueTypeOf<DstRange>>::_,
Range::ResultOfTake<DstRange>> StringEscapeToAdvance(const SrcRange& src, DstRange&& dstBuffer,
	Range::ValueTypeOf<DstRange> escapeChar, const RangeOfRanges& escapeSequences, const CharsToEscapeRange& charsToEscape)
{
	auto range = src;
	auto dstBegin = dstBuffer;
	AsciiSet escapeCharset = AsciiSet(charsToEscape);
	while(CopyToAdvance(ReadUntilAdvance(src, escapeCharset, dstBuffer)), !src.Empty())
	{
		dstBuffer.Put(escapeChar);
		size_t index = 0;
		Find(charsToEscape, src.First(), &index);
		CopyToAdvance(Range::AtIndex(escapeSequences, index), dstBuffer);
	}
	return Range::Take(dstBegin, DistanceTo(dstBegin, dstBuffer));
}

}}

