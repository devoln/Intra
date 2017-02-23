#pragma once

#include "Range/Decorators/Take.h"
#include "Range/Operations.h"
#include "Range/Generators/Count.h"
#include "Algo/Op.h"

namespace Intra { namespace Range {

template<typename R> struct RChunks
{
	enum: bool {RangeIsFinite = IsFiniteRange<R>::_,
		RangeIsInfinite = IsInfiniteRange<R>::_};

	RChunks(const R& range, size_t chunkLen): mOriginalRange(range), mChunkLen(chunkLen) {}
	RChunks(R&& range, size_t chunkLen): mOriginalRange(Meta::Move(range)), mChunkLen(chunkLen) {}

	TakeResult<R> First() const {return Take(mOriginalRange, mChunkLen);}
	void PopFirst() {PopFirstN(mOriginalRange, mChunkLen);}
	bool Empty() const {return mOriginalRange.Empty();}

	template<typename U=R> forceinline Meta::EnableIf<
		HasLength<U>::_,
	size_t> Length() const {return (mOriginalRange.Length()+mChunkLen-1)/mChunkLen;}

	template<typename U=R> forceinline Meta::EnableIf<
		HasSlicing<U>::_ && HasLength<U>::_,
	SliceTypeOf<U>> operator[](size_t index) const
	{
		INTRA_ASSERT(index<Length());
		const size_t startIndex = index*mChunkLen;
		const size_t endIndex = Op::Min(startIndex+mChunkLen, mOriginalRange.Length());
		return mOriginalRange(startIndex, endIndex);
	}

	template<typename U=R> forceinline Meta::EnableIf<
		HasSlicing<U>::_ && HasLength<U>::_,
	RChunks<SliceTypeOf<U>>> operator()(size_t start, size_t end) const
	{
		INTRA_ASSERT(end <= Length());
		INTRA_ASSERT(start <= end);
		const size_t startIndex = start*mChunkLen;
		const size_t endIndex = Op::Min(end*mChunkLen, mOriginalRange.Length());
		return {mOriginalRange(startIndex, endIndex), mChunkLen};
	}

	template<typename U=R> forceinline Meta::EnableIf<
		HasSlicing<U>::_ && HasLength<U>::_,
	SliceTypeOf<U>> Last() const
	{
		INTRA_ASSERT(!Empty());
		size_t numToTake = mOriginalRange.Length()%mChunkLen;
		if(numToTake==0) numToTake = mChunkLen;
		return Drop(mOriginalRange, mOriginalRange.Length()-numToTake);
	}

	template<typename U=R> forceinline Meta::EnableIf<
		HasPopLast<U>::_ && HasLength<U>::_
	> PopLast()
	{
		INTRA_ASSERT(!Empty());
		size_t numToPop = mOriginalRange.Length()%mChunkLen;
		if(numToPop==0) numToPop = mChunkLen;
		PopLastExactly(mOriginalRange, numToPop);
	}

private:
	R mOriginalRange;
	size_t mChunkLen;
};

template<typename R> forceinline Meta::EnableIf<
	IsAsForwardRange<R>::_,
RChunks<AsRangeResultNoCRef<R>>> Chunks(R&& range, size_t chunkLen)
{return {Range::Forward<R>(range), chunkLen};}

}}
