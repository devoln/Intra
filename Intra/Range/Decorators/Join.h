#pragma once

#include "Concepts/Range.h"
#include "Cpp/Warnings.h"
#include "Cpp/Features.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED

namespace Intra { namespace Range {

template<typename RR> class RJoin
{
	typedef Concepts::ReturnValueTypeOfAs<Concepts::ValueTypeOf<RR>> ReturnValueType;
	typedef Concepts::RangeOfType<Concepts::ValueTypeOf<RR>> R;
public:
	RJoin(const RR& range): mOriginalRanges(range) {goToNearestNonEmptyElement();}
	RJoin(RR&& range): mOriginalRanges(Cpp::Move(range)) {goToNearestNonEmptyElement();}

	forceinline ReturnValueType First() const
	{
		INTRA_DEBUG_ASSERT(!Empty());
		return mCurrentRange.First();
	}

	forceinline bool Empty() const {return mCurrentRange.Empty();}

	void PopFirst()
	{
		INTRA_DEBUG_ASSERT(!mCurrentRange.Empty());
		mCurrentRange.PopFirst();
		goToNearestNonEmptyElement();
	}

	template<typename U=RR> Meta::EnableIf<
		Concepts::HasIndex<R>::_ &&
		Concepts::HasLength<R>::_ &&
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
		Concepts::HasLength<R>::_ &&
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

template<typename RR,
	typename AsRR = Concepts::RangeOfTypeNoCRef<RR>
> forceinline Meta::EnableIf<
	Concepts::IsConsumableRange<AsRR>::_ &&
	Concepts::IsAsConsumableRange<Concepts::ValueTypeOf<AsRR>>::_,
RJoin<AsRR>> Join(RR&& range)
{return {Range::Forward<RR>(range)};}

}}

INTRA_WARNING_POP
