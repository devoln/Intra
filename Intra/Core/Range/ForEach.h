#pragma once

#include "Core/Type.h"
#include "Core/Range/Concepts.h"


INTRA_BEGIN
namespace Range {

template<typename R, typename F,
	typename AsR = TRangeOfType<R>
> INTRA_CONSTEXPR2 Requires<
	CConsumableRange<AsR> &&
	CCallable<F, TValueTypeOf<AsR>>
> ForEach(R&& r, F&& f)
{
	auto range = ForwardAsRange<R>(r);
	while(!range.Empty())
	{
		f(range.First());
		range.PopFirst();
	}
}

template<typename R,
	typename AsR = TRangeOfType<R>, typename... Args
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

}
INTRA_END
