#pragma once

#include "Range/Concepts.h"
#include "Range/ForwardDecls.h"
#include "Algo/Search.hh"
#include "Algo/Mutation/Copy.h"
#include "Range/Decorators/Transversal.h"
#include "Utils/AsciiSet.h"

namespace Intra { namespace Algo {

using namespace Range::Concepts;

template<typename R, typename ES, typename CTE> Meta::EnableIf<
	IsAsCharRange<R>::_ &&
	IsFiniteInputRange<ES>::_ &&
	IsFiniteForwardRange<ValueTypeOf<ES>>::_ &&
	Meta::IsConvertible<ValueTypeOf<ValueTypeOf<ES>>, ValueTypeOf<R>>::_ &&
	IsFiniteForwardRange<CTE>::_ &&
	Meta::IsConvertible<ValueTypeOf<CTE>, ValueTypeOf<R>>::_,
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
	IsCharRange<R>::_ &&
	IsOutputCharRange<OR>::_ &&
	IsNonInfiniteInputRange<ES>::_ &&
	IsNonInfiniteForwardRange<ValueTypeOf<ES>>::_ &&
	Meta::IsConvertible<ValueTypeOf<ValueTypeOf<ES>>, ValueTypeOf<OR>>::_ &&
	IsNonInfiniteForwardRange<CTE>::_ &&
	Meta::IsConvertible<ValueTypeOf<CTE>, ValueTypeOf<OR>>::_,
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

