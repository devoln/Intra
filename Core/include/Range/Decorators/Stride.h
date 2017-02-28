#pragma once

#include "Platform/CppWarnings.h"
#include "Platform/CppFeatures.h"
#include "Platform/Debug.h"
#include "Range/ForwardDecls.h"
#include "Range/Concepts.h"
#include "Range/AsRange.h"
#include "Range/Operations.h"

namespace Intra { namespace Range {

INTRA_WARNING_PUSH
INTRA_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED

template<typename R> struct RStride
{
	enum: bool {RangeIsFinite = IsFiniteRange<R>::_, RangeIsInfinite = IsInfiniteRange<R>::_};

	forceinline RStride(null_t=null): mOriginalRange(null), mStep(0) {}

	forceinline RStride(const R& range, size_t strideStep):
		mOriginalRange(range), mStep(strideStep)
	{
		INTRA_DEBUG_ASSERT(strideStep!=0);
		skip_back_odd();
	}

	forceinline RStride(R&& range, size_t strideStep):
		mOriginalRange(Meta::Forward<R>(range)), mStep(strideStep)
	{
		INTRA_DEBUG_ASSERT(strideStep!=0);
		skip_back_odd();
	}

	forceinline bool Empty() const {return mOriginalRange.Empty();}


	forceinline ReturnValueTypeOf<R> First() const {return mOriginalRange.First();}
	forceinline void PopFirst() {PopFirstN(mOriginalRange, mStep);}

	forceinline ReturnValueTypeOf<R> Last() const {return mOriginalRange.Last();}

	forceinline void PopLast() {PopLastN(mOriginalRange, mStep);}

	forceinline ReturnValueTypeOf<R> operator[](size_t index) const {return mOriginalRange[index*mStep];}

	template<typename U=R> forceinline SliceTypeOf<U> operator()(size_t start, size_t end) const
	{return mOriginalRange(start*mStep, end*mStep);}

	forceinline bool operator==(const RStride& rhs) const
	{return mStep==rhs.mStep && (mStep==0 || mOriginalRange==rhs.mOriginalRange);}

	forceinline size_t Length() const {return (mOriginalRange.Length()+mStep-1)/mStep;}

	RStride Stride(size_t strideStep) const
	{return RStride(mOriginalRange, mStep*strideStep);}

private:
	template<typename U=R, typename = Meta::EnableIf<
		HasPopLast<U>::_ && HasLength<U>::_
	>> forceinline void skip_back_odd()
	{
		size_t len = mOriginalRange.Length();
		if(len==0) return;
		PopLastExactly(mOriginalRange, (len-1) % mStep);
	}

	template<typename U=R> forceinline Meta::EnableIf<
		!(HasPopLast<U>::_ && HasLength<U>::_)
	> skip_back_odd() {}

	R mOriginalRange;
	size_t mStep;
};


template<typename R, typename AsR=AsRangeResultNoCRef<R>> forceinline Meta::EnableIf<
	IsAccessibleRange<AsR>::_,
RStride<AsR>> Stride(R&& range, size_t step)
{return {Range::Forward<R>(range), step};}

INTRA_WARNING_POP

}}
