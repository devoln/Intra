#pragma once

#include "Range/Concepts.h"
#include "Range/ForwardDecls.h"
#include "Algo/Search.hh"
#include "Algo/Mutation/Copy.h"
#include "Range/Decorators/Transversal.h"
#include "Utils/AsciiSet.h"

namespace Intra { namespace Algo {

template<typename R, typename RangeOfRanges, typename CharsToEscapeRange> Meta::EnableIf<
	Range::IsAsCharRange<R>::_ &&
	Range::IsFiniteInputRange<RangeOfRanges>::_ &&
	Range::IsFiniteForwardRange<Range::ValueTypeOf<RangeOfRanges>>::_ &&
	Meta::IsConvertible<Range::ValueTypeOf<Range::ValueTypeOf<RangeOfRanges>>, Range::ValueTypeOf<R>>::_ &&
	Range::IsFiniteForwardRange<CharsToEscapeRange>::_ &&
	Meta::IsConvertible<Range::ValueTypeOf<CharsToEscapeRange>, Range::ValueTypeOf<R>>::_,
size_t> StringEscapeLength(R&& src, const RangeOfRanges& escapeSequences, const CharsToEscapeRange& charsToEscape)
{
	R range = Range::Forward<R>(src);
	size_t len = 0;
	//AsciiSet escapeCharset = AsciiSet(charsToEscape);
	while(len += Algo::CountUntilAdvanceAny(range, charsToEscape), !range.Empty())
	{
		size_t index = 0;
		Algo::Find(charsToEscape, range.First(), &index);
		len += 1+Range::Count(Range::AtIndex(escapeSequences, index));
	}
	return len;
}

template<typename OR, typename R, typename RangeOfRanges, typename CharsToEscapeRange> Meta::EnableIf<
	Range::IsCharRange<R>::_ &&
	Range::IsOutputCharRange<OR>::_ &&
	Range::IsNonInfiniteInputRange<RangeOfRanges>::_ &&
	Range::IsNonInfiniteForwardRange<Range::ValueTypeOf<RangeOfRanges>>::_ &&
	Meta::IsConvertible<Range::ValueTypeOf<Range::ValueTypeOf<RangeOfRanges>>, Range::ValueTypeOf<OR>>::_ &&
	Range::IsNonInfiniteForwardRange<CharsToEscapeRange>::_ &&
	Meta::IsConvertible<Range::ValueTypeOf<CharsToEscapeRange>, Range::ValueTypeOf<OR>>::_,
Range::ResultOfTake<OR>> StringEscapeToAdvance(const R& src, OR&& dstBuffer,
	Range::ValueTypeOf<OR> escapeChar, const RangeOfRanges& escapeSequences, const CharsToEscapeRange& charsToEscape)
{
	auto range = src;
	auto dstBegin = dstBuffer;
	AsciiSet escapeCharset = AsciiSet(charsToEscape);
	while(Algo::CopyToAdvance(Range::TakeUntilAdvance(src, escapeCharset, dstBuffer)), !src.Empty())
	{
		dstBuffer.Put(escapeChar);
		size_t index = 0;
		Algo::Find(charsToEscape, src.First(), &index);
		Algo::CopyToAdvance(Range::AtIndex(escapeSequences, index), dstBuffer);
	}
	return Range::Take(dstBegin, DistanceTo(dstBegin, dstBuffer));
}

}}

