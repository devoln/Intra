#pragma once

#include "Core/Type.h"
#include "Core/Range/Concepts.h"

INTRA_BEGIN
///@{
/** Pops elements from ``from``, until it becomes equal to ``to`` or ``from`` becomes empty.
  As a result either from.Empty() or from == to.
  @return The number of popped elements.
*/
template<typename R> Requires<
	CInputRange<R> &&
	CEqualityComparable<R> &&
	!CConst<R> &&
	!CHasData<R>,
size_t> DistanceAdvanceTo(R& from, const R& to)
{
	size_t result = 0;
	while(!from.Empty() && !(from==to))
	{
		from.PopFirst();
		result++;
	}
	return result;
}

template<typename R> Requires<
	CInputRange<R> &&
	!CConst<R> &&
	CHasData<R>,
size_t> DistanceAdvanceTo(R& from, const R& to)
{
	size_t result = size_t(to.Data()-from.Data());
	from = to;
	return result;
}
///@}

//! How much elements must be popped from ``from``, to get range ``to`` or an empty range.
//! @return The number of popped elements.
template<typename R> Requires<
	CAccessibleRange<R>,
size_t> DistanceTo(R&& from, R&& to)
{
	auto fromCopy = Forward<R>(from);
	return DistanceAdvanceTo(fromCopy, to);
}
INTRA_END
