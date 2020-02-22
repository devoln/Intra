#pragma once

#include "Core/Range/Concepts.h"

INTRA_BEGIN
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED
template<typename RR> class RJoin
{
	typedef TReturnValueTypeOfAs<TValueTypeOf<RR>> ReturnValueType;
	typedef TRangeOfType<TValueTypeOf<RR>> R;
public:
	constexpr forceinline RJoin(const RR& range): mOriginalRanges(range) {goToNearestNonEmptyElement();}
	constexpr forceinline RJoin(RR&& range): mOriginalRanges(Move(range)) {goToNearestNonEmptyElement();}

	constexpr forceinline ReturnValueType First() const
	{
		INTRA_PRECONDITION(!Empty());
		return mCurrentRange.First();
	}

	INTRA_NODISCARD constexpr forceinline bool Empty() const {return mCurrentRange.Empty();}

	constexpr void PopFirst()
	{
		INTRA_PRECONDITION(!mCurrentRange.Empty());
		mCurrentRange.PopFirst();
		goToNearestNonEmptyElement();
	}

	template<typename U=RR> INTRA_NODISCARD constexpr Requires<
		CHasIndex<R> &&
		CHasLength<R> &&
		CCopyConstructible<U>,
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

	template<typename U = RR> INTRA_NODISCARD constexpr Requires<
		CHasLength<R> &&
		CCopyConstructible<U>,
	index_t> Length() const
	{
		size_t result = 0;
		for(RR rr=mOriginalRanges; !rr.Empty(); rr.PopFirst())
			result += rr.First().Length();
		return result;
	}

private:
	RR mOriginalRanges;
	R mCurrentRange;

	constexpr void goToNearestNonEmptyElement()
	{
		while(mCurrentRange.Empty() && !mOriginalRanges.Empty())
		{
			mCurrentRange = AsRange(mOriginalRanges.First());
			mOriginalRanges.PopFirst();
		}
	}
};

template<typename RR,
	typename AsRR = TRangeOfTypeNoCRef<RR>
> INTRA_NODISCARD constexpr forceinline Requires<
	CConsumableRange<AsRR> &&
	CAsConsumableRange<TValueTypeOf<AsRR>>,
RJoin<AsRR>> Join(RR&& range)
{return {ForwardAsRange<RR>(range)};}
INTRA_END
