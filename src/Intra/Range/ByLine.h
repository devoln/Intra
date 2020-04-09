#pragma once

#include "Intra/Functional.h"
#include "Intra/Range/Concepts.h"
#include "Intra/Range/StringView.h"
#include "Intra/Range/Mutation/CopyUntil.h"

//TODO: make this class more generic to work with even containers to avoid line splitting.

INTRA_BEGIN
INTRA_IGNORE_WARNING_COPY_IMPLICITLY_DELETED
template<class R> class RByLine: NonCopyableType
{
	using T = TValueTypeOf<R>;
public:
	enum: bool
	{
		IsAnyInstanceFinite = CFiniteRange<R>,
		IsAnyInstanceInfinite = CInfiniteRange<R>
	};

	constexpr RByLine() = default;

	constexpr RByLine(R range, Span<T> buf, bool keepTerminator = false):
		mOriginalRange(Move(range)), mOutputBuffer(buf), mKeepTerminator(keepTerminator) {PopFirst();}

	constexpr RByLine(RByLine&& rhs) = default;
	constexpr RByLine& operator=(RByLine&& rhs) = default;

	[[nodiscard]] constexpr bool Empty() const noexcept {return !mOutputBuffer.IsInitialized();}

	constexpr void PopFirst()
	{
		mOutputBuffer.Reset();
		mFirstResult = ReadWriteUntil(mOriginalRange, mOutputBuffer, IsLineSeparator);
		if(mOriginalRange.Empty())
		{
			if(mOutputBuffer.Position() == 0) mOutputBuffer = null;
			return;
		}
		if(mOutputBuffer.Full()) return; //This line is longer than buffer size, so it will be splitted

		//Processing the line terminator: CR (\r), LF (\n) or CRLF(\r\n)
		const T firstTerminatorChar = mOriginalRange.First();
		INTRA_DEBUG_ASSERT(firstTerminatorChar == '\r' || firstTerminatorChar == '\n');
		const bool CR = firstTerminatorChar == '\r';
		if(mKeepTerminator) mOutputBuffer.Put(firstTerminatorChar);
		mOriginalRange.PopFirst();

		if(!CR || mOriginalRange.Empty() || mOriginalRange.First() != '\n') return;

		if(mKeepTerminator)
		{
			if(mOutputBuffer.Empty()) return; //Buffer is full, so the remaining "\n" will become the next range element
			mOutputBuffer.Put('\n');
		}
		mOriginalRange.PopFirst();
	}

	[[nodiscard]] constexpr GenericStringView<const T> First() const
	{
		INTRA_PRECONDITION(!Empty());
		return mOutputBuffer.WrittenRange();
	}

	[[nodiscard]] constexpr FindResult GetFindResult() const {return mFirstResult;}

private:
	R mOriginalRange;
	SpanOutput<T> mOutputBuffer;
	FindResult mFirstResult;
	bool mKeepTerminator = false;
};


template<typename R, typename OR,
	typename AsR = TRangeOf<R>,
	typename AsOR = TRangeOf<OR>
> [[nodiscard]] constexpr Requires<
	CForwardCharRange<AsR> &&
	CArrayRange<AsOR>,
RByLine<AsR>> ByLine(R&& range, OR&& buf, Tags::TKeepTerminator)
{return {ForwardAsRange<R>(range), ForwardAsRange<OR>(buf), true};}

template<typename R, typename OR,
	typename AsR = TRangeOf<R>,
	typename AsOR = TRangeOf<OR>
> [[nodiscard]] constexpr Requires<
	CForwardCharRange<AsR> &&
	CArrayRange<AsOR>,
RByLine<AsR>> ByLine(R&& range, OR&& buf)
{return {ForwardAsRange<R>(range), ForwardAsRange<OR>(buf), false};}
INTRA_END
