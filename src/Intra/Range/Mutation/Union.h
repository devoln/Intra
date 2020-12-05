#pragma once

#include "Intra/Range/Concepts.h"
#include "Intra/Functional.h"
#include "Intra/Range/Mutation/Heap.h"
#include "Intra/Range/Mutation/Remove.h"

INTRA_BEGIN
template<typename RR, typename P> struct RUnion: P
{
	constexpr RUnion(RR ranges, P pred): P(Move(pred)), mRanges(Move(ranges))
	{
		RemoveRightAdvance(mRanges, IsEmpty);
		HeapBuild(mRanges);
	}

	[[nodiscard]] constexpr decltype(auto) First() const {return mRanges.First().First();}
	[[nodiscard]] constexpr bool Empty() const {return mRanges.Empty();}

	constexpr void PopFirst()
	{
		auto& pred = *static_cast<P*>(this);
		auto& range = HeapPop(mRanges, pred);
		range.PopFirst();
		if(range.Empty()) mRanges.PopLast();
		else HeapPush(mRanges, pred);
	}

private:
	RR mRanges;
};

template<typename RR, typename P,
	typename AsRR = TRangeOfRef<RR>,
	typename T = TRangeValue<AsRR>
> [[nodiscard]] constexpr Requires<
	CRandomAccessRange<AsRR> &&
	CCallable<P, T, T>,
RUnion<AsRR, TFunctorOf<P>>> Union(RR&& range, P&& pred) {return {ForwardAsRange<RR>(range), ForwardAsFunc<P>(pred)};}
INTRA_END
