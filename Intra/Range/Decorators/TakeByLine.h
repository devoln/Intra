#pragma once

#include "Cpp/Features.h"
#include "Cpp/Warnings.h"

#include "Funal/Op.h"

#include "Concepts/Range.h"
#include "Concepts/RangeOf.h"

#include "Range/ForwardDecls.h"
#include "Range/Decorators/Take.h"
#include "Range/Search/Single.h"

namespace Intra { namespace Range {

INTRA_WARNING_PUSH
INTRA_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED

template<typename R> struct RTakeByLine
{
	enum: bool
	{
		RangeIsFinite = Concepts::IsFiniteRange<R>::_,
		RangeIsInfinite = Concepts::IsInfiniteRange<R>::_
	};

	forceinline RTakeByLine(null_t=null): mKeepTerminator(false) {}

	forceinline RTakeByLine(R&& range, bool keepTerminator=false):
		mOriginalRange(Cpp::Move(range)), mKeepTerminator(keepTerminator) {PopFirst();}

	forceinline RTakeByLine(const R& range, bool keepTerminator=false):
		mOriginalRange(range), mKeepTerminator(keepTerminator) {PopFirst();}

	forceinline bool Empty() const noexcept {return mFirst.Empty();}

	void PopFirst()
	{
		const auto rangeCopy = mOriginalRange;
		size_t lineLength = CountUntilAdvance(mOriginalRange, Funal::IsLineSeparator);
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

	forceinline TakeResult<R> First() const
	{
		INTRA_DEBUG_ASSERT(!Empty());
		return mFirst;
	}

private:
	R mOriginalRange;
	TakeResult<R> mFirst;
	bool mKeepTerminator;
};


template<typename R,
	typename AsR = Concepts::RangeOfTypeNoCRef<R>
> forceinline Meta::EnableIf<
	Concepts::IsForwardCharRange<AsR>::_,
RTakeByLine<AsR>> TakeByLine(R&& range, bool keepTerminator=false)
{return {Range::Forward<R>(range), keepTerminator};}

INTRA_WARNING_POP

}}
