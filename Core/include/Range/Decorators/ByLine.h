#pragma once

#include "Platform/CppFeatures.h"
#include "Platform/CppWarnings.h"
#include "Meta/Operators.h"
#include "Range/ForwardDecls.h"
#include "Range/Concepts.h"
#include "Range/AsRange.h"
#include "Utils/Optional.h"
#include "Range/Decorators/Take.h"
#include "Algo/Search/Single.h"

namespace Intra { namespace Range {

INTRA_WARNING_PUSH
INTRA_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED

template<typename R> struct RByLine
{
	enum: bool {RangeIsFinite = IsFiniteRange<R>::_, RangeIsInfinite = IsInfiniteRange<R>::_};

	forceinline RByLine(null_t=null): mKeepTerminator(false) {}

	forceinline RByLine(R&& range, bool keepTerminator=false):
		mOriginalRange(Meta::Move(range)), mKeepTerminator(keepTerminator) {PopFirst();}

	forceinline RByLine(const R& range, bool keepTerminator=false):
		mOriginalRange(range), mKeepTerminator(keepTerminator) {PopFirst();}

	forceinline bool Empty() const {return mFirst.Empty();}

	void PopFirst()
	{
		auto rangeCopy = mOriginalRange;
		size_t lineLength = Algo::CountUntilAdvance(mOriginalRange,
			[](char v) {return v=='\r' || v=='\n';});
		if(!mOriginalRange.Empty())
		{
			bool CR = mOriginalRange.First()=='\r';
			mOriginalRange.PopFirst();
			if(mKeepTerminator) lineLength++;
			if(CR && !mOriginalRange.Empty() && mOriginalRange.First()=='\n')
			{
				mOriginalRange.PopFirst();
				if(mKeepTerminator) lineLength++;
			}
		}
		mFirst = Take(mOriginalRange, lineLength);
	}

	forceinline TakeResult<R> First() const
	{
		INTRA_ASSERT(!Empty());
		return mFirst;
	}

	bool operator==(const RByLine& rhs) const
	{return mOriginalRange==rhs.mOriginalRange;}

private:
	R mOriginalRange;
	TakeResult<R> mFirst;
	bool mKeepTerminator;
};


template<typename R> forceinline Meta::EnableIf<
	IsAsForwardCharRange<R>::_,
RByLine<AsRangeResultNoCRef<R>>> ByLine(R&& range, bool keepTerminator=false)
{return {Range::Forward<R>(range), keepTerminator};}

INTRA_WARNING_POP

}}
