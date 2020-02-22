#pragma once

#include "Core/Functional.h"
#include "Core/Range/Take.h"
#include "Core/Range/Operations.h"
#include "Core/Range/Count.h"

INTRA_BEGIN
template<typename R> struct RChunks
{
	enum: bool
	{
		RangeIsFinite = CFiniteRange<R>,
		RangeIsInfinite = CInfiniteRange<R>
	};

	RChunks(const R& range, index_t chunkLen): mOriginalRange(range), mChunkLen(chunkLen) {}
	RChunks(R&& range, index_t chunkLen): mOriginalRange(Move(range)), mChunkLen(chunkLen) {}

	TTakeResult<R> First() const {return Take(mOriginalRange, mChunkLen);}
	void PopFirst() {PopFirstN(mOriginalRange, mChunkLen);}
	bool Empty() const {return mChunkLen == 0 || mOriginalRange.Empty();}

	template<typename U=R> INTRA_NODISCARD constexpr forceinline Requires<
		CHasLength<U>,
	index_t> Length() const {return (mOriginalRange.Length() + mChunkLen - 1) / mChunkLen;}

	template<typename U=R> forceinline Requires<
		CSliceable<U> &&
		CHasLength<U>,
	TSliceTypeOf<U>> operator[](size_t index) const
	{
		INTRA_PRECONDITION(index < Length());
		const size_t startIndex = index*mChunkLen;
		const size_t endIndex = FMin(startIndex + mChunkLen, mOriginalRange.Length());
		return mOriginalRange(startIndex, endIndex);
	}

	template<typename U=R> INTRA_NODISCARD constexpr forceinline Requires<
		CSliceable<U> &&
		CHasLength<U>,
	RChunks<TSliceTypeOf<U>>> operator()(index_t start, index_t end) const
	{
		INTRA_PRECONDITION(end <= Length());
		INTRA_PRECONDITION(start <= end);
		const size_t startIndex = start*mChunkLen;
		const size_t endIndex = FMin(end*mChunkLen, mOriginalRange.Length());
		return {mOriginalRange(startIndex, endIndex), mChunkLen};
	}

	template<typename U=R> INTRA_NODISCARD constexpr forceinline Requires<
		CSliceable<U> &&
		CHasLength<U>,
	TSliceTypeOf<U>> Last() const
	{
		INTRA_PRECONDITION(!Empty());
		size_t numToTake = mOriginalRange.Length()%mChunkLen;
		if(numToTake == 0) numToTake = mChunkLen;
		return Drop(mOriginalRange, mOriginalRange.Length()-numToTake);
	}

	template<typename U=R> constexpr forceinline Requires<
		CHasPopLast<U> &&
		CHasLength<U>
	> PopLast()
	{
		INTRA_PRECONDITION(!Empty());
		size_t numToPop = mOriginalRange.Length() % mChunkLen;
		if(numToPop == 0) numToPop = mChunkLen;
		PopLastExactly(mOriginalRange, numToPop);
	}

private:
	R mOriginalRange;
	index_t mChunkLen;
};

template<typename R> forceinline Requires<
	CAsForwardRange<R>,
RChunks<TRangeOfTypeNoCRef<R>>> Chunks(R&& range, index_t chunkLen)
{return {ForwardAsRange<R>(range), chunkLen};}
INTRA_END
