#pragma once

#include "Range/ForwardDecls.h"
#include "Concepts/Range.h"
#include "Range/Decorators/Take.h"
#include "Cpp/Warnings.h"
#include "Concepts/RangeOf.h"

namespace Intra { namespace Range {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename R> struct RCycle
{
	enum: bool {RangeIsInfinite = true};

	forceinline RCycle(null_t=null): mCounter(0) {}

	forceinline RCycle(const R& range):
		mOriginalRange(range), mOffsetRange(range), mCounter(0) {}

	forceinline RCycle(R&& range):
		mOriginalRange(Cpp::Move(range)), mOffsetRange(range), mCounter(0) {}

	forceinline bool Empty() const {return mOriginalRange.Empty();}
	forceinline Concepts::ReturnValueTypeOf<R> First() const {return mOffsetRange.First();}

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
	forceinline RCycleRandom(R&& range): mOriginalRange(Cpp::Move(range)), mCounter(0) {}

	forceinline bool Empty() const {return mOriginalRange.Empty();}

	forceinline Concepts::ReturnValueTypeOf<R> First() const {return mOriginalRange[mCounter];}

	forceinline void PopFirst()
	{
		mCounter++;
		if(mCounter==mOriginalRange.Length()) mCounter=0;
	}

	forceinline Concepts::ReturnValueTypeOf<R> operator[](size_t index) const
	{return mOriginalRange[(index+mCounter) % mOriginalRange.Length()];}

	forceinline bool operator==(const RCycleRandom& rhs) const
	{return mOriginalRange==rhs.mOriginalRange && mCounter==rhs.mCounter;}

	forceinline RTake<RCycleRandom<R>> operator()(size_t startIndex, size_t endIndex) const
	{
		INTRA_DEBUG_ASSERT(startIndex <= endIndex);
		RCycleRandom<R> result(mOriginalRange);
		result.mCounter = (mCounter+startIndex) % mOriginalRange.Length();
		return Take(result, endIndex-startIndex);
	}

private:
	R mOriginalRange;
	size_t mCounter;
};

template<typename R> forceinline Meta::EnableIf<
	Concepts::IsAsInfiniteRange<R>::_,
Concepts::RangeOfType<R&&>> Cycle(R&& range) {return Range::Forward<R>(range);}

template<typename R> forceinline Meta::EnableIf<
	Concepts::IsAsNonInfiniteForwardRange<R>::_ &&
	!Concepts::IsAsRandomAccessRange<R>::_,
RCycle<Meta::RemoveConstRef<Concepts::RangeOfType<R>>>> Cycle(R&& range)
{return Range::Forward<R>(range);}

template<typename R> forceinline Meta::EnableIf<
	Concepts::IsAsNonInfiniteRandomAccessRange<R>::_,
RCycleRandom<Meta::RemoveConstRef<Concepts::RangeOfType<R>>>> Cycle(R&& range)
{return Range::Forward<R>(range);}

INTRA_WARNING_POP

}}
