#pragma once

#include <Intra/Concepts.h>

namespace Intra { INTRA_BEGIN
INTRA_IGNORE_WARN_COPY_MOVE_CONSTRUCT_IMPLICITLY_DELETED
INTRA_IGNORE_WARN_ASSIGN_IMPLICITLY_DELETED

template<typename F, typename... BindArgs>
[[nodiscard]] constexpr auto Bind(F&& f, BindArgs&&... args)
{
	return [...args = INTRA_FWD(args), f = FunctorOf(INTRA_FWD(f))](auto&&... restArgs)
		noexcept(noexcept(f(args..., INTRA_FWD(restArgs)...)))
		-> decltype(f(args..., INTRA_FWD(restArgs)...))
	{return f(args..., INTRA_FWD(restArgs)...);};
}

template<typename F, typename... BindArgs>
[[nodiscard]] constexpr auto BindRef(F&& f, BindArgs&... args)
{
	return [&, f = FunctorOf(INTRA_FWD(f))](auto&&... restArgs)
		noexcept(noexcept(f(args..., INTRA_FWD(restArgs)...)))
		-> decltype(f(args..., INTRA_FWD(restArgs)...))
	{return f(args..., INTRA_FWD(restArgs)...);};
}

template<class P> class FCount
{
	INTRA_NO_UNIQUE_ADDRESS P Predicate;
	index_t InvocationCounter = 0;
	constexpr FCount(P pred) noexcept: Predicate(INTRA_MOVE(pred)) {}
	template<typename... Args> requires CCallable<P, Args&&...>
	constexpr bool operator()(Args&&... args)
	{
		InvocationCounter++;
		return Predicate(INTRA_FWD(args)...);
	}
};

template<class P> class CountPred
{
	INTRA_NO_UNIQUE_ADDRESS P Predicate;
	index_t FalseInvocations = 0;
	index_t TrueInvocations = 0;
	constexpr CountPred(P pred) noexcept: Predicate(INTRA_MOVE(pred)) {}
	template<typename... Args> requires CCallable<P, Args&&...>
	constexpr bool operator()(Args&&... args)
	{
		const bool res = Predicate(INTRA_FWD(args)...);
		FalseInvocations += index_t(!res);
		TrueInvocations += index_t(!!res);
		return res;
	}
};

template<typename FR, typename FM, typename T> struct Accum
{
	INTRA_NO_UNIQUE_ADDRESS FR Reduce;
	INTRA_NO_UNIQUE_ADDRESS FM Map;
	T Result;
	constexpr Accum(FR reduce, FM map, T initialValue):
		Reduce(INTRA_MOVE(reduce)), Map(INTRA_MOVE(map)), Result(INTRA_MOVE(initialValue)) {}
	template<typename... Args> constexpr T operator()(Args&&... args)
		requires CCallable<FM, Args&&...> && CCallable<FR, T, TResultOf<FM, Args&&...>>
	{
		return Result = Reduce(Result, Map(INTRA_FWD(args)...));
	}
};
template<typename Reduce, typename Map, typename T>
Accum(Reduce&& reduce, Map&& map, T initialValue) ->
	Accum<TRemoveConstRef<TFunctorOf<Reduce>>, TRemoveConstRef<TFunctorOf<Map>>, T>;

template<typename P> constexpr auto AccumAll(P&& pred) {return Accum(And, INTRA_FWD(pred), true);}
template<typename P> constexpr auto AccumAny(P&& pred) {return Accum(Or, INTRA_FWD(pred), false);}

} INTRA_END
