#pragma once


#include "Intra/Range/Concepts.h"

INTRA_BEGIN
INTRA_IGNORE_WARNING_COPY_MOVE_IMPLICITLY_DELETED
template<typename R, class P> struct RFilter: private P
{
	enum: bool
	{
		IsAnyInstanceFinite = CFiniteRange<R>,
		IsAnyInstanceInfinite = CInfiniteRange<R>
	};

	constexpr RFilter(R&& range, P filterPredicate):
		P(filterPredicate), mOriginalRange(Move(range))
	{skip_falses_front(mOriginalRange, filterPredicate);}

	constexpr RFilter(const R& range, P filterPredicate):
		P(filterPredicate), mOriginalRange(range)
	{skip_falses_front(mOriginalRange, filterPredicate);}


	[[nodiscard]] constexpr TReturnValueTypeOf<R> First() const
	{
		INTRA_DEBUG_ASSERT(!Empty());
		return mOriginalRange.First();
	}

	constexpr void PopFirst()
	{
		mOriginalRange.PopFirst();
		skip_falses_front(mOriginalRange, *this);
	}

	template<typename U=R> [[nodiscard]] constexpr Requires<
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

	[[nodiscard]] constexpr bool Empty() const {return mOriginalRange.Empty();}

private:
	static constexpr void skip_falses_front(R& originalRange, const P& p)
	{while(!originalRange.Empty() && !p(originalRange.First())) originalRange.PopFirst();}

	static constexpr void skip_falses_back(R& originalRange, const P& p)
	{while(!originalRange.Empty() && !p(originalRange.Last())) originalRange.PopLast();}

	R mOriginalRange;
};

template<typename R, typename P> INTRA_FORCEINLINE Requires<
	CAsConsumableRange<R>,
RFilter<TRemoveConstRef<TRangeOfRef<R>>, P>> Filter(R&& range, P predicate)
{return {Forward<R>(range), predicate};}
INTRA_END
