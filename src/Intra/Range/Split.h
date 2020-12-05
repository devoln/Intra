#pragma once

#include "Intra/Functional.h"
#include "Intra/Concepts.h"
#include "Intra/Range/Decorators.h"

INTRA_BEGIN
INTRA_IGNORE_WARN_COPY_IMPLICITLY_DELETED
template<typename R, class F1, class P2> class RSplit
{
	R mOriginalRange;
	TTakeResult<R> mFirst;
	[[no_unique_address]] F1 mSkipDelimiter;
	[[no_unique_address]] P2 mIsElementDelimiter;
public:
	using TagAnyInstanceFinite = TTag<CFiniteRange<R>>;
	using TagAnyInstanceInfinite = TTag<CInfiniteRange<R>>;

	RSplit() = default;

	constexpr RSplit(R range, F1 SkipDelimiter, P2 isElementDelimiter):
		mSkipDelimiter(Move(isSkippedDelimiter)),
		mIsElementDelimiter(Move(isElementDelimiter)),
		mOriginalRange(Move(range)) {PopFirst();}

	[[nodiscard]] constexpr bool Empty() const {return mFirst.Empty();}

	constexpr void PopFirst()
	{
		while(!mOriginalRange.Empty())
		{
			if constexpr(CCallable<decltype(mSkipDelimiter), Advance<R>>)
			{
				if(!mSkipDelimiter(Advance(mOriginalRange))) break;
			}
			else
			{
				if(!mSkipDelimiter(mOriginalRange.First())) break;
				mOriginalRange.PopFirst();
			}
		}
		if(mOriginalRange.Empty())
		{
			mFirst = mOriginalRange|Take(0);
			return;
		}
		if(mIsElementDelimiter(mOriginalRange.First()))
		{
			mFirst = mOriginalRange|Take(1);
			mOriginalRange.PopFirst();
			return;
		}
		mFirst = Advance(mOriginalRange)|TakeUntilEagerly([this](const auto& v) {
			return mSkipDelimiter(v) || mIsElementDelimiter(v);
		});
	}

	[[nodiscard]] constexpr TTakeResult<R> First() const
	{
		INTRA_PRECONDITION(!Empty());
		return mFirst;
	}
};

constexpr auto Split = []<typename F>(F&& skipDelimiter) {
	return [skipDelimiter = ForwardAsFunc<F>(skipDelimiter)]<CForwardList L>(L&& list) requires
		CCallable<F, TListValue<L>> || //skipDelimiter acts as a predicate on single L's element
		CCallable<F, TRangeOf<L>> && //skipDelimiter acts as a predicate on unconsumed range part.
		CCallable<F, Advance<TRangeOf<L>>> //If skipDelimiter finds a delimiter, it is expected to advance the range to skip it and return true.
	{
		return RSplit(ForwardAsRange<L>(list), skipDelimiter);
	};
};

template<typename R, typename P1, typename P2 = decltype(Never),
	typename AsR = TRangeOfRef<R>,
	typename T = TRangeValue<AsR>
> [[nodiscard]] constexpr Requires<
	CForwardRange<AsR> &&
	CCallable<P1, T> &&
	CCallable<P2, T>,
RSplit<TRemoveConstRef<AsR>, TRemoveConstRef<P1>, TRemoveConstRef<P2>>> Split(
	R&& range, P1&& isSkippedDelimiter, P2&& isElementDelimiter = Never)
{return {ForwardAsRange<R>(range), ForwardAsFunc<P1>(isSkippedDelimiter), ForwardAsFunc<P2>(isElementDelimiter)};}

template<typename R,
	typename AsR = TRangeOfRef<R>,
	typename T = TRangeValue<AsR>
> [[nodiscard]] constexpr Requires<
	CForwardRange<AsR>,
RSplit<TRemoveConstRef<AsR>, TIsLineSeparator, decltype(Never)>> SplitLines(R&& range)
{return Split(INTRA_FWD(range), IsLineSeparator, Never);}
INTRA_END
