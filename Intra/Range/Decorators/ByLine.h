#pragma once

#include "Cpp/Features.h"
#include "Cpp/Warnings.h"
#include "Range/ForwardDecls.h"
#include "Concepts/Range.h"
#include "Concepts/RangeOf.h"
#include "Utils/Op.h"
#include "Range/Output/OutputArrayRange.h"
#include "Utils/StringView.h"
#include "Range/Output/Inserter.h"
#include "Range/Mutation/CopyUntil.h"
#include "Container/ForwardDecls.h"

namespace Intra { namespace Range {

INTRA_WARNING_PUSH
INTRA_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED

template<class R, class C> struct RByLine
{
	enum: bool
	{
		RangeIsFinite = Concepts::IsFiniteRange<R>::_,
		RangeIsInfinite = Concepts::IsInfiniteRange<R>::_
	};

	forceinline RByLine(null_t=null): mKeepTerminator(false) {}

	forceinline RByLine(R&& range, bool keepTerminator=false):
		mOriginalRange(Cpp::Move(range)), mKeepTerminator(keepTerminator) {PopFirst();}

	forceinline RByLine(const R& range, bool keepTerminator=false):
		mOriginalRange(range), mKeepTerminator(keepTerminator) {PopFirst();}

	forceinline bool Empty() const {return mOriginalRange.Empty() && mFirst.empty();}

	void PopFirst()
	{
		mFirst.clear();
		ReadToUntil(mOriginalRange, LastAppender(mFirst), Op::IsLineSeparator<Concepts::ValueTypeOf<R>>);
		if(mOriginalRange.Empty()) return;

		const auto nextChar = mOriginalRange.First();
		if(mKeepTerminator) mFirst.push_back(nextChar);
		const bool CR = nextChar == '\r';
		mOriginalRange.PopFirst();

		if(!CR || mOriginalRange.Empty() || mOriginalRange.First() != '\n') return;

		if(mKeepTerminator) mFirst.push_back('\n');
		mOriginalRange.PopFirst();
	}

	forceinline const C& First() const
	{
		INTRA_DEBUG_ASSERT(!Empty());
		return mFirst;
	}

private:
	R mOriginalRange;
	C mFirst;
	bool mKeepTerminator;
};


template<typename R, typename AsR=Concepts::RangeOfTypeNoCRef<R>> forceinline Meta::EnableIf<
	Concepts::IsForwardCharRange<AsR>::_,
RByLine<AsR, String>> ByLine(R&& range, Tags::TKeepTerminator)
{return {Range::Forward<R>(range), true};}

template<typename R, typename AsR=Concepts::RangeOfTypeNoCRef<R>> forceinline Meta::EnableIf<
	Concepts::IsForwardCharRange<AsR>::_,
RByLine<AsR, String>> ByLine(R&& range)
{return {Range::Forward<R>(range), false};}

INTRA_WARNING_POP

}}
