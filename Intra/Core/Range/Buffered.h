#pragma once

#include "Core/Range/Concepts.h"
#include "Core/CContainer.h"
#include "Core/Range/Operations.h"
#include "Core/Range/Span.h"
#include "Core/Range/RangeMethodsMixin.h"

INTRA_CORE_RANGE_BEGIN
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED
template<class R, class B> class INTRA_EMPTY_BASES RBuffered:
	public RangeMethodsMixin<RBuffered<R, B>>,
	CopyableIf<!CInputRange<B>> //External buffer cannot be copied
{
public:
	constexpr forceinline RBuffered(R range, B&& buffer):
		mOriginalRange(Move(range)), mBuffer(Move(buffer)) {loadBuffer();}

	template<typename U = B, typename = Requires<
		!CInputRange<U>
	>> constexpr RBuffered(const RBuffered& rhs):
		mOriginalRange(rhs.mOriginalRange),
		mBuffer(rhs.mBuffer), mBufferRange(rhs.mBufferRange) {}

	template<typename U=B, typename = Requires<
		!CInputRange<U>	// To be copyable buffer must be a container, not a range.
	>> constexpr RBuffered& operator=(const RBuffered& rhs)
	{
		mOriginalRange = rhs.mOriginalRange;
		mBuffer = rhs.mBuffer;
		mBufferRange = rhs.mBufferRange;
		return *this;
	}

	RBuffered(RBuffered&& rhs) = default;

	RBuffered& operator=(RBuffered&& rhs) = default;


	INTRA_NODISCARD constexpr forceinline bool Empty() const {return mBufferRange.Empty();}
	INTRA_NODISCARD constexpr forceinline auto First() const {return mBufferRange.First();}

	INTRA_CONSTEXPR2 void PopFirst()
	{
		mBufferRange.PopFirst();
		if(!mBufferRange.Empty()) return;
		loadBuffer();
	}

	INTRA_CONSTEXPR2 void PopFirstN(size_t n)
	{
		n -= mBufferRange.PopFirstN(n);
		if(!mBufferRange.Empty()) return;
		PopFirstN(mOriginalRange, n);
		loadBuffer();
	}

	INTRA_CONSTEXPR2 void ReadWrite(Span<T>& dst)
	{
		ReadWrite(mBufferRange, dst);
		ReadWrite(mOriginalRange, dst);
		if(!mBufferRange.Empty()) return;
		loadBuffer();
	}

	template<typename U=R> INTRA_NODISCARD INTRA_CONSTEXPR2 Requires<
		CHasIndex<U>,
	T> operator[](size_t index)
	{
		if(index < mBufferRange.Length()) return mBufferRange[index];
		return mOriginalRange[index - mBufferRange.Length()];
	}

private:
	void loadBuffer() {mBufferRange = Take(mBuffer, ReadTo(mOriginalRange, mBuffer));}

	R mOriginalRange;
	B mBuffer;
	Span<T> mBufferRange;
};

template<typename R> INTRA_NODISCARD constexpr forceinline Requires<
	CAsAccessibleRange<R>,
RBuffered<TRangeOfTypeNoCRef<R>, Span<TValueTypeOfAs<R>>>> Buffered(R&& range, Span<TValueTypeOfAs<R>> buffer)
{return {ForwardAsRange<R>(range), buffer};}

template<typename R> INTRA_NODISCARD forceinline Requires<
	CAsAccessibleRange<R>,
RBuffered<TRangeOfTypeNoCRef<R>, Array<TValueTypeOfAs<R>>>> Buffered(R&& range, size_t bufferSize)
{return {ForwardAsRange<R>(range), Array<TValueTypeOfAs<R>>::CreateWithCount(bufferSize)};}
INTRA_CORE_RANGE_END
