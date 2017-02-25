#pragma once

#include "Range/Concepts.h"

namespace Intra { namespace Range {

template<typename RR> class RJoin
{
	typedef ReturnValueTypeOfAs<ValueTypeOf<RR>> ReturnValueType;
	typedef AsRangeResult<ValueTypeOf<RR>> R;
public:
	RJoin(const RR& range): mOriginalRanges(range) {goToNearestNonEmptyElement();}
	RJoin(RR&& range): mOriginalRanges(Meta::Move(range)) {goToNearestNonEmptyElement();}
	RJoin(const RJoin& rhs) = default;

	forceinline ReturnValueType First() const
	{
		INTRA_ASSERT(!Empty());
		return mCurrentRange.First();
	}

	forceinline bool Empty() const {return mCurrentRange.Empty();}

	void PopFirst()
	{
		INTRA_ASSERT(!mCurrentRange.Empty());
		mCurrentRange.PopFirst();
		goToNearestNonEmptyElement();
	}

	template<typename U=RR> Meta::EnableIf<
		HasIndex<R>::_ && HasLength<R>::_ &&
		Meta::IsCopyConstructible<U>::_,
	ReturnValueType> operator[](size_t index) const
	{
		RR rr = mOriginalRanges;
		while(!rr.Empty() && rr.First().Length()<=index)
		{
			index -= rr.First().Length();
			rr.PopFirst();
		}
		return rr.First()[index];
	}

	template<typename U=RR> Meta::EnableIf<
		HasLength<R>::_ &&
		Meta::IsCopyConstructible<U>::_,
	size_t> Length() const
	{
		size_t result = 0;
		for(RR rr=mOriginalRanges; !rr.Empty(); rr.PopFirst())
			result += rr.First().Length();
		return result;
	}

private:
	RR mOriginalRanges;
	R mCurrentRange;

	void goToNearestNonEmptyElement()
	{
		while(mCurrentRange.Empty() && !mOriginalRanges.Empty())
		{
			mCurrentRange = AsRange(mOriginalRanges.First());
			mOriginalRanges.PopFirst();
		}
	}
};

template<typename RR, typename AsRR=AsRangeResultNoCRef<RR>> forceinline Meta::EnableIf<
	IsConsumableRange<AsRR>::_ &&
	IsAsConsumableRange<ValueTypeOf<AsRR>>::_,
RJoin<AsRR>> Join(RR&& range)
{return {Range::Forward<RR>(range)};}

}}
