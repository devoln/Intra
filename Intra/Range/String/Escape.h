#pragma once

#include "Concepts/Range.h"
#include "Concepts/RangeOf.h"
#include "Range/Search.hh"
#include "Range/Mutation/Copy.h"
#include "Range/Decorators/Transversal.h"
#include "Utils/AsciiSet.h"

namespace Intra { namespace Range {

template<typename R, typename ES, typename CTE> Meta::EnableIf<
	Concepts::IsAsCharRange<R>::_ &&
	Concepts::IsFiniteInputRange<ES>::_ &&
	Concepts::IsFiniteForwardRange<Concepts::ValueTypeOf<ES>>::_ &&
	Meta::IsConvertible<Concepts::ValueTypeOf<Concepts::ValueTypeOf<ES>>, Concepts::ValueTypeOf<R>>::_ &&
	Concepts::IsFiniteForwardRange<CTE>::_ &&
	Meta::IsConvertible<Concepts::ValueTypeOf<CTE>, Concepts::ValueTypeOf<R>>::_,
size_t> StringEscapeLength(R&& src, const ES& escapeSequences, const CTE& charsToEscape)
{
	auto range = Range::Forward<R>(src);
	size_t len = 0;
	//AsciiSet escapeCharset = AsciiSet(charsToEscape);
	while(len += CountUntilAdvanceAny(range, charsToEscape), !range.Empty())
	{
		size_t index = 0;
		Find(charsToEscape, range.First(), &index);
		len += 1+Range::Count(Range::AtIndex(escapeSequences, index));
	}
	return len;
}

template<typename OR, typename R, typename ES, typename CTE> Meta::EnableIf<
	Concepts::IsCharRange<R>::_ &&
	Concepts::IsOutputCharRange<OR>::_ &&
	Concepts::IsNonInfiniteInputRange<ES>::_ &&
	Concepts::IsNonInfiniteForwardRange<Concepts::ValueTypeOf<ES>>::_ &&
	Meta::IsConvertible<Concepts::ValueTypeOf<Concepts::ValueTypeOf<ES>>, Concepts::ValueTypeOf<OR>>::_ &&
	Concepts::IsNonInfiniteForwardRange<CTE>::_ &&
	Meta::IsConvertible<Concepts::ValueTypeOf<CTE>, Concepts::ValueTypeOf<OR>>::_,
TakeResult<OR>> StringEscapeToAdvance(const R& src, OR&& dstBuffer,
	ValueTypeOf<OR> escapeChar, const ES& escapeSequences, const CTE& charsToEscape)
{
	using namespace Range;

	auto range = src;
	auto dstBegin = dstBuffer;
	AsciiSet escapeCharset = AsciiSet(charsToEscape);
	while(CopyToAdvance(TakeUntilAdvance(src, escapeCharset, dstBuffer)), !src.Empty())
	{
		dstBuffer.Put(escapeChar);
		size_t index = 0;
		Find(charsToEscape, src.First(), &index);
		CopyToAdvance(AtIndex(escapeSequences, index), dstBuffer);
	}
	return Take(dstBegin, DistanceTo(dstBegin, dstBuffer));
}

}}

