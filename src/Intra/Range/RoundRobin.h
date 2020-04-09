#pragma once

#include "Intra/Numeric.h"
#include "Intra/Functional.h"
#include "Intra/Container/Tuple.h"
#include "Intra/Range/Concepts.h"
#include "Intra/Range/Operations.h"

INTRA_BEGIN
INTRA_IGNORE_WARNING_ASSIGN_IMPLICITLY_DELETED
template<typename... Ranges> struct RoundRobin
{
	static_assert(VAll(CInputRange<Ranges>...));
	static constexpr bool IsAnyInstanceFinite = VAll(CFiniteRange<Ranges>...);

	RoundRobin() = default;
	template<typename... Rs> constexpr RoundRobin(Rs&&... ranges): mRanges(ForwardAsRange<Rs>(ranges)...)
	{
		while(mCurrent < sizeof...(Ranges) && ForFieldAtIndex<bool>(IsEmpty, mRanges, mCurrent))
			mCurrent++;
	}

	constexpr decltype(auto) First() const
	{
		INTRA_PRECONDITION(!Empty());
		return ForFieldAtIndex<TCommon<Ranges...>>(Intra::First, mRanges, mCurrent);
	}

	constexpr void PopFirst()
	{
		INTRA_PRECONDITION(!Empty());
		ForFieldAtIndex<TCommon<Ranges...>>(Intra::PopFirst, mRanges, mCurrent);
		if(Empty()) return;
		do {
			mCurrent++;
			if(mCurrent == sizeof...(Ranges)) mCurrent = 0;
		} while(ForFieldAtIndex<bool>(IsEmpty, mRanges, mCurrent));
	}

	[[nodiscard]] constexpr bool Empty() const
	{
		return mCurrent == sizeof...(Ranges);
	}

	template<typename = Requires<
		VAll(CHasLength<Ranges>...)
	>> [[nodiscard]] constexpr index_t Length() const
	{
		return Accum(FAdd, Count, index_t(0))(mRanges);
	}

private:
	Tuple<Ranges...> mRanges;
	size_t mCurrent = 0;
};
template<typename... Rs> RoundRobin(Rs...) -> RoundRobin<TRangeOf<Rs>...>;
INTRA_END

#if INTRA_CONSTEXPR_TEST
#include "Intra/Range/RoundRobin.h"
#include "Intra/Range/Span.h"
#include "Intra/Range/Operations.h"
static_assert(Equals
	(RoundRobin(CSpan{1, 2, 3}, CSpan{10, 20, 30, 40}))
	(CSpan{1, 10, 2, 20, 3, 30, 40})
);
static_assert(Equals
	(RoundRobin(CSpan<int>{}, CSpan<unsigned>{10, 20, 30, 40}))
	(CSpan<unsigned>{10, 20, 30, 40})
);
#endif
