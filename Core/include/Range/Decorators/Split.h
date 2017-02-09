#pragma once

#include "Platform/CppFeatures.h"
#include "Platform/CppWarnings.h"
#include "Meta/Operators.h"
#include "Range/ForwardDecls.h"
#include "Range/Concepts.h"
#include "Range/AsRange.h"
#include "Utils/Optional.h"
#include "Range/Decorators/Take.h"
#include "Range/Decorators/TakeUntil.h"

namespace Intra { namespace Range {

INTRA_WARNING_PUSH
INTRA_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED

template<typename R, typename P1, typename P2> struct RSplit
{
	enum: bool {RangeIsFinite = IsFiniteRange<R>::_, RangeIsInfinite = IsInfiniteRange<R>::_};

	forceinline RSplit(null_t=null):
		mOriginalRange(null), mIsSkippedDelimiter(), mIsElementDelimiter() {}

	forceinline RSplit(R&& range, P1 isSkippedDelimiter, P2 isElementDelimiter):
		mOriginalRange(Meta::Move(range)), mIsSkippedDelimiter(isSkippedDelimiter),
		mIsElementDelimiter(isElementDelimiter) {PopFirst();}

	forceinline RSplit(const R& range, P1 isSkippedDelimiter, P2 isElementDelimiter):
		mOriginalRange(range), mIsSkippedDelimiter(isSkippedDelimiter),
		mIsElementDelimiter(isElementDelimiter) {PopFirst();}

	forceinline bool Empty() const {return mFirst.Empty();}

	void PopFirst()
	{
		if(mIsSkippedDelimiter!=null)
			while(!mOriginalRange.Empty() &&
				mIsSkippedDelimiter()(mOriginalRange.First()))
					mOriginalRange.PopFirst();
		if(mOriginalRange.Empty())
		{
			mFirst = Take(mOriginalRange, 0);
			return;
		}
		if(mIsElementDelimiter!=null && mIsElementDelimiter()(mOriginalRange.First()))
		{
			mFirst = Take(mOriginalRange, 1);
			mOriginalRange.PopFirst();
			return;
		}
		mFirst = Range::TakeUntilAdvance(mOriginalRange, [this](const ValueTypeOf<R>& v)
		{
			return (mIsSkippedDelimiter!=null && mIsSkippedDelimiter()(v)) ||
				(mIsElementDelimiter!=null && mIsElementDelimiter()(v));
		});
	}

	forceinline ResultOfTake<R> First() const
	{
		INTRA_ASSERT(!Empty());
		return mFirst;
	}

	bool operator==(const RSplit& rhs) const
	{return mOriginalRange==rhs.mOriginalRange;}

private:
	R mOriginalRange;
	Utils::Optional<P1> mIsSkippedDelimiter;
	Utils::Optional<P2> mIsElementDelimiter;
	ResultOfTake<R> mFirst;
};


template<typename R, typename P1, typename P2 = bool(*)(const ValueTypeOfAs<R>&)> forceinline Meta::EnableIf<
	IsAsForwardRange<R>::_ &&
	Meta::IsCallable<P1, ValueTypeOfAs<R>>::_ &&
	Meta::IsCallable<P2, ValueTypeOfAs<R>>::_,
RSplit<Meta::RemoveConstRef<AsRangeResult<R>>, Meta::RemoveConstRef<P1>, Meta::RemoveConstRef<P2>>> Split(
	R&& range, P1&& isSkippedDelimiter, P2&& isElementDelimiter=&Op::FalsePredicate<ValueTypeOfAs<R>>)
{return {Range::Forward<R>(range), Meta::Forward<P1>(isSkippedDelimiter), Meta::Forward<P2>(isElementDelimiter)};}

INTRA_WARNING_POP

}}
