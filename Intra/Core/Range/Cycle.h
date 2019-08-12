#pragma once

#include "Core/Operations.h"
#include "Core/Range/Concepts.h"


#include "Core/Range/Take.h"

INTRA_CORE_RANGE_BEGIN
INTRA_WARNING_DISABLE_ASSIGN_IMPLICITLY_DELETED
template<typename R> struct RCycle
{
	enum: bool {RangeIsInfinite = true};

	constexpr forceinline RCycle(R range):
		mOriginalRange(Move(range)), mOffsetRange(range), mCounter(0) {}

	INTRA_NODISCARD constexpr forceinline bool Empty() const {return mOriginalRange.Empty();}
	INTRA_NODISCARD constexpr forceinline auto First() const {return mOffsetRange.First();}

	INTRA_CONSTEXPR2 forceinline void PopFirst()
	{
		mOffsetRange.PopFirst();
		mCounter++;
		if(!mOffsetRange.Empty()) return;
		CopyAssign(mOffsetRange, mOriginalRange);
		mCounter = 0;
	}

private:
	R mOriginalRange, mOffsetRange;
	size_t mCounter;
};

template<typename R> struct RCycleRandom
{
	enum: bool {RangeIsInfinite = true};

	constexpr forceinline RCycleRandom(const R& range): mOriginalRange(range), mCounter(0) {}
	constexpr forceinline RCycleRandom(R&& range): mOriginalRange(Move(range)), mCounter(0) {}

	INTRA_NODISCARD constexpr forceinline bool Empty() const {return mOriginalRange.Empty();}

#if defined(_MSC_VER) && _MSC_VER <= 1900
#pragma warning(disable: 4100) //this: unreferenced formal parameter? What?!
#endif
	INTRA_NODISCARD constexpr forceinline auto First() const {return mOriginalRange[mCounter];}

	INTRA_CONSTEXPR2 forceinline void PopFirst()
	{
		mCounter++;
		if(mCounter == mOriginalRange.Length()) mCounter=0;
	}

	INTRA_NODISCARD constexpr forceinline TReturnValueTypeOf<R> operator[](size_t index) const
	{
		return mOriginalRange[(index+mCounter) % mOriginalRange.Length()];
	}

	INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline RTake<RCycleRandom<R>> operator()(size_t startIndex, size_t endIndex) const
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

template<typename R, typename AsR = TRangeOfType<R&&>> INTRA_NODISCARD constexpr forceinline Requires<
	CInfiniteRange<AsR>,
AsR> Cycle(R&& range) {return ForwardAsRange<R>(range);}

template<typename R, typename AsR = TRangeOfTypeNoCRef<R>> INTRA_NODISCARD constexpr forceinline Requires<
	CNonInfiniteForwardRange<AsR> &&
	!CRandomAccessRange<AsR>,
RCycle<AsR>> Cycle(R&& range)
{return ForwardAsRange<R>(range);}

template<typename R, typename AsR = TRangeOfTypeNoCRef<R>> INTRA_NODISCARD constexpr forceinline Requires<
	CNonInfiniteRandomAccessRange<AsR>,
RCycleRandom<AsR>> Cycle(R&& range)
{return ForwardAsRange<R>(range);}
INTRA_CORE_RANGE_END
