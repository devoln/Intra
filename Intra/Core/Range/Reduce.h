#pragma once

#include "Core/Tuple.h"
#include "Core/Range/Span.h"
#include "Core/Range/Concepts.h"


INTRA_BEGIN
inline namespace Range {

template<typename R, typename F, typename S> INTRA_CONSTEXPR2 Requires<
	CConsumableRange<R>,
S> ReduceAdvance(R& range, const F& func, const S& seed)
{
	auto result = seed;
	while(!range.Empty())
	{
		result = func(result, range.First());
		range.PopFirst();
	}
	return result;
}

template<typename R, typename F,
	typename T = TValueTypeOf<R>
> INTRA_CONSTEXPR2 Requires<
	CInputRange<R> &&
	!CConst<R>,
TResultOf<F, T, T>> ReduceAdvance(R& range, F func)
{
	TResultOf<F, T, T> seed = range.First();
	range.PopFirst();
	return ReduceAdvance(range, func, seed);
}

template<typename R, typename F, typename S> INTRA_CONSTEXPR2 forceinline Requires<
	CAsConsumableRange<R>,
S> Reduce(R&& range, const F& func, const S& seed)
{
	auto rangeCopy = ForwardAsRange<R>(range);
	return ReduceAdvance(rangeCopy, func, seed);
}

template<typename R, typename F,
	typename AsR = TRangeOfType<R>,
	typename T = TValueTypeOf<AsR>,	
typename = Requires<
	CAsForwardRange<R>
>> INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline auto Reduce(R&& range, F func)
{
	auto rangeCopy = ForwardAsRange<R>(range);
	return ReduceAdvance(rangeCopy, func);
}

}
INTRA_END
