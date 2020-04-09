#pragma once

#include "Intra/Functional.h"
#include "Intra/Range/Concepts.h"
#include "Intra/Range/Take.h"
#include "Intra/Range/Search/Single.h"

INTRA_BEGIN
INTRA_IGNORE_WARNING_COPY_IMPLICITLY_DELETED
template<typename R> struct RTakeByLine
{
	enum: bool
	{
		IsAnyInstanceFinite = CFiniteRange<R>,
		IsAnyInstanceInfinite = CInfiniteRange<R>
	};

	INTRA_FORCEINLINE RTakeByLine() = default;

	constexpr RTakeByLine(R&& range, bool keepTerminator = false):
		mOriginalRange(Move(range)), mKeepTerminator(keepTerminator) {PopFirst();}

	constexpr RTakeByLine(const R& range, bool keepTerminator = false):
		mOriginalRange(range), mKeepTerminator(keepTerminator) {PopFirst();}

	[[nodiscard]] constexpr bool Empty() const noexcept {return mFirst.Empty();}

	constexpr void PopFirst()
	{
		const auto rangeCopy = mOriginalRange;
		size_t lineLength = CountUntilAdvance(mOriginalRange, IsLineSeparator);
		if(!mOriginalRange.Empty())
		{
			const bool CR = mOriginalRange.First() == '\r';
			mOriginalRange.PopFirst();
			if(mKeepTerminator) lineLength++;
			if(CR && !mOriginalRange.Empty() && mOriginalRange.First() == '\n')
			{
				mOriginalRange.PopFirst();
				if(mKeepTerminator) lineLength++;
			}
		}
		mFirst = Take(rangeCopy, lineLength);
	}

	[[nodiscard]] constexpr TTakeResult<R> First() const
	{
		INTRA_PRECONDITION(!Empty());
		return mFirst;
	}

private:
	R mOriginalRange;
	TTakeResult<R> mFirst;
	bool mKeepTerminator = false;
};


template<typename R,
	typename AsR = TRangeOf<R>
> [[nodiscard]] constexpr Requires<
	CForwardCharRange<AsR>,
RTakeByLine<AsR>> TakeByLine(R&& range, bool keepTerminator = false)
{return {ForwardAsRange<R>(range), keepTerminator};}
INTRA_END
