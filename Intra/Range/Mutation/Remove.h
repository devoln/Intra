#pragma once

#include "Cpp/Warnings.h"
#include "Utils/Debug.h"
#include "Concepts/Range.h"
#include "Range/Operations.h"
#include "Range/Sort/IsSorted.h"

namespace Intra { namespace Range {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename R, typename IndexRange> Meta::EnableIf<
	Concepts::IsBidirectionalRange<R>::_,
R> Remove(const R& rhs, const IndexRange& indices)
{
	if(indices.Empty()) return rhs;
	INTRA_DEBUG_ASSERT(IsSorted(indices, [](size_t a, size_t b){return a<b;}));
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

template<typename R, typename P> Meta::EnableIf<
	Concepts::IsBidirectionalRange<R>::_ &&
	!Meta::IsConst<R>::_ &&
	Meta::IsCallable<P, Concepts::ValueTypeOf<R>>::_,
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
	typename AsR = Concepts::RangeOfType<R> 
> forceinline Meta::EnableIf<
	Concepts::IsBidirectionalRange<AsR>::_ &&
	Meta::IsCallable<P, Concepts::ValueTypeOf<AsR>>::_,
AsR> Remove(R&& range, P pred)
{
	auto rangeCopy = Range::Forward<R>(range);
	return RemoveRightAdvance(rangeCopy, pred);}

INTRA_WARNING_POP

}}
