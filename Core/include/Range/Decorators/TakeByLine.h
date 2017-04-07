#pragma once

#include "Platform/CppFeatures.h"
#include "Platform/CppWarnings.h"
#include "Range/ForwardDecls.h"
#include "Range/Concepts.h"
#include "Range/AsRange.h"
#include "Range/Decorators/Take.h"
#include "Algo/Search/Single.h"
#include "Algo/Op.h"

namespace Intra { namespace Range {

INTRA_WARNING_PUSH
INTRA_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED

template<typename R> struct RTakeByLine
{
	enum: bool {RangeIsFinite = IsFiniteRange<R>::_, RangeIsInfinite = IsInfiniteRange<R>::_};

	forceinline RTakeByLine(null_t=null): mKeepTerminator(false) {}

	forceinline RTakeByLine(R&& range, bool keepTerminator=false):
		mOriginalRange(Meta::Move(range)), mKeepTerminator(keepTerminator) {PopFirst();}

	forceinline RTakeByLine(const R& range, bool keepTerminator=false):
		mOriginalRange(range), mKeepTerminator(keepTerminator) {PopFirst();}

	forceinline bool Empty() const {return mFirst.Empty();}

	void PopFirst()
	{
		auto rangeCopy = mOriginalRange;
		size_t lineLength = Algo::CountUntilAdvance(mOriginalRange, Op::IsLineSeparator<ValueTypeOf<R>>);
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


template<typename R, typename AsR=AsRangeResultNoCRef<R>> forceinline Meta::EnableIf<
	IsForwardCharRange<AsR>::_,
RTakeByLine<AsR>> TakeByLine(R&& range, bool keepTerminator=false)
{return {Range::Forward<R>(range), keepTerminator};}

INTRA_WARNING_POP

}}
