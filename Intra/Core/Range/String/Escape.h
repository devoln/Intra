#pragma once

#include "Core/Range/Concepts.h"
#include "Core/Range/Search/Distance.h"
#include "Core/Range/Take.h"
#include "Core/Range/Mutation/Copy.h"
#include "Core/Range/Transversal.h"
#include "Utils/AsciiSet.h"

INTRA_BEGIN
template<typename R, typename ES, typename CTE> Requires<
	CAsCharRange<R> &&
	CFiniteInputRange<ES> &&
	CFiniteForwardRange<TValueTypeOf<ES>> &&
	CConvertible<TValueTypeOf<TValueTypeOf<ES>>, TValueTypeOf<R>> &&
	CFiniteForwardRange<CTE> &&
	CConvertible<TValueTypeOf<CTE>, TValueTypeOf<R>>,
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
	CNonInfiniteInputRange<ES> &&
	CNonInfiniteForwardRange<TValueTypeOf<ES>> &&
	CConvertible<TValueTypeOf<TValueTypeOf<ES>>, TValueTypeOf<OR>> &&
	CNonInfiniteForwardRange<CTE> &&
	CConvertible<TValueTypeOf<CTE>, TValueTypeOf<OR>>,
TTakeResult<OR>> StringEscapeToAdvance(const R& src, OR&& dstBuffer,
	TValueTypeOf<OR> escapeChar, const ES& escapeSequences, const CTE& charsToEscape)
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
INTRA_END
