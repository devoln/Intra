#pragma once

#include "Range/ForwardDecls.h"
#include "Range/Concepts.h"
#include "Range/Decorators/Take.h"
#include "Platform/CppWarnings.h"
#include "Range/AsRange.h"

namespace Intra { namespace Range {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename R> struct RCycle
{
	enum: bool {RangeIsInfinite = true};

	forceinline RCycle(null_t=null): mCounter(0) {}

	forceinline RCycle(const R& range):
		mOriginalRange(range), mOffsetRange(range), mCounter(0) {}

	forceinline RCycle(R&& range):
		mOriginalRange(Meta::Move(range)), mOffsetRange(range), mCounter(0) {}

	forceinline bool Empty() const {return mOriginalRange.Empty();}
	forceinline ReturnValueTypeOf<R> First() const {return mOffsetRange.First();}

	forceinline void PopFirst()
	{
		mOffsetRange.PopFirst();
		mCounter++;
		if(!mOffsetRange.Empty()) return;
		mOffsetRange = mOriginalRange;
		mCounter = 0;
	}

	forceinline bool operator==(const RCycle& rhs) const
	{return mOffsetRange==rhs.mOffsetRange && mOriginalRange==rhs.mOriginalRange;}

private:
	R mOriginalRange, mOffsetRange;
	size_t mCounter;
};

template<typename R> struct RCycleRandom
{
	enum: bool {RangeIsInfinite = true};

	forceinline RCycleRandom(null_t=null): mCounter(0) {}
	forceinline RCycleRandom(const R& range): mOriginalRange(range), mCounter(0) {}
	forceinline RCycleRandom(R&& range): mOriginalRange(Meta::Move(range)), mCounter(0) {}

	forceinline bool Empty() const {return mOriginalRange.Empty();}

	forceinline ReturnValueTypeOf<R> First() const {return mOriginalRange[mCounter];}

	forceinline void PopFirst()
	{
		mCounter++;
		if(mCounter==mOriginalRange.Length()) mCounter=0;
	}

	forceinline ReturnValueTypeOf<R> operator[](size_t index) const
	{return mOriginalRange[(index+mCounter) % mOriginalRange.Length()];}

	forceinline bool operator==(const RCycleRandom& rhs) const
	{return mOriginalRange==rhs.mOriginalRange && mCounter==rhs.mCounter;}

	forceinline RTake<RCycleRandom<R>> operator()(size_t startIndex, size_t endIndex) const
	{
		INTRA_ASSERT(startIndex <= endIndex);
		RCycleRandom<R> result(mOriginalRange);
		result.mCounter = (mCounter+startIndex) % mOriginalRange.Length();
		return Take(result, endIndex-startIndex);
	}

private:
	R mOriginalRange;
	size_t mCounter;
};

template<typename R> forceinline Meta::EnableIf<
	IsAsInfiniteRange<R>::_,
AsRangeResult<R&&>> Cycle(R&& range) {return Range::Forward<R>(range);}

template<typename R> forceinline Meta::EnableIf<
	IsAsNonInfiniteForwardRange<R>::_ && !IsAsRandomAccessRange<R>::_,
RCycle<Meta::RemoveConstRef<AsRangeResult<R>>>> Cycle(R&& range)
{return Range::Forward<R>(range);}

template<typename R> forceinline Meta::EnableIf<
	IsAsNonInfiniteRandomAccessRange<R>::_,
RCycleRandom<Meta::RemoveConstRef<AsRangeResult<R>>>> Cycle(R&& range)
{return Range::Forward<R>(range);}

INTRA_WARNING_POP

}}
