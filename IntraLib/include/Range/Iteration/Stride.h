#pragma once
#include "Range/Mixins/RangeMixins.h"

namespace Intra { namespace Range
{

template<typename R> struct StrideResult:
	RangeMixin<StrideResult<R>, typename R::value_type,
	R::RangeType!=TypeEnum::Bidirectional || HasLength<R>::_? R::RangeType: TypeEnum::Forward, R::RangeIsFinite>
{
	typedef typename R::value_type value_type;
	typedef typename R::value_type return_value_type;

	forceinline StrideResult(null_t=null): original_range(null), step(0) {}
	forceinline StrideResult(const R& range, size_t strideStep):
		original_range(range), step(strideStep) {INTRA_ASSERT(strideStep!=0); skip_back_odd();}

	template<typename U=R> forceinline Meta::EnableIf<
		HasLength<U>::_ || !U::RangeIsFinite,
		bool> Empty() const {return original_range.Empty();}

	template<typename U=R> forceinline Meta::EnableIf<
		!HasLength<U>::_ && U::RangeIsFinite,
		bool> Empty() const {return original_range.Empty();}


	forceinline return_value_type First() const {return original_range.First();}
	forceinline void PopFirst() {original_range.PopFirstN(step);}

	template<typename U=R> forceinline Meta::EnableIf<
		U::RangeType>=TypeEnum::Bidirectional && HasLength<U>::_,
	return_value_type> Last() const {return original_range.Last();}

	template<typename U=R> forceinline Meta::EnableIf<
		U::RangeType>=TypeEnum::Bidirectional && HasLength<U>::_
	> PopLast() {original_range.PopLastN(step);}

	template<typename U=R> forceinline Meta::EnableIf<
		U::RangeType==TypeEnum::RandomAccess,
	return_value_type> operator[](size_t index) const {return original_range[index*step];}

	template<typename U=R> forceinline Meta::EnableIf<
		U::RangeType==TypeEnum::RandomAccess,
	return_value_type> opSlice(size_t start, size_t end) const {return original_range.opSlice(start*step, end*step);}

	forceinline bool operator==(const StrideResult& rhs) const
	{
		return step==rhs.step && (step==0 || original_range==rhs.original_range);
	}

	template<typename U=R> forceinline Meta::EnableIf<
		HasLength<U>::_ || !U::RangeIsFinite,
	size_t> Length() const {return (original_range.Length()+step-1)/step;}

	StrideResult Stride(size_t strideStep) const
	{
		return StrideResult(original_range, step*strideStep);
	}

private:
	template<typename U=R> forceinline Meta::EnableIf<
		U::RangeType>=TypeEnum::Bidirectional && HasLength<U>::_
	> skip_back_odd()
	{
		size_t len = original_range.Length();
		if(len==0) return;
		original_range.PopLastExactly((len-1) % step);
	}

	template<typename U=R> forceinline Meta::EnableIf<
		U::RangeType<TypeEnum::Bidirectional || !HasLength<U>::_
	> skip_back_odd() {}

	R original_range;
	size_t step;
};

template<typename R> forceinline Meta::EnableIf<
	!Meta::IsReference<R>::_ && IsInputRange<R>::_,
StrideResult<R>> Stride(R&& range, size_t step)
{return StrideResult<Meta::RemoveReference<R>>(core::move(range), step);}

template<typename R> forceinline Meta::EnableIf<
	IsForwardRange<R>::_,
StrideResult<R>> Stride(const R& range, size_t step) {return StrideResult<R>(range, step);}

}}
