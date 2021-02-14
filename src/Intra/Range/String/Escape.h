#pragma once

#include "Intra/Range/Concepts.h"
#include "Intra/Range/Search/Distance.h"
#include "Intra/Range/Take.h"
#include "Intra/Range/Mutation/Copy.h"
#include "Intra/Range/Transversal.h"
#include "IntraX/Utils/AsciiSet.h"

namespace Intra { INTRA_BEGIN
template<typename R, typename ES, typename CTE> Requires<
	CCharList<R> &&
	CFiniteRange<ES> &&
	CFiniteForwardRange<TRangeValue<ES>> &&
	CConvertibleTo<TRangeValue<TRangeValue<ES>>, TRangeValue<R>> &&
	CFiniteForwardRange<CTE> &&
	CConvertibleTo<TRangeValue<CTE>, TRangeValue<R>>,
size_t> StringEscapeLength(R&& src, const ES& escapeSequences, const CTE& charsToEscape)
{
	auto range = ForwardAsRange<R>(src);
	size_t len = 0;
	//AsciiSet escapeCharset = AsciiSet(charsToEscape);
	while(len += CountUntilAdvanceAny(range, charsToEscape), !range.Empty())
	{
		size_t index = 0;
		Find(charsToEscape, range.First(), &index);
		len += 1 + Count(AtIndex(escapeSequences, index));
	}
	return len;
}

template<typename OR, typename R, typename ES, typename CTE> Requires<
	CCharRange<R> &&
	COutputCharRange<OR> &&
	CNonInfiniteRange<ES> &&
	CNonInfiniteForwardRange<TRangeValue<ES>> &&
	CConvertibleTo<TRangeValue<TRangeValue<ES>>, TRangeValue<OR>> &&
	CNonInfiniteForwardRange<CTE> &&
	CConvertibleTo<TRangeValue<CTE>, TRangeValue<OR>>,
TTakeResult<OR>> StringEscapeToAdvance(const R& src, OR&& dstBuffer,
	TRangeValue<OR> escapeChar, const ES& escapeSequences, const CTE& charsToEscape)
{
	auto range = src;
	auto dstBegin = dstBuffer;
	AsciiSet escapeCharset = AsciiSet(charsToEscape);
	while(WriteTo(TakeUntilAdvance(src, escapeCharset, dstBuffer)), !src.Empty())
	{
		dstBuffer.Put(escapeChar);
		size_t index = 0;
		Find(charsToEscape, src.First(), &index);
		WriteTo(AtIndex(escapeSequences, index), dstBuffer);
	}
	return Take(dstBegin, DistanceTo(dstBegin, dstBuffer));
}
} INTRA_END
