#pragma once

#include "Cpp/Features.h"
#include "Cpp/Warnings.h"

#include "Meta/Operators.h"

#include "Concepts/Range.h"
#include "Concepts/RangeOf.h"

#include "Funal/Op.h"
#include "Utils/Optional.h"

#include "Range/ForwardDecls.h"
#include "Range/Decorators/Take.h"
#include "Range/Decorators/TakeUntil.h"

namespace Intra { namespace Range {

INTRA_WARNING_PUSH
INTRA_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED

template<typename R, typename P1, typename P2> struct RSplit
{
	enum: bool
	{
		RangeIsFinite = Concepts::IsFiniteRange<R>::_,
		RangeIsInfinite = Concepts::IsInfiniteRange<R>::_
	};

	forceinline RSplit(null_t=null):
		mOriginalRange(null), mIsSkippedDelimiter(), mIsElementDelimiter() {}

	forceinline RSplit(R&& range, P1 isSkippedDelimiter, P2 isElementDelimiter):
		mOriginalRange(Cpp::Move(range)), mIsSkippedDelimiter(isSkippedDelimiter),
		mIsElementDelimiter(isElementDelimiter) {PopFirst();}

	forceinline RSplit(const R& range, P1 isSkippedDelimiter, P2 isElementDelimiter):
		mOriginalRange(range), mIsSkippedDelimiter(isSkippedDelimiter),
		mIsElementDelimiter(isElementDelimiter) {PopFirst();}

	forceinline bool Empty() const {return mFirst.Empty();}

	void PopFirst()
	{
		if(mIsSkippedDelimiter != null)
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
		mFirst = TakeUntilAdvance(mOriginalRange, [this](const Concepts::ValueTypeOf<R>& v)
		{
			return (mIsSkippedDelimiter!=null && mIsSkippedDelimiter()(v)) ||
				(mIsElementDelimiter!=null && mIsElementDelimiter()(v));
		});
	}

	forceinline TakeResult<R> First() const
	{
		INTRA_DEBUG_ASSERT(!Empty());
		return mFirst;
	}

	bool operator==(const RSplit& rhs) const
	{return mOriginalRange == rhs.mOriginalRange;}

private:
	R mOriginalRange;
	Utils::Optional<P1> mIsSkippedDelimiter;
	Utils::Optional<P2> mIsElementDelimiter;
	TakeResult<R> mFirst;
};


template<typename R, typename P1, typename P2 = Funal::TFalse,
	typename AsR = Concepts::RangeOfType<R>,
	typename T = Concepts::ValueTypeOf<AsR>
> forceinline Meta::EnableIf<
	Concepts::IsForwardRange<AsR>::_ &&
	Meta::IsCallable<P1, T>::_ &&
	Meta::IsCallable<P2, T>::_,
RSplit<Meta::RemoveConstRef<AsR>, Meta::RemoveConstRef<P1>, Meta::RemoveConstRef<P2>>> Split(
	R&& range, P1&& isSkippedDelimiter, P2&& isElementDelimiter = Funal::False)
{return {Range::Forward<R>(range), Cpp::Forward<P1>(isSkippedDelimiter), Cpp::Forward<P2>(isElementDelimiter)};}

template<typename R,
	typename AsR = Concepts::RangeOfType<R>,
	typename T = Concepts::ValueTypeOf<AsR>
> forceinline Meta::EnableIf<
	Concepts::IsForwardRange<AsR>::_,
RSplit<Meta::RemoveConstRef<T>, Funal::TIsLineSeparator, Funal::TFalse>> SplitLines(R&& range)
{return Split(Cpp::Forward<R>(range), Funal::IsLineSeparator, Funal::False);}

INTRA_WARNING_POP

}}
