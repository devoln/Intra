#pragma once

#include "Core/Functional.h"
#include "Core/Assert.h"
#include "Core/Range/Concepts.h"
#include "Core/Range/Operations.h"
#include "Core/Range/Sort/IsSorted.h"

INTRA_BEGIN
template<typename R, typename IndexRange> constexpr Requires<
	CBidirectionalRange<R>,
R> Remove(const R& rhs, const IndexRange& indices)
{
	if(indices.Empty()) return rhs;
	INTRA_DEBUG_ASSERT(IsSorted(indices, FLess));
	size_t nextIndex = indices.First();
	auto indicesCopy = indices;
	indicesCopy.PopFirst();
	auto range = rhs;
	auto dst = Drop(range, nextIndex);
	auto src = dst;
	for(size_t i=nextIndex; !src.Empty(); i++)
	{
		if(i==nextIndex)
		{
			nextIndex = indicesCopy.First();
			indicesCopy.PopFirst();
			src.PopFirst();
			range.PopLast();
			continue;
		}
		dst.First() = src.First();
		src.PopFirst();
		dst.PopFirst();
	}
	return range;
}

template<typename R, typename P> constexpr Requires<
	CBidirectionalRange<R> &&
	!CConst<R> &&
	CCallable<P, TValueTypeOf<R>>,
R&> RemoveRightAdvance(R& range, P pred)
{
	auto dst = range;
	auto src = dst;
	bool somethingRemoved = false;
	for(size_t i=0; !src.Empty(); i++)
	{
		if(pred(src.First()))
		{
			src.PopFirst();
			somethingRemoved = true;
			range.PopLast();
			continue;
		}
		if(somethingRemoved) dst.First() = src.First();
		src.PopFirst();
		dst.PopFirst();
	}
	return range;
}

template<typename R, typename P,
	typename AsR = TRangeOfType<R> 
> constexpr forceinline Requires<
	CBidirectionalRange<AsR> &&
	CCallable<P, TValueTypeOf<AsR>>,
AsR> Remove(R&& range, P pred)
{
	auto rangeCopy = ForwardAsRange<R>(range);
	return RemoveRightAdvance(rangeCopy, pred);
}
INTRA_END
