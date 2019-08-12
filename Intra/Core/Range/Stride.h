#pragma once

#include "Core/Assert.h"
#include "Core/Range/Concepts.h"
#include "Core/Range/Operations.h"

INTRA_CORE_RANGE_BEGIN
INTRA_WARNING_DISABLE_ASSIGN_IMPLICITLY_DELETED
template<typename R> struct RStride
{
	enum: bool
	{
		RangeIsFinite = CFiniteRange<R>,
		RangeIsInfinite = CInfiniteRange<R>
	};

	forceinline RStride() = default;

	INTRA_CONSTEXPR2 forceinline RStride(R range, size_t strideStep):
		mOriginalRange(Move(range)), mStep(strideStep)
	{
		INTRA_DEBUG_ASSERT(strideStep != 0);
		skip_back_odd();
	}

	INTRA_NODISCARD constexpr forceinline bool Empty() const {return mOriginalRange.Empty();}
	INTRA_NODISCARD constexpr forceinline decltype(auto) First() const {return mOriginalRange.First();}
	INTRA_CONSTEXPR2 forceinline void PopFirst() {PopFirstN(mOriginalRange, mStep);}

	INTRA_NODISCARD constexpr forceinline decltype(auto) Last() const {return mOriginalRange.Last();}

	INTRA_CONSTEXPR2 forceinline void PopLast() {PopLastN(mOriginalRange, mStep);}

	INTRA_NODISCARD constexpr forceinline decltype(auto) operator[](size_t index) const {return mOriginalRange[index*mStep];}

	template<typename U=R> INTRA_NODISCARD forceinline TSliceTypeOf<U> operator()(size_t start, size_t end) const
	{return mOriginalRange(start*mStep, end*mStep);}

	INTRA_NODISCARD forceinline index_t Length() const {return (mOriginalRange.Length() + mStep - 1) / mStep;}

	INTRA_NODISCARD constexpr forceinline RStride Stride(size_t strideStep) const
	{return RStride(mOriginalRange, mStep*strideStep);}

private:
	template<typename U=R, typename = Requires<
		CHasPopLast<U> && CHasLength<U>
	>> INTRA_CONSTEXPR2 forceinline void skip_back_odd()
	{
		const size_t len = mOriginalRange.Length();
		if(len == 0) return;
		PopLastExactly(mOriginalRange, (len-1) % mStep);
	}

	template<typename U=R> INTRA_CONSTEXPR2 forceinline Requires<
		!(CHasPopLast<U> && CHasLength<U>)
	> skip_back_odd() {}

	R mOriginalRange;
	size_t mStep = 0;
};

template<typename R, typename AsR = TRangeOfTypeNoCRef<R>> INTRA_NODISCARD constexpr forceinline Requires<
	CAccessibleRange<AsR>,
RStride<AsR>> Stride(R&& range, size_t step)
{return {ForwardAsRange<R>(range), step};}
INTRA_CORE_RANGE_END
