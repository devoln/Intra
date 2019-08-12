#pragma once

#include "Core/Range/Concepts.h"


#include "Core/Range/Search/Single.h"
#include "Core/Range/Search/Subrange.h"
#include "Take.h"

INTRA_CORE_RANGE_BEGIN
//TODO: Implement class RTakeUntilResult for InputRange


/** Takes part of `range` until `valueOrPredOrSubrange` advancing `range`.

  Pop first elements one by one until:
  1) an element for which predicate valueOrPredOrSubrange is true;
  2) an element equal valueOrPredOrSubrange;
  3) a subrange equal valueOrPredOrSubrange;
  4) range becomes empty.
  @return range of popped elements.
  @see CountUntilAdvance
*/
template<typename R, typename X,
	typename T = TValueTypeOf<R>
> INTRA_CONSTEXPR2 forceinline Requires<
	CNonInfiniteForwardRange<R> &&
	!CConst<R> &&
	((CConvertible<X, T> ||
		CCallable<X, T>) ||
	(CForwardRange<X> && !CInfiniteRange<X> &&
		CConvertible<TValueTypeOf<X>, T>)),
TTakeResult<R>> TakeUntilAdvance(R&& range, const X& valueOrPredOrSubrange, size_t* ioIndex=null)
{
	auto rangeCopy = range;
	size_t index = CountUntilAdvance(range, valueOrPredOrSubrange);
	if(ioIndex != null) *ioIndex += index;
	return Take(rangeCopy, index);
}

/** Takes part of `range` until `valueOrPredOrSubrange`.

  Goes through elements one by one until:
  1) an element for which `valueOrPredOrSubrange` is true if it is a predicate;
  2) an element equal valueOrPredOrSubrange if it is a value;
  3) a subrange equal valueOrPredOrSubrange if it is a range;
  4) end if `range`.
  @param valueOrPredOrSubrange Value to find or predicate or range.
  @return Part of `range` before found element.
  @see CountUntilAdvance
*/
template<typename R, typename X,
	typename T = TValueTypeOf<R>
> INTRA_NODISCARD constexpr forceinline Requires<
	CForwardRange<R> &&
	!CInfiniteRange<R> &&
	((CConvertible<X, T> ||
		CCallable<X, T>) ||
	(CForwardRange<X> &&
		!CInfiniteRange<X> &&
		CConvertible<TValueTypeOf<X>, T>)),
TTakeResult<R>> TakeUntil(const R& range, const X& valueOrPredOrSubrange, size_t* ioIndex=null)
{return TakeUntilAdvance(R(range), valueOrPredOrSubrange, ioIndex);}
INTRA_CORE_RANGE_END
