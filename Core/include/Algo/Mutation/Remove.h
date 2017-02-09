#pragma once

#include "Platform/CppWarnings.h"
#include "Platform/Debug.h"
#include "Range/Concepts.h"
#include "Range/Operations.h"
#include "Algo/Sort/IsSorted.h"

namespace Intra { namespace Algo {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename R, typename IndexRange> Meta::EnableIf<
	Range::IsBidirectionalRange<R>::_,
R> Remove(const R& rhs, const IndexRange& indices)
{
	if(indices.Empty()) return rhs;
	INTRA_ASSERT(IsSorted(indices, [](size_t a, size_t b){return a<b;}));
	size_t nextIndex = indices.First();
	auto indicesCopy = indices;
	indicesCopy.PopFirst();
	auto range = rhs;
	R dst = Range::Drop(range, nextIndex), src = dst;
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
	Range::IsBidirectionalRange<R>::_ && !Meta::IsConst<R>::_ &&
	Meta::IsCallable<P, Range::ValueTypeOf<R>>::_,
R&&> RemoveRightAdvance(R&& range, P pred)
{
	R dst = range, src = dst;
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
	return Meta::Forward<R>(range);
}

template<typename R, typename P> forceinline Meta::EnableIf<
	Range::IsBidirectionalRange<R>::_ &&
	Meta::IsCallable<P, Range::ValueTypeOf<R>>::_,
R> Remove(const R& range, P pred) {return RemoveRightAdvance(R(range), pred);}

INTRA_WARNING_POP

}}
