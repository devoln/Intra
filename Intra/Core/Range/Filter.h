#pragma once


#include "Core/Range/Concepts.h"

INTRA_BEGIN
INTRA_WARNING_DISABLE_COPY_MOVE_IMPLICITLY_DELETED
template<typename R, class P> struct RFilter: private P
{
	enum: bool
	{
		RangeIsFinite = CFiniteRange<R>,
		RangeIsInfinite = CInfiniteRange<R>
	};

	constexpr forceinline RFilter(R&& range, P filterPredicate):
		P(filterPredicate), mOriginalRange(Move(range))
	{skip_falses_front(mOriginalRange, filterPredicate);}

	constexpr forceinline RFilter(const R& range, P filterPredicate):
		P(filterPredicate), mOriginalRange(range)
	{skip_falses_front(mOriginalRange, filterPredicate);}


	INTRA_NODISCARD constexpr forceinline TReturnValueTypeOf<R> First() const
	{
		INTRA_DEBUG_ASSERT(!Empty());
		return mOriginalRange.First();
	}

	constexpr void PopFirst()
	{
		mOriginalRange.PopFirst();
		skip_falses_front(mOriginalRange, *this);
	}

	template<typename U=R> INTRA_NODISCARD constexpr Requires<
		CHasLast<U> &&
		CHasPopLast<U>,
	TReturnValueTypeOf<R>> Last() const
	{
		INTRA_DEBUG_ASSERT(!Empty());
		auto&& b = mOriginalRange.Last();
		if(P::operator()(b)) return b;

		auto copy = mOriginalRange;
		copy.PopLast();
		skip_falses_back(copy, *this);
		return copy.Last();
	}

	template<typename U=R> constexpr Requires<
		CHasLast<U> &&
		CHasPopLast<U>
	> PopLast()
	{
		skip_falses_back(mOriginalRange, *this);
		mOriginalRange.PopLast();
	}

	INTRA_NODISCARD constexpr forceinline bool Empty() const {return mOriginalRange.Empty();}

private:
	static constexpr void skip_falses_front(R& originalRange, const P& p)
	{while(!originalRange.Empty() && !p(originalRange.First())) originalRange.PopFirst();}

	static constexpr void skip_falses_back(R& originalRange, const P& p)
	{while(!originalRange.Empty() && !p(originalRange.Last())) originalRange.PopLast();}

	R mOriginalRange;
};

template<typename R, typename P> forceinline Requires<
	CAsConsumableRange<R>,
RFilter<TRemoveConstRef<TRangeOfType<R>>, P>> Filter(R&& range, P predicate)
{return {Forward<R>(range), predicate};}
INTRA_END
