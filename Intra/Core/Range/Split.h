#pragma once

#include "Core/Operations.h"
#include "Core/Functional.h"
#include "Core/Tuple.h"

#include "Core/Range/Concepts.h"
#include "Core/Range/Take.h"
#include "Core/Range/TakeUntil.h"

INTRA_BEGIN
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED
template<typename R, typename IsSkippedDelimiter, typename IsElementDelimiter> class RSplit:
	private TWrapper<IsSkippedDelimiter>, TWrapper<IsElementDelimiter, 1>
{
	R mOriginalRange;
	TTakeResult<R> mFirst;
public:
	enum: bool
	{
		RangeIsFinite = CFiniteRange<R>,
		RangeIsInfinite = CInfiniteRange<R>
	};

	forceinline RSplit() = default;

	constexpr forceinline RSplit(R range, IsSkippedDelimiter isSkippedDelimiter, IsElementDelimiter isElementDelimiter):
		TWrapper<IsSkippedDelimiter>(Move(isSkippedDelimiter)),
		TWrapper<IsElementDelimiter, 1>(Move(isElementDelimiter)),
		mOriginalRange(Move(range)) {PopFirst();}

	INTRA_NODISCARD constexpr forceinline bool Empty() const {return mFirst.Empty();}

	constexpr void PopFirst()
	{
		while(!mOriginalRange.Empty() &&
			TWrapper<IsSkippedDelimiter>::operator()(mOriginalRange.First()))
				mOriginalRange.PopFirst();
		if(mOriginalRange.Empty())
		{
			mFirst = Take(mOriginalRange, 0);
			return;
		}
		if(TWrapper<IsElementDelimiter, 1>::operator()(mOriginalRange.First()))
		{
			mFirst = Take(mOriginalRange, 1);
			mOriginalRange.PopFirst();
			return;
		}
		mFirst = TakeUntilAdvance(mOriginalRange, [this](const TValueTypeOf<R>& v)
		{
			return TWrapper<IsSkippedDelimiter>::operator()(v) ||
				TWrapper<IsElementDelimiter, 1>::operator()(v);
		});
	}

	INTRA_NODISCARD constexpr forceinline TTakeResult<R> First() const
	{
		INTRA_PRECONDITION(!Empty());
		return mFirst;
	}
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
{return {ForwardAsRange<R>(range), ForwardAsFunc<P1>(isSkippedDelimiter), ForwardAsFunc<P2>(isElementDelimiter)};}

template<typename R,
	typename AsR = TRangeOfType<R>,
	typename T = TValueTypeOf<AsR>
> INTRA_NODISCARD constexpr forceinline Requires<
	CForwardRange<AsR>,
RSplit<TRemoveConstRef<AsR>, TIsLineSeparator, TFFalse>> SplitLines(R&& range)
{return Split(Forward<R>(range), IsLineSeparator, FFalse);}
INTRA_END
