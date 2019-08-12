#pragma once

#include "Core/Functional.h"
#include "Core/Range/StringView.h"
#include "Core/Range/Concepts.h"
#include "Core/Range/Inserter.h"
#include "Core/Range/Mutation/CopyUntil.h"
#include "Core/Range/Operations.h"
#include "Container/ForwardDecls.h"

INTRA_CORE_RANGE_BEGIN
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED
template<class R, class OB> struct RByLine
{
	static_assert(COutputBufferOf<OB, TValueTypeOf<R>>, "OB must be an output buffer of range element!");
	enum: bool
	{
		RangeIsFinite = CFiniteRange<R>,
		RangeIsInfinite = CInfiniteRange<R>
	};

	constexpr forceinline RByLine(null_t=null): mKeepTerminator(false) {}

	INTRA_CONSTEXPR2 forceinline RByLine(R range, bool keepTerminator=false):
		mOriginalRange(Move(range)), mKeepTerminator(keepTerminator) {PopFirst();}

	INTRA_NODISCARD constexpr forceinline bool Empty() const {return mOriginalRange.Empty() && mOutputBuffer.WrittenRange().Empty();}

	INTRA_CONSTEXPR2 void PopFirst()
	{
		mOutputBuffer.Reset();
		auto mFirstResult = ReadToUntil(mOriginalRange, mOutputBuffer, IsLineSeparator);
		if(mFirstResult.StopReason == StopReason::SourceEmpty) return;

		const auto nextChar = mOriginalRange.First();
		if(mKeepTerminator) mOutputBuffer.Put(nextChar);
		const bool CR = nextChar == '\r';
		mOriginalRange.PopFirst();

		if(!CR || mOriginalRange.Empty() || mOriginalRange.First() != '\n') return;

		if(mKeepTerminator) mOutputBuffer.Put('\n');
		mOriginalRange.PopFirst();
	}

	INTRA_CONSTEXPR2 forceinline auto&& First() const
	{
		INTRA_DEBUG_ASSERT(!Empty());
		return mOutputBuffer.WrittenRange();
	}

	INTRA_NODISCARD constexpr FindResult GetFindResult() const {return mFindResult;}

private:
	R mOriginalRange;
	OB mOutputBuffer;
	FindResult mFirstResult;
	bool mKeepTerminator;
};


template<typename R, typename AsR = TRangeOfTypeNoCRef<R>> INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline Requires<
	CForwardCharRange<AsR>,
RByLine<AsR, String>> ByLine(R&& range, Tags::TKeepTerminator)
{return {ForwardAsRange<R>(range), true};}

template<typename R, typename AsR = TRangeOfTypeNoCRef<R>> INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline Requires<
	CForwardCharRange<AsR>,
RByLine<AsR, String>> ByLine(R&& range)
{return {ForwardAsRange<R>(range), false};}
INTRA_CORE_RANGE_END
