#pragma once

#include "Intra/Functional.h"
#include "Intra/Assert.h"
#include "Intra/Range/Concepts.h"
#include "Intra/Range/Operations.h"
#include "Intra/Range/Sort/IsSorted.h"

namespace Intra { INTRA_BEGIN
template<typename R, typename IndexRange> constexpr Requires<
	CBidirectionalRange<R>,
R> Remove(const R& rhs, const IndexRange& indices)
{
	INTRA_PRECONDITION(IsSorted(indices, FLess));
	if(indices.Empty()) return rhs;
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
	CCallable<P, TRangeValue<R>>,
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
	typename AsR = TRangeOfRef<R>
> constexpr Requires<
	CBidirectionalRange<AsR> &&
	CCallable<P, TRangeValue<AsR>>,
AsR> Remove(R&& range, P pred)
{
	auto rangeCopy = ForwardAsRange<R>(range);
	return RemoveRightAdvance(rangeCopy, pred);
}
} INTRA_END
