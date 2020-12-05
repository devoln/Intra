#pragma once

#include "Intra/Range/Concepts.h"
#include "Intra/Range/Operations.h"
#include "Intra/Range/Span.h"
#include "Intra/Range/RangeMethodsMixin.h"

INTRA_BEGIN
INTRA_IGNORE_WARN_COPY_IMPLICITLY_DELETED
template<class R, class B> class INTRA_EMPTY_BASES RBuffered:
	public RangeMethodsMixin<RBuffered<R, B>>,
	CopyableIf<!CRange<B>> //External buffer cannot be copied
{
public:
	constexpr RBuffered(R range, B&& buffer):
		mOriginalRange(Move(range)), mBuffer(Move(buffer)) {loadBuffer();}

	template<typename U = B, typename = Requires<
		!CRange<U>
	>> constexpr RBuffered(const RBuffered& rhs):
		mOriginalRange(rhs.mOriginalRange),
		mBuffer(rhs.mBuffer), mBufferRange(rhs.mBufferRange) {}

	template<typename U = B, typename = Requires<
		!CRange<U>	// To be copyable buffer must be a container, not a range.
	>> constexpr RBuffered& operator=(const RBuffered& rhs)
	{
		mOriginalRange = rhs.mOriginalRange;
		mBuffer = rhs.mBuffer;
		mBufferRange = rhs.mBufferRange;
		return *this;
	}

	RBuffered(RBuffered&& rhs) = default;

	RBuffered& operator=(RBuffered&& rhs) = default;


	[[nodiscard]] constexpr bool Empty() const {return mBufferRange.Empty();}
	[[nodiscard]] constexpr auto First() const {return mBufferRange.First();}

	constexpr void PopFirst()
	{
		mBufferRange.PopFirst();
		if(!mBufferRange.Empty()) return;
		loadBuffer();
	}

	constexpr void PopFirstCount(ClampedSize n)
	{
		const auto leftToPop = size_t(n) - size_t(mBufferRange.PopFirstCount(n));
		if(!mBufferRange.Empty()) return;
		PopFirstCount(mOriginalRange, leftToPop);
		loadBuffer();
	}

	constexpr void ReadWrite(Span<T>& dst)
	{
		ReadWrite(mBufferRange, dst);
		ReadWrite(mOriginalRange, dst);
		if(!mBufferRange.Empty()) return;
		loadBuffer();
	}

	template<typename U = R> [[nodiscard]] constexpr Requires<
		CHasIndex<U>,
	T> operator[](Index index)
	{
		if(size_t(index) < size_t(mBufferRange.Length())) return mBufferRange[index];
		return mOriginalRange[size_t(index) - size_t(mBufferRange.Length())];
	}

private:
	void loadBuffer() {mBufferRange = Take(mBuffer, ReadTo(mOriginalRange, mBuffer));}

	R mOriginalRange;
	B mBuffer;
	Span<T> mBufferRange;
};

template<typename R> [[nodiscard]] constexpr Requires<
	CAccessibleList<R>,
RBuffered<TRangeOf<R>, Span<TListValue<R>>>> Buffered(R&& range, Span<TListValue<R>> buffer)
{return {ForwardAsRange<R>(range), buffer};}

template<typename R> [[nodiscard]] inline Requires<
	CAccessibleList<R>,
RBuffered<TRangeOf<R>, Array<TListValue<R>>>> Buffered(R&& range, size_t bufferSize)
{return {ForwardAsRange<R>(range), Array<TListValue<R>>::CreateWithCount(bufferSize)};}
INTRA_END
