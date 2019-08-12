#pragma once


#include "Core/Range/Concepts.h"

INTRA_CORE_RANGE_BEGIN
INTRA_WARNING_DISABLE_COPY_MOVE_IMPLICITLY_DELETED
template<typename R, class P> struct RFilter: private P
{
	enum: bool
	{
		RangeIsFinite = CFiniteRange<R>,
		RangeIsInfinite = CInfiniteRange<R>
	};

	INTRA_CONSTEXPR2 forceinline RFilter(R&& range, P filterPredicate):
		P(filterPredicate), mOriginalRange(Move(range))
	{skip_falses_front(mOriginalRange, filterPredicate);}

	INTRA_CONSTEXPR2 forceinline RFilter(const R& range, P filterPredicate):
		P(filterPredicate), mOriginalRange(range)
	{skip_falses_front(mOriginalRange, filterPredicate);}


	INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline TReturnValueTypeOf<R> First() const
	{
		INTRA_DEBUG_ASSERT(!Empty());
		return mOriginalRange.First();
	}

	INTRA_CONSTEXPR2 void PopFirst()
	{
		mOriginalRange.PopFirst();
		skip_falses_front(mOriginalRange, *this);
	}

	template<typename U=R> INTRA_NODISCARD INTRA_CONSTEXPR2 Requires<
		CHasLast<U> &&
		CHasPopLast<U>,
	TReturnValueTypeOf<R>> Last() const
	{
		INTRA_DEBUG_ASSERT(!Empty());
		auto&& b = mOriginalRange.Last();
		if(operator(b)) return b;

		auto copy = mOriginalRange;
		copy.PopLast();
		skip_falses_back(copy, *this);
		return copy.Last();
	}

	template<typename U=R> INTRA_CONSTEXPR2 Requires<
		CHasLast<U> &&
		CHasPopLast<U>
	> PopLast()
	{
		skip_falses_back(mOriginalRange, *this);
		mOriginalRange.PopLast();
	}

	INTRA_NODISCARD constexpr forceinline bool Empty() const {return mOriginalRange.Empty();}

private:
	static INTRA_CONSTEXPR2 void skip_falses_front(R& originalRange, const P& p)
	{while(!originalRange.Empty() && !p(originalRange.First())) originalRange.PopFirst();}

	static INTRA_CONSTEXPR2 void skip_falses_back(R& originalRange, const P& p)
	{while(!originalRange.Empty() && !p(originalRange.Last())) originalRange.PopLast();}

	R mOriginalRange;
};

template<typename R, typename P> forceinline Requires<
	CAsConsumableRange<R>,
RFilter<TRemoveConstRef<TRangeOfType<R>>, P>> Filter(R&& range, P predicate)
{return {Forward<R>(range), predicate};}
INTRA_CORE_RANGE_END
