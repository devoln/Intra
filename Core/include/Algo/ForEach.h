#pragma once

#include "Meta/Type.h"
#include "Range/Concepts.h"
#include "Range/AsRange.h"

namespace Intra { namespace Algo {

using namespace Range::Concepts;

template<typename R, typename F, typename AsR=AsRangeResult<R>> Meta::EnableIf<
	IsConsumableRange<AsR>::_ &&
	Meta::IsCallable<F, ValueTypeOf<AsR>>::_
> ForEach(R&& r, F&& f)
{
	auto range = Range::Forward<R>(r);
	while(!range.Empty())
	{
		f(range.First());
		range.PopFirst();
	}
}

template<typename R, typename AsR=AsRangeResult<R>, typename... Args> Meta::EnableIf<
	IsConsumableRange<AsR>::_ &&
	Meta::IsCallable<ReturnValueTypeOf<AsR>, Args...>::_
> CallEach(R&& r, Args&&... args)
{
	auto range = Range::Forward<R>(r);
	while(!range.Empty())
	{
		range.First()(args...);
		range.PopFirst();
	}
}


}}
