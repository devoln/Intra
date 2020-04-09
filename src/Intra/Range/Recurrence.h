#pragma once

#include "Intra/Range/Concepts.h"
#include "Intra/Functional.h"
#include "Intra/Container/SArray.h"
#include "Intra/Container/Tuple.h"

INTRA_BEGIN
INTRA_IGNORE_WARNING_ASSIGN_IMPLICITLY_DELETED
template<typename F, typename T, size_t N> class Recurrence: F, TSelect<Tuple<size_t>, EmptyType, (N > 2)>
{
	SArray<T, N> mState;
	using IndexT = TSelect<Tuple<size_t>, EmptyType, (N > 2)>;
	template<size_t... Is> void popFirstVarImpl(TIndexSeq<Is...>)
	{
		size_t& index = get<0>(static_cast<IndexT&>(*this));
		auto& nextVal = mState[index++];
		if(index == N) index = 0;
		nextVal = F::operator()(mState[index], mState[(index+(1+Is) - (index >= N-(1+Is)? N: 0))...]);
	}
public:
	static constexpr bool IsAnyInstanceInfinite = true;

	template<typename F1, typename... Ts, typename = Requires<CCallable<F, Ts...>>>
	constexpr Recurrence(F1&& function, Ts&&... initialSequence):
		F(ForwardAsFunc<F1>(function)),
		mState{Forward<Ts>(initialSequence)...} {}

	[[nodiscard]] constexpr T First() const
	{
		if constexpr(N <= 2) return mState[0];
		else return mState[get<0>(static_cast<IndexT&>(*this))];
	}
	constexpr void PopFirst()
	{
		if constexpr(N == 1) mState[0] = F::operator()(mState[0]);
		else if constexpr(N == 2)
		{
			mState[0] = F::operator()(mState[0], mState[1]);
			Swap(mState[0], mState[1]);
		}
		else popFirstVarImpl(TMakeIndexSeq<N-1>());
	}
	[[nodiscard]] constexpr bool Empty() const {return false;}
};
template<typename F, typename... Ts>
Recurrence(F&&, Ts&&...) -> Recurrence<TFunctorOf<F&&>, TCommon<Ts...>, sizeof...(Ts)>;
INTRA_END
