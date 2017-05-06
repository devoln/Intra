#pragma once

#include "Cpp/Features.h"
#include "Cpp/Warnings.h"
#include "Range/ForwardDecls.h"
#include "Concepts/Range.h"
#include "Concepts/RangeOf.h"
#include "Utils/Op.h"
#include "Range/Output/OutputArrayRange.h"
#include "Utils/StringView.h"
#include "Range/Mutation/CopyUntil.h"

namespace Intra { namespace Range {

INTRA_WARNING_PUSH
INTRA_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED

template<class R> class RByLineTo: Meta::NonCopyableType
{
public:
	typedef Concepts::ValueTypeOf<R> T;

	enum: bool
	{
		RangeIsFinite = Concepts::IsFiniteRange<R>::_,
		RangeIsInfinite = Concepts::IsInfiniteRange<R>::_
	};

	forceinline RByLineTo(null_t=null): mKeepTerminator(false) {}

	forceinline RByLineTo(R&& range, GenericStringView<T> buf, bool keepTerminator=false):
		mOriginalRange(Cpp::Move(range)), mBuffer(buf), mKeepTerminator(keepTerminator) {PopFirst();}

	forceinline RByLineTo(const R& range, GenericStringView<T> buf, bool keepTerminator=false):
		mOriginalRange(range), mBuffer(buf), mKeepTerminator(keepTerminator) {PopFirst();}

	RByLineTo(const RByLineTo& rhs) = delete;
	RByLineTo& operator=(const RByLineTo&) = delete;

	forceinline RByLineTo(RByLineTo&& rhs) = default;
	RByLineTo& operator=(RByLineTo&& rhs) = default;

	forceinline bool Empty() const noexcept {return mBuffer == null;}

	void PopFirst()
	{
		mBuffer.Reset();
		CopyAdvanceToAdvanceUntil(mOriginalRange, mBuffer, Op::IsLineSeparator<Concepts::ValueTypeOf<R>>);
		if(mOriginalRange.Empty())
		{
			if(mBuffer.ElementsWritten() == 0) mBuffer = null;
			return;
		}

		const auto nextChar = mOriginalRange.First();
		if(mBuffer.Empty()) return; //Buffer is full, so current line will be splitted

		if(mKeepTerminator) mBuffer.Put(nextChar);
		const bool CR = nextChar == '\r';
		mOriginalRange.PopFirst();

		if(!CR || mOriginalRange.Empty() || mOriginalRange.First() != '\n') return;

		if(mKeepTerminator)
		{
			if(mBuffer.Empty()) return; //Buffer is full, so next value will contain '\n'
			mBuffer.Put('\n');
		}
		mOriginalRange.PopFirst();
	}

	forceinline GenericStringView<const T> First() const
	{
		INTRA_DEBUG_ASSERT(!Empty());
		return mBuffer.GetWrittenData();
	}

private:
	R mOriginalRange;
	OutputArrayRange<T> mBuffer;
	bool mKeepTerminator;
};


template<typename R, typename OR,
	typename AsR=Concepts::RangeOfTypeNoCRef<R>,
	typename AsOR=Concepts::RangeOfTypeNoCRef<OR>
> forceinline Meta::EnableIf<
	Concepts::IsForwardCharRange<AsR>::_ &&
	Concepts::IsArrayRange<AsOR>::_,
RByLineTo<AsR>> ByLine(R&& range, OR&& buf, Tags::TKeepTerminator)
{return {Range::Forward<R>(range), Range::Forward<OR>(buf), true};}

template<typename R, typename OR,
	typename AsR = Concepts::RangeOfTypeNoCRef<R>,
	typename AsOR = Concepts::RangeOfTypeNoCRef<OR>
> forceinline Meta::EnableIf<
	Concepts::IsForwardCharRange<AsR>::_ &&
	Concepts::IsArrayRange<AsOR>::_,
RByLineTo<AsR>> ByLine(R&& range, OR&& buf)
{return {Range::Forward<R>(range), Range::Forward<OR>(buf), false};}

INTRA_WARNING_POP

}}
