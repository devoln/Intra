#pragma once

#include "Range/Concepts.h"
#include "Container/ForwardDecls.h"
#include "Range/Operations.h"
#include "Range/Generators/Span.h"

INTRA_WARNING_PUSH
INTRA_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED

namespace Intra { namespace Range {

template<class R, class B> class RBuffered:
	Meta::CopyableIf<!IsInputRange<B>::_> //Внешний буфер нельзя копировать
{
	typedef ValueTypeOf<R> T;
public:
	RBuffered(const R& range, B&& buffer):
		mOriginalRange(range), mBuffer(Meta::Move(buffer)) {loadBuffer();}

	RBuffered(R&& range, B&& buffer):
		mOriginalRange(Meta::Move(range)), mBuffer(Meta::Move(buffer)) {loadBuffer();}

	template<typename U=B, typename=Meta::EnableIf<
		!IsInputRange<U>::_	
	>> RBuffered(const RBuffered& rhs):
		mOriginalRange(rhs.mOriginalRange),
		mBuffer(rhs.mBuffer), mBufferRange(rhs.mBufferRange) {}

	template<typename U=B, typename=Meta::EnableIf<
		!IsInputRange<U>::_	//Буфером должен быть не диапазон, а контейнер, чтобы он копировался по значению
	>> RBuffered& operator=(const RBuffered& rhs)
	{
		mOriginalRange = rhs.mOriginalRange;
		mBuffer = rhs.mBuffer;
		mBufferRange = rhs.mBufferRange;
		return *this;
	}

	RBuffered(RBuffered&& rhs):
		mOriginalRange(Meta::Move(rhs.mOriginalRange)),
		mBuffer(Meta::Move(rhs.mBuffer)), mBufferRange(rhs.mBufferRange) {}

	RBuffered& operator=(RBuffered&& rhs)
	{
		mOriginalRange = Meta::Move(rhs.mOriginalRange);
		mBuffer = Meta::Move(rhs.mBuffer);
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

	void CopyAdvanceToAdvance(Span<T>& dst)
	{
		Algo::CopyAdvanceToAdvance(mBufferRange, dst);
		Algo::CopyAdvanceToAdvance(mOriginalRange, dst);
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
	void loadBuffer() {mBufferRange = Take(mBuffer, Algo::CopyAdvanceTo(mOriginalRange, mBuffer));}

	R mOriginalRange;
	Span<T> mBuffer;
	Span<T> mBufferRange;
};

template<typename R> forceinline Meta::EnableIf<
	IsAsAccessibleRange<R>::_,
RBuffered<AsRangeResultNoCRef<R>>> Buffered(R&& range, Span<ValueTypeOfAs<R>> buffer)
{return {Range::Forward<R>(range), buffer};}

template<typename R> forceinline Meta::EnableIf<
	IsAsAccessibleRange<R>::_,
RBuffered<AsRangeResultNoCRef<R>>> Buffered(R&& range, size_t bufferSize)
{return {Range::Forward<R>(range), Array<ValueTypeOfAs<R>>::CreateWithCount(bufferSize)};}

}}

INTRA_WARNING_POP
