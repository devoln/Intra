#pragma once

#include "Intra/Range/Concepts.h"
#include "Intra/Range/Search/Single.h"
#include "Intra/Range/Search/Subrange.h"
#include "Take.h"

INTRA_BEGIN

template<class R, class P> class TakeUntil: P
{
	R mRange;
	bool mEmpty = false;
public:
	constexpr TakeUntil(R range, P pred): P(Move(pred)), mRange(Move(range)) {}

	[[nodiscard]] constexpr decltype(auto) First() const
	{
		INTRA_PRECONDITION(!Empty());
		return mRange.First();
	}

	constexpr void PopFirst()
	{
		mRange.PopFirst();
		mEmpty = mRange.Empty();
		if constexpr(CCallable<P, TValueTypeOf<R>>)
			mEmpty = mEmpty || P::operator()(mRange.First());
		else mEmpty = mEmpty || P::operator()(mRange);
	}

	[[nodiscard]] constexpr bool Empty() const noexcept {return mEmpty;}

	[[nodiscard]] constexpr decltype(auto) Next()
	{
		INTRA_PRECONDITION(!Empty());
		if constexpr(CCallable<P, TValueTypeOf<R>>)
		{
			decltype(auto) res = Next(mRange);
			mEmpty = mRange.Empty() || P::operator()(res);
		}
		else
		{
			decltype(auto) res = mRange.First();
			PopFirst();
		}
		return res;
	}

	[[nodiscard]] constexpr const R& OriginalRange() const noexcept {return mRange;}
	[[nodiscard]] constexpr P& Predicate() noexcept {return *this;}
	[[nodiscard]] constexpr const P& Predicate() const noexcept {return *this;}
};
//TODO: deduction rules

template<class R, class P> constexpr decltype(auto) TakeUntilEagerly(R&& range, P&& pred)
{
	return Take(Forward<R>(range), Count(TakeUntil(range, Forward<P>(pred))));
}
INTRA_END
