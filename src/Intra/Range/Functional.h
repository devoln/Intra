#pragma once

#include <Intra/Functional.h>
#include <Intra/Concepts.h>

namespace Intra { INTRA_BEGIN

template<CNonInfiniteForwardRange R, CElementPredicate P = decltype(Equal)> class IsOneOf
{
	INTRA_NO_UNIQUE_ADDRESS P mPred;
	R mSet;
public:
	constexpr IsOneOf(R set): mSet(INTRA_MOVE(set)) {}
	constexpr IsOneOf(R set, P pred): mPred(INTRA_MOVE(pred)), mSet(INTRA_MOVE(set)) {}

	template<typename T> requires CCallableWithSignature<P, bool(TRangeValueRef<R>, T&&)>
	[[nodiscard]] constexpr bool operator()(T&& arg)
	{
		Which = mSet;
		while(!Which.Empty() && !mPred(Which.First(), arg))
			Which.PopFirst();
		return !Which.Empty();
	}

	// If last predicate call returned true, points to the first matching element. Otherwise empty.
	R Which;
};
template<CNonInfiniteForwardList L> IsOneOf(L) -> IsOneOf<TRangeOfRef<L>>;
template<CNonInfiniteForwardList L, class P> IsOneOf(L, P) -> IsOneOf<TRangeOfRef<L>, TFunctorOf<P>>;

} INTRA_END
