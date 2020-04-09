#pragma once

#include "Intra/Range/Concepts.h"

INTRA_BEGIN
INTRA_IGNORE_WARNING_COPY_IMPLICITLY_DELETED
template<typename Rs> struct RFirstTransversal
{
	static constexpr bool IsAnyInstanceFinite = CFiniteRange<Rs>;

	INTRA_FORCEINLINE RFirstTransversal() = default;

	constexpr RFirstTransversal(Rs rangeOfRanges):
		mRanges(Move(rangeOfRanges)) {skip_empty();}

	[[nodiscard]] constexpr bool Empty() const {return mRanges.Empty();}

	[[nodiscard]] constexpr decltype(auto) First() const {return mRanges.First().First();}
	[[nodiscard]] constexpr decltype(auto) FirstRange() const {return mRanges.First();}

	constexpr void PopFirst()
	{
		mRanges.PopFirst();
		skip_empty();
	}

	[[nodiscard]] constexpr auto Last() const
	{
		while(mRanges.Last().Empty()) mRanges.PopLast();
		return mRanges.Last().First();
	}

	constexpr void PopLast()
	{
		INTRA_PRECONDITION(!Empty());
		while(mRanges.Last().Empty()) mRanges.PopLast();
		mRanges.PopLast();
	}

private:
	mutable Rs mRanges;

	constexpr void skip_empty() {while(!Empty() && mRanges.First().Empty()) mRanges.PopFirst();}
};

template<typename R,
	typename AsR = TRangeOfRef<R&&>
> [[nodiscard]] constexpr Requires<
	CAccessibleRange<AsR> &&
	CAsInputRange<TValueTypeOf<AsR>>,
RFirstTransversal<TRemoveConstRef<AsR>>> FirstTransversal(R&& range)
{return ForwardAsRange<R>(range);}
INTRA_END
