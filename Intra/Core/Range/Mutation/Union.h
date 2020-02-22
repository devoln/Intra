#pragma once

#include "Core/Range/Concepts.h"
#include "Core/Functional.h"
#include "Core/Range/Mutation/Heap.h"
#include "Core/Range/Mutation/Remove.h"

INTRA_BEGIN
template<typename RR, typename P> struct RUnion: P
{
	constexpr RUnion(RR ranges, P pred): P(Move(pred)), mRanges(Move(ranges))
	{
		RemoveRightAdvance(mRanges, IsEmpty);
		HeapBuild(mRanges);
	}

	INTRA_NODISCARD constexpr forceinline decltype(auto) First() const {return mRanges.First().First();}
	INTRA_NODISCARD constexpr forceinline bool Empty() const {return mRanges.Empty();}

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
	typename AsRR = TRangeOfType<RR>,
	typename T = TValueTypeOf<AsRR>
> INTRA_NODISCARD constexpr forceinline Requires<
	CRandomAccessRange<AsRR> &&
	CCallable<P, T, T>,
RUnion<AsRR, TFunctorOf<P>>> Union(RR&& range, P&& pred) {return {ForwardAsRange<RR>(range), ForwardAsFunc<P>(pred)};}
INTRA_END
