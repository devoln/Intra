#pragma once

#include "Intra/Range/Concepts.h"
#include "Intra/Range/Span.h"
#include "Intra/Container/Tuple.h"

INTRA_BEGIN
template<class F, typename T> struct TReducer: F
{
	T Value;

	template<typename F1, typename T> constexpr TReducer(F1&& f, T&& seed):
		F(ForwardAsFunc<F1>(f)), Value(INTRA_FWD(seed)) {}

	template<typename R> requires CConsumableList<R>
	constexpr auto operator()(R&& range)
	{
		if constexpr(CRange<R> && !CConst<R>)
		{
			auto res = Move(Value);
			while(!range.Empty()) res = F::operator()(res, Next(range));
			Value = Move(res);
			return Value;
		}
		else return operator()(Dup(ForwardAsRange<R>(range)));
	}
};
template<class F, typename T> TReducer(F, T) -> TReducer<TFunctorOf<F>, T>;

template<class F> struct Reduce: F
{
	template<typename F1> constexpr Reduce(F&& f): F(ForwardAsFunc<F1>(f)) {}

	template<typename R> requires CConsumableList<R>
	constexpr auto operator()(R&& range)
	{
		if constexpr(CRange<R> && !CConst<R>)
		{
			TReducer<F&, TResultOf<F, TRangeValue<R>, TRangeValue<R>>> reducer(*this, Next(range));
			return reducer(range);
		}
		else return operator()(Dup(ForwardAsRange<R>(range)));
	}
};
INTRA_END
