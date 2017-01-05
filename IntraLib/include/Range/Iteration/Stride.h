#pragma once

#include "Range/ForwardDecls.h"
#include "Range/Concepts.h"
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
		mOriginalRange(range), mStep(strideStep) {INTRA_ASSERT(strideStep!=0); skip_back_odd();}

	template<typename U=R> forceinline Meta::EnableIf<
		HasLength<U>::_ || !U::RangeIsFinite,
		bool> Empty() const {return mOriginalRange.Empty();}

	template<typename U=R> forceinline Meta::EnableIf<
		!HasLength<U>::_ && U::RangeIsFinite,
		bool> Empty() const {return mOriginalRange.Empty();}


	forceinline ReturnValueTypeOf<R> First() const {return mOriginalRange.First();}
	forceinline void PopFirst() {PopFirstN(mOriginalRange, mStep);}

	template<typename U=R> forceinline Meta::EnableIf<
		HasLast<U>::_ && HasLength<U>::_,
	ReturnValueTypeOf<R>> Last() const {return mOriginalRange.Last();}

	template<typename U=R> forceinline Meta::EnableIf<
		HasPopLast<U>::_ && HasLength<U>::_
	> PopLast() {PopLastN(mOriginalRange, mStep);}

	template<typename U=R> forceinline Meta::EnableIf<
		HasIndex<U>::_,
	ReturnValueTypeOf<R>> operator[](size_t index) const {return mOriginalRange[index*mStep];}

	template<typename U=R> forceinline Meta::EnableIf<
		HasSlicing<U>::_,
	SliceTypeOf<R>> operator()(size_t start, size_t end) const
	{return mOriginalRange(start*mStep, end*mStep);}

	forceinline bool operator==(const RStride& rhs) const
	{return mStep==rhs.mStep && (mStep==0 || mOriginalRange==rhs.mOriginalRange);}

	template<typename U=R> forceinline Meta::EnableIf<
		HasLength<U>::_ || IsInfiniteRange<U>::_,
	size_t> Length() const {return (mOriginalRange.Length()+mStep-1)/mStep;}

	RStride Stride(size_t strideStep) const
	{return RStride(mOriginalRange, mStep*strideStep);}

private:
	template<typename U=R> forceinline Meta::EnableIf<
		HasPopLast<U>::_ && HasLength<U>::_
	> skip_back_odd()
	{
		size_t len = mOriginalRange.Length();
		if(len==0) return;
		PopLastExactly(mOriginalRange, (len-1) % mStep);
	}

	template<typename U=R> forceinline Meta::EnableIf<
		!HasPopLast<U>::_ || !HasLength<U>::_
	> skip_back_odd() {}

	R mOriginalRange;
	size_t mStep;
};

INTRA_WARNING_POP

template<typename R> forceinline Meta::EnableIf<
	!Meta::IsReference<R>::_ && IsInputRange<R>::_,
RStride<Meta::RemoveReference<R>>> Stride(R&& range, size_t step)
{return RStride<Meta::RemoveReference<R>>(Meta::Move(range), step);}

template<typename R> forceinline Meta::EnableIf<
	IsForwardRange<R>::_,
RStride<R>> Stride(const R& range, size_t step)
{return RStride<R>(range, step);}

template<typename T, size_t N> forceinline
RStride<ArrayRange<T>> Stride(T(&arr)[N], size_t step)
{return Stride(AsRange(arr), step);}

}}
