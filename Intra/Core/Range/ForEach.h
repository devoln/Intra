#pragma once

#include "Core/Type.h"
#include "Core/Range/Concepts.h"

INTRA_BEGIN
template<typename R, typename F,
	typename AsF = TFunctorOf<TRemoveReference<F>>,
	typename AsR = TRangeOfType<R>
> constexpr Requires<
	CConsumableRange<AsR> &&
	CCallable<AsF, TReturnValueTypeOf<AsR>>
> ForEach(R&& r, F&& f)
{
	auto func = ForwardAsFunc<F>(f);
	auto range = ForwardAsRange<R>(r);
	while(!range.Empty())
	{
		func(range.First());
		range.PopFirst();
	}
}

template<typename R,
	typename AsR = TRangeOfType<R>,
	typename... Args
> Requires<
	CConsumableRange<AsR> &&
	CCallable<TReturnValueTypeOf<AsR>, Args...>
> CallEach(R&& r, Args&&... args)
{
	auto range = ForwardAsRange<R>(r);
	while(!range.Empty())
	{
		range.First()(args...);
		range.PopFirst();
	}
}
INTRA_END
