#pragma once

#include "Core/Operations.h"
#include "Core/Range/Concepts.h"

#include "Core/Functional.h"


#include "Core/Range/Take.h"
#include "Core/Range/TakeUntil.h"

INTRA_CORE_RANGE_BEGIN
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED
template<typename R, typename IsSkippedDelimiter, typename IsElementDelimiter> struct RSplit:
	private IsSkippedDelimiter, private IsElementDelimiter
{
	enum: bool
	{
		RangeIsFinite = CFiniteRange<R>,
		RangeIsInfinite = CInfiniteRange<R>
	};

	forceinline RSplit() = default;

	constexpr forceinline RSplit(R range, P1 isSkippedDelimiter, P2 isElementDelimiter):
		IsSkippedDelimiter(isSkippedDelimiter),
		IsElementDelimiter(isElementDelimiter),
		mOriginalRange(Move(range)) {PopFirst();}

	INTRA_NODISCARD constexpr forceinline bool Empty() const {return mFirst.Empty();}

	INTRA_CONSTEXPR2 void PopFirst()
	{
		while(!mOriginalRange.Empty() &&
			IsSkippedDelimiter::operator()(mOriginalRange.First()))
				mOriginalRange.PopFirst();
		if(mOriginalRange.Empty())
		{
			mFirst = Take(mOriginalRange, 0);
			return;
		}
		if(IsElementDelimiter::operator()(mOriginalRange.First()))
		{
			mFirst = Take(mOriginalRange, 1);
			mOriginalRange.PopFirst();
			return;
		}
		mFirst = TakeUntilAdvance(mOriginalRange, [this](const TValueTypeOf<R>& v)
		{
			return IsSkippedDelimiter::operator()(v) ||
				IsElementDelimiter::operator()(v));
		});
	}

	INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline TTakeResult<R> First() const
	{
		INTRA_DEBUG_ASSERT(!Empty());
		return mFirst;
	}

private:
	R mOriginalRange;
	RTakeResult<R> mFirst;
};


template<typename R, typename P1, typename P2 = TFFalse,
	typename AsR = TRangeOfType<R>,
	typename T = TValueTypeOf<AsR>
> INTRA_NODISCARD constexpr forceinline Requires<
	CForwardRange<AsR> &&
	CCallable<P1, T> &&
	CCallable<P2, T>,
RSplit<TRemoveConstRef<AsR>, TRemoveConstRef<P1>, TRemoveConstRef<P2>>> Split(
	R&& range, P1&& isSkippedDelimiter, P2&& isElementDelimiter = FFalse)
{return {ForwardAsRange<R>(range), Forward<P1>(isSkippedDelimiter), ForwardAsFunc<P2>(isElementDelimiter)};}

template<typename R,
	typename AsR = TRangeOfType<R>,
	typename T = TValueTypeOf<AsR>
> INTRA_NODISCARD constexpr forceinline Requires<
	CForwardRange<AsR>,
RSplit<TRemoveConstRef<T>, TIsLineSeparator, TFFalse>> SplitLines(R&& range)
{return Split(Forward<R>(range), IsLineSeparator, FFalse);}
INTRA_CORE_RANGE_END
