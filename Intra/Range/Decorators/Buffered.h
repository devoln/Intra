#pragma once

#include "Concepts/Range.h"
#include "Container/ForwardDecls.h"
#include "Range/Operations.h"
#include "Utils/Span.h"

INTRA_WARNING_PUSH
INTRA_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED

namespace Intra { namespace Range {

template<class R, class B> class RBuffered:
	Meta::CopyableIf<!Concepts::IsInputRange<B>::_> //Внешний буфер нельзя копировать
{
	typedef Concepts::ValueTypeOf<R> T;
public:
	RBuffered(const R& range, B&& buffer):
		mOriginalRange(range), mBuffer(Cpp::Move(buffer)) {loadBuffer();}

	RBuffered(R&& range, B&& buffer):
		mOriginalRange(Cpp::Move(range)), mBuffer(Cpp::Move(buffer)) {loadBuffer();}

	template<typename U=B, typename=Meta::EnableIf<
		!Concepts::IsInputRange<U>::_
	>> RBuffered(const RBuffered& rhs):
		mOriginalRange(rhs.mOriginalRange),
		mBuffer(rhs.mBuffer), mBufferRange(rhs.mBufferRange) {}

	template<typename U=B, typename=Meta::EnableIf<
		!Concepts::IsInputRange<U>::_	//Буфером должен быть не диапазон, а контейнер, чтобы он копировался по значению
	>> RBuffered& operator=(const RBuffered& rhs)
	{
		mOriginalRange = rhs.mOriginalRange;
		mBuffer = rhs.mBuffer;
		mBufferRange = rhs.mBufferRange;
		return *this;
	}

	RBuffered(RBuffered&& rhs):
		mOriginalRange(Cpp::Move(rhs.mOriginalRange)),
		mBuffer(Cpp::Move(rhs.mBuffer)), mBufferRange(rhs.mBufferRange) {}

	RBuffered& operator=(RBuffered&& rhs)
	{
		mOriginalRange = Cpp::Move(rhs.mOriginalRange);
		mBuffer = Cpp::Move(rhs.mBuffer);
		mBufferRange = rhs.mBufferRange;
		return *this;
	}


	forceinline bool Empty() const {return mBufferRange.Empty();}
	forceinline T First() const {return mBufferRange.First();}

	void PopFirst()
	{
		mBufferRange.PopFirst();
		if(!mBufferRange.Empty()) return;
		loadBuffer();
	}

	void PopFirstN(size_t n)
	{
		n -= mBufferRange.PopFirstN(n);
		if(!mBufferRange.Empty()) return;
		PopFirstN(mOriginalRange, n);
		loadBuffer();
	}

	void ReadToAdvance(Span<T>& dst)
	{
		ReadToAdvance(mBufferRange, dst);
		ReadToAdvance(mOriginalRange, dst);
		if(!mBufferRange.Empty()) return;
		loadBuffer();
	}

	template<typename U=R> Meta::EnableIf<
		HasIndex<U>::_,
	T> operator[](size_t index)
	{
		if(index<mBufferRange.Length()) return mBufferRange[index];
		return mOriginalRange[index-mBufferRange.Length()];
	}

private:
	void loadBuffer() {mBufferRange = Take(mBuffer, ReadTo(mOriginalRange, mBuffer));}

	R mOriginalRange;
	Span<T> mBuffer;
	Span<T> mBufferRange;
};

template<typename R> forceinline Meta::EnableIf<
	Concepts::IsAsAccessibleRange<R>::_,
RBuffered<Concepts::RangeOfTypeNoCRef<R>>> Buffered(R&& range, Span<Concepts::ValueTypeOfAs<R>> buffer)
{return {Range::Forward<R>(range), buffer};}

template<typename R> forceinline Meta::EnableIf<
	Concepts::IsAsAccessibleRange<R>::_,
RBuffered<Concepts::RangeOfTypeNoCRef<R>>> Buffered(R&& range, size_t bufferSize)
{return {Range::Forward<R>(range), Array<Concepts::ValueTypeOfAs<R>>::CreateWithCount(bufferSize)};}

}}

INTRA_WARNING_POP
