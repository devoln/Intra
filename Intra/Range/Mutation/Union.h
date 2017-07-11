#pragma once

#include "Cpp/Warnings.h"
#include "Cpp/Features.h"

#include "Concepts/Range.h"

#include "Funal/Op.h"

#include "Range/Mutation/Heap.h"
#include "Range/Mutation/Remove.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Range {

template<typename RR, typename P> struct RUnion
{
	typedef Concepts::ValueTypeOf<RR> R;
	typedef Concepts::ReturnValueTypeOf<R> T;

	RUnion(RR ranges, P pred): mRanges(Cpp::Move(ranges)), mPred(Cpp::Move(pred))
	{
		RemoveRightAdvance(mRanges, Funal::IsEmpty);
		HeapBuild(mRanges);
	}

	forceinline T First() const {return mRanges.First().First();}
	forceinline bool Empty() const {return mRanges.Empty();}

	void PopFirst()
	{
		auto& range = HeapPop(mRanges, mPred);
		range.PopFirst();
		if(range.Empty()) mRanges.PopLast();
		else HeapPush(mRanges, mPred);
	}

private:
	RR mRanges;
	P mPred;
};

template<typename RR, typename P,
	typename AsRR = Concepts::RangeOfType<RR>,
	typename T = Concepts::ValueTypeOf<AsRR>
> forceinline Meta::EnableIf<
	Concepts::IsRandomAccessRange<AsRR>::_,
	Meta::IsCallable<P, T, T>::_,
RUnion<AsRR, P>> Union(RR&& range, P pred) {return {Range::Forward<RR>(range), Cpp::Move(pred)};}

}}

INTRA_WARNING_POP
