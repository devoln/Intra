#pragma once

#include "Range/ForwardDecls.h"
#include "Range/AsRange.h"

namespace Intra { namespace Range {

INTRA_WARNING_PUSH
INTRA_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED

template<typename Rs> struct RFirstTransversal
{
	enum: bool {RangeIsFinite = IsFiniteRange<Rs>::_};
	typedef ReturnValueTypeOf<ValueTypeOf<Rs>> return_value_type;

	forceinline RFirstTransversal(null_t=null) {}

	forceinline RFirstTransversal(Rs&& rangeOfRanges):
		mRanges(Meta::Move(rangeOfRanges)) {skip_empty();}

	forceinline RFirstTransversal(const Rs& rangeOfRanges):
		mRanges(rangeOfRanges) {skip_empty();}

	forceinline bool Empty() const {return mRanges.Empty();}

	forceinline return_value_type First() const {return mRanges.First().First();}
	forceinline ReturnValueTypeOf<Rs> FirstRange() const {return mRanges.First();}

	forceinline void PopFirst()
	{
		mRanges.PopFirst();
		skip_empty();
	}

	template<typename U=Rs> forceinline Meta::EnableIf<
		HasLast<U>::_ && HasPopLast<U>::_,
	return_value_type> Last() const
	{
		while(mRanges.Last().Empty()) mRanges.PopLast();
		return mRanges.Last().First();
	}

	template<typename U=Rs> forceinline Meta::EnableIf<
		HasLast<U>::_ && HasPopLast<Rs>::_
	> PopLast()
	{
		INTRA_ASSERT(!Empty());
		while(mRanges.Last().Empty()) mRanges.PopLast();
		mRanges.PopLast();
	}

	forceinline bool operator==(const RFirstTransversal& rhs) const {return mRanges==rhs.mRanges;}

private:
	Rs mRanges;

	void skip_empty() {while(!Empty() && mRanges.First().Empty()) mRanges.PopFirst();}
};

INTRA_WARNING_POP

template<typename R> forceinline Meta::EnableIf<
	!Meta::IsReference<R>::_ && !Meta::IsConst<R>::_ &&
	IsInputRange<R>::_ && IsInputRange<ValueTypeOf<R>>::_,
RFirstTransversal<R>> FirstTransversal(R&& range)
{return Meta::Move(range);}

template<typename R> forceinline Meta::EnableIf<
	IsForwardRange<R>::_ && IsInputRange<ValueTypeOf<R>>::_,
RFirstTransversal<R>> FirstTransversal(const R& range)
{return range;}

template<typename T, size_t N> forceinline Meta::EnableIf<
	IsInputRange<T>::_,
RFirstTransversal<AsRangeResult<T(&)[N]>>> FirstTransversal(T(&rangeArr)[N])
{return AsRange(rangeArr);}

}}
