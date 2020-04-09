#pragma once

#include "Intra/Range/Concepts.h"
#include "Intra/Range/Span.h"
#include "Intra/Container/Tuple.h"

INTRA_BEGIN
template<class F, typename T> struct TReducer: F
{
	T Value;

	template<typename F, typename T> constexpr TReducer(F&& f, T&& seed):
		F(ForwardAsFunc<F>(f)), Value(Forward<T>(seed)) {}

	template<typename R, typename = Requires<
		CAsConsumableRange<R>
	>> constexpr auto operator()(R&& range)
	{
		if constexpr(CInputRange<R> && !CConst<R>)
		{
			auto res = Move(Value);
			while(!range.Empty()) res = func(res, Next(range));
			Value = Move(res);
			return Value;
		}
		else
		{
			auto r = ForwardAsRange<R>(range);
			return operator()(r);
		}
	}
};
template<class F, typename T> TReducer(F, T) -> TReducer<TFunctorOf<F>, T>;

template<class F> struct Reduce: F
{
	template<typename F> constexpr Reduce(F&& f): F(ForwardAsFunc<F>(f)) {}

	template<typename R, typename = Requires<
		CAsConsumableRange<R>
	>> constexpr auto operator()(R&& range)
	{
		if constexpr(CInputRange<R> && !CConst<R>)
		{
			TReducer<F&, TResultOf<F, TValueTypeOf<R>, TValueTypeOf<R>>> reducer(*this, Next(range));
			return reducer(range);
		}
		else
		{
			auto r = ForwardAsRange<R>(range);
			return operator()(r);
		}
	}
};
INTRA_END
