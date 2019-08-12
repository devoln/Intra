#pragma once

#include "Core/Core.h"
#include "Core/Functional.h"

#include "Core/Range/Concepts.h"
#include "Core/Range/StringView.h"
#include "Core/Range/Mutation/CopyUntil.h"

INTRA_CORE_RANGE_BEGIN
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED
template<class R> class RByLineTo: NonCopyableType
{
public:
	typedef TValueTypeOf<R> T;

	enum: bool
	{
		RangeIsFinite = CFiniteRange<R>,
		RangeIsInfinite = CInfiniteRange<R>
	};

	constexpr forceinline RByLineTo() = default;

	constexpr forceinline RByLineTo(R&& range, GenericStringView<T> buf, bool keepTerminator=false):
		mOriginalRange(Move(range)), mBuffer(buf), mKeepTerminator(keepTerminator) {PopFirst();}

	constexpr forceinline RByLineTo(const R& range, GenericStringView<T> buf, bool keepTerminator=false):
		mOriginalRange(range), mBuffer(buf), mKeepTerminator(keepTerminator) {PopFirst();}

	RByLineTo(const RByLineTo& rhs) = delete;
	RByLineTo& operator=(const RByLineTo&) = delete;

	forceinline RByLineTo(RByLineTo&& rhs) = default;
	RByLineTo& operator=(RByLineTo&& rhs) = default;

	INTRA_NODISCARD constexpr forceinline bool Empty() const noexcept {return mBuffer == null;}

	INTRA_CONSTEXPR2 void PopFirst()
	{
		mBuffer.Reset();
		ReadWriteUntil(mOriginalRange, mBuffer, Funal::IsLineSeparator);
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

	INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline GenericStringView<const T> First() const
	{
		INTRA_DEBUG_ASSERT(!Empty());
		return mBuffer.GetWrittenData();
	}

private:
	R mOriginalRange;
	SpanOutput<T> mBuffer;
	bool mKeepTerminator = false;
};


template<typename R, typename OR,
	typename AsR = TRangeOfTypeNoCRef<R>,
	typename AsOR = TRangeOfTypeNoCRef<OR>
> INTRA_NODISCARD constexpr forceinline Requires<
	CForwardCharRange<AsR> &&
	CArrayRange<AsOR>,
RByLineTo<AsR>> ByLine(R&& range, OR&& buf, Tags::TKeepTerminator)
{return {ForwardAsRange<R>(range), ForwardAsRange<OR>(buf), true};}

template<typename R, typename OR,
	typename AsR = TRangeOfTypeNoCRef<R>,
	typename AsOR = TRangeOfTypeNoCRef<OR>
> INTRA_NODISCARD constexpr forceinline Requires<
	CForwardCharRange<AsR> &&
	CArrayRange<AsOR>,
RByLineTo<AsR>> ByLine(R&& range, OR&& buf)
{return {ForwardAsRange<R>(range), ForwardAsRange<OR>(buf), false};}
INTRA_CORE_RANGE_END
