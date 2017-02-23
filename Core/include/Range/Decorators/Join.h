#pragma once

#include "Range/Concepts.h"

namespace Intra { namespace Range {

template<typename RR> class RJoin
{
	typedef Range::ReturnValueTypeOfAs<Range::ValueTypeOf<RR>> ReturnValueType;
	typedef Range::ValueTypeOfAs<RR> R;
public:

	RJoin(const RR& range): mOriginalRanges(range) {goToNearestNonEmptyElement();}
	RJoin(RR&& range): mOriginalRanges(Meta::Move(range)) {goToNearestNonEmptyElement();}

	ReturnValueType First() const
	{
		INTRA_ASSERT(!Empty());
		return mCurrentRange.First();
	}

	bool Empty() const {return mCurrentRange.Empty();}

	void PopFirst()
	{
		INTRA_ASSERT(!mCurrentRange.Empty());
		mCurrentRange.PopFirst();
		goToNearestNonEmptyElement();
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

template<typename RR> RJoin<AsRangeResultNoCRef<RR>> Join(RR&& range)
{return {Range::Forward<RR>(range)};}

}}
