#pragma once

#include "Core/Functional.h"
#include "Core/Range/Concepts.h"
#include "Core/Range/Take.h"
#include "Core/Range/Search/Single.h"

INTRA_BEGIN
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED
template<typename R> struct RTakeByLine
{
	enum: bool
	{
		RangeIsFinite = CFiniteRange<R>,
		RangeIsInfinite = CInfiniteRange<R>
	};

	forceinline RTakeByLine() = default;

	constexpr forceinline RTakeByLine(R&& range, bool keepTerminator = false):
		mOriginalRange(Move(range)), mKeepTerminator(keepTerminator) {PopFirst();}

	constexpr forceinline RTakeByLine(const R& range, bool keepTerminator = false):
		mOriginalRange(range), mKeepTerminator(keepTerminator) {PopFirst();}

	INTRA_NODISCARD constexpr forceinline bool Empty() const noexcept {return mFirst.Empty();}

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

	INTRA_NODISCARD constexpr forceinline TTakeResult<R> First() const
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
	typename AsR = TRangeOfTypeNoCRef<R>
> INTRA_NODISCARD constexpr forceinline Requires<
	CForwardCharRange<AsR>,
RTakeByLine<AsR>> TakeByLine(R&& range, bool keepTerminator = false)
{return {ForwardAsRange<R>(range), keepTerminator};}
INTRA_END
