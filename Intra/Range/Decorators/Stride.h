#pragma once

#include "Cpp/Warnings.h"
#include "Cpp/Features.h"
#include "Utils/Debug.h"
#include "Range/ForwardDecls.h"
#include "Concepts/Range.h"
#include "Concepts/RangeOf.h"
#include "Range/Operations.h"

namespace Intra { namespace Range {

INTRA_WARNING_PUSH
INTRA_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED

template<typename R> struct RStride
{
	enum: bool
	{
		RangeIsFinite = Concepts::IsFiniteRange<R>::_,
		RangeIsInfinite = Concepts::IsInfiniteRange<R>::_
	};

	forceinline RStride(null_t=null): mOriginalRange(null), mStep(0) {}

	forceinline RStride(const R& range, size_t strideStep):
		mOriginalRange(range), mStep(strideStep)
	{
		INTRA_DEBUG_ASSERT(strideStep!=0);
		skip_back_odd();
	}

	forceinline RStride(R&& range, size_t strideStep):
		mOriginalRange(Cpp::Forward<R>(range)), mStep(strideStep)
	{
		INTRA_DEBUG_ASSERT(strideStep!=0);
		skip_back_odd();
	}

	forceinline bool Empty() const {return mOriginalRange.Empty();}


	forceinline Concepts::ReturnValueTypeOf<R> First() const {return mOriginalRange.First();}
	forceinline void PopFirst() {PopFirstN(mOriginalRange, mStep);}

	forceinline Concepts::ReturnValueTypeOf<R> Last() const {return mOriginalRange.Last();}

	forceinline void PopLast() {PopLastN(mOriginalRange, mStep);}

	forceinline Concepts::ReturnValueTypeOf<R> operator[](size_t index) const {return mOriginalRange[index*mStep];}

	template<typename U=R> forceinline Concepts::SliceTypeOf<U> operator()(size_t start, size_t end) const
	{return mOriginalRange(start*mStep, end*mStep);}

	forceinline bool operator==(const RStride& rhs) const
	{return mStep==rhs.mStep && (mStep==0 || mOriginalRange==rhs.mOriginalRange);}

	forceinline size_t Length() const {return (mOriginalRange.Length()+mStep-1)/mStep;}

	RStride Stride(size_t strideStep) const
	{return RStride(mOriginalRange, mStep*strideStep);}

private:
	template<typename U=R, typename = Meta::EnableIf<
		Concepts::HasPopLast<U>::_ &&
		Concepts::HasLength<U>::_
	>> forceinline void skip_back_odd()
	{
		size_t len = mOriginalRange.Length();
		if(len==0) return;
		PopLastExactly(mOriginalRange, (len-1) % mStep);
	}

	template<typename U=R> forceinline Meta::EnableIf<
		!(Concepts::HasPopLast<U>::_ &&
			Concepts::HasLength<U>::_)
	> skip_back_odd() {}

	R mOriginalRange;
	size_t mStep;
};


template<typename R, typename AsR= Concepts::RangeOfTypeNoCRef<R>> forceinline Meta::EnableIf<
	Concepts::IsAccessibleRange<AsR>::_,
RStride<AsR>> Stride(R&& range, size_t step)
{return {Range::Forward<R>(range), step};}

INTRA_WARNING_POP

}}
