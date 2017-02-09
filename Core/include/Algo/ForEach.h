#pragma once

#include "Meta/Type.h"
#include "Range/Concepts.h"
#include "Range/AsRange.h"

namespace Intra { namespace Algo {

template<typename R, typename P> Meta::EnableIf<
	Range::IsAsConsumableRange<R>::_ &&
	Meta::IsCallable<P, Range::ValueTypeOfAs<R>>::_
> ForEach(R&& r, P&& pred)
{
	auto range = Range::Forward<R>(r);
	while(!range.Empty())
	{
		pred(range.First());
		range.PopFirst();
	}
}

template<typename T, size_t N, typename P> Meta::EnableIf<
	Meta::IsCallable<P, T>::_
> ForEach(T(&arr)[N], P&& pred)
{ForEach(Range::AsRange(arr), Meta::Forward<P>(pred));}

}}
