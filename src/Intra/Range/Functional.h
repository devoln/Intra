#pragma once

#include "Core/Functional.h"
#include "Core/Range/Concepts.h"
#include "Core/Range/Take.h"

INTRA_BEGIN

template<class R, typename P = TFEqual> class TIsOneOf: P
{
	static_assert(CNonInfiniteForwardRange<R>);
	R mSet;
	R mFound;
public:
	constexpr TIsOneOf(R set): mSet(Move(set)) {}
	constexpr TIsOneOf(R set, P pred): P(Move(pred)), mSet(Move(set)) {}
	template<typename T> [[nodiscard]] constexpr Requires<
		CCallable<P, T&&>,
	bool> operator()(T&& arg)
	{
		mFound = mSet;
		while(!mFound.Empty() && !P::operator()(mFound.First())) mFound.PopFirst();
		return !mFound.Empty();
	}

	auto Which() const {return mFound.First();}
};
template<class R> TIsOneOf(R) -> TIsOneOf<TRangeOfRef<R>>;
template<class R, class P> TIsOneOf(R, P) -> TIsOneOf<TRangeOfRef<R>, TFunctorOf<P>>;

INTRA_END
