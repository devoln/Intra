#pragma once

#include "Intra/Operations.h"
#include "Intra/Functional.h"
#include "Intra/Container/Tuple.h"

#include "Intra/Range/Concepts.h"
#include "Intra/Range/Take.h"
#include "Intra/Range/TakeUntil.h"

INTRA_BEGIN
INTRA_IGNORE_WARNING_COPY_IMPLICITLY_DELETED
template<typename R, typename IsSkippedDelimiter, typename IsElementDelimiter> class RSplit:
	private TWrapper<IsSkippedDelimiter>, TWrapper<IsElementDelimiter, 1>
{
	R mOriginalRange;
	TTakeResult<R> mFirst;
public:
	enum: bool
	{
		IsAnyInstanceFinite = CFiniteRange<R>,
		IsAnyInstanceInfinite = CInfiniteRange<R>
	};

	RSplit() = default;

	constexpr RSplit(R range, IsSkippedDelimiter isSkippedDelimiter, IsElementDelimiter isElementDelimiter):
		TWrapper<IsSkippedDelimiter>(Move(isSkippedDelimiter)),
		TWrapper<IsElementDelimiter, 1>(Move(isElementDelimiter)),
		mOriginalRange(Move(range)) {PopFirst();}

	[[nodiscard]] constexpr bool Empty() const {return mFirst.Empty();}

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

	[[nodiscard]] constexpr TTakeResult<R> First() const
	{
		INTRA_PRECONDITION(!Empty());
		return mFirst;
	}
};


template<typename R, typename P1, typename P2 = TAlwaysFalse,
	typename AsR = TRangeOfRef<R>,
	typename T = TValueTypeOf<AsR>
> [[nodiscard]] constexpr Requires<
	CForwardRange<AsR> &&
	CCallable<P1, T> &&
	CCallable<P2, T>,
RSplit<TRemoveConstRef<AsR>, TRemoveConstRef<P1>, TRemoveConstRef<P2>>> Split(
	R&& range, P1&& isSkippedDelimiter, P2&& isElementDelimiter = AlwaysFalse)
{return {ForwardAsRange<R>(range), ForwardAsFunc<P1>(isSkippedDelimiter), ForwardAsFunc<P2>(isElementDelimiter)};}

template<typename R,
	typename AsR = TRangeOfRef<R>,
	typename T = TValueTypeOf<AsR>
> [[nodiscard]] constexpr Requires<
	CForwardRange<AsR>,
RSplit<TRemoveConstRef<AsR>, TIsLineSeparator, TAlwaysFalse>> SplitLines(R&& range)
{return Split(Forward<R>(range), IsLineSeparator, AlwaysFalse);}
INTRA_END
