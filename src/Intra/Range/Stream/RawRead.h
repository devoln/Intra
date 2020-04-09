#pragma once

#include "Intra/Concepts.h"
#include "Intra/Range/Concepts.h"
#include "Intra/Range/Span.h"
#include "Intra/Range/Mutation/Copy.h"

INTRA_BEGIN
template<typename R, typename = Requires<CInputRange<R>>>
constexpr index_t RawReadTo(R& src, void* dst, Size n)
{
	return ReadTo(src, SpanOfPtr(static_cast<TValueTypeOf<R>*>(dst), n));
}

template<typename R, typename = Requires<CAsInputRange<R>>>
constexpr index_t RawCopyTo(R&& src, void* dst, Size n)
{
	auto srcCopy = ForwardAsRange<R>(src);
	return RawReadTo(srcCopy, dst, n);
}

template<class R, typename T, typename = Requires<CInputRange<R> && !CConst<T>>>
constexpr index_t RawReadTo(R& src, Span<T> dst)
{
	const auto dstLen = size_t(dst.Length())*sizeof(T)/sizeof(src.First());
	return RawReadTo(src, dst.Data(), dstLen);
}

template<class R, typename T, typename = Requires<CInputRange<R> && !CConst<T>>>
constexpr index_t RawReadTo(R& src, Span<T> dst, Size n)
{
	const auto dstLen = size_t(dst.Length())*(sizeof(T)/sizeof(src.First()));
	return RawReadTo(src, dst.Data(), FMin(size_t(n), dstLen));
}


template<class R, typename U, typename = Requires<CInputRange<R> && CTriviallyCopyable<U>>>
constexpr index_t RawReadWrite(R& src, Span<U>& dst, Size maxElementsToRead)
{
	typedef TValueTypeOf<R> T;
	auto dst1 = dst.Take(maxElementsToRead).template ReinterpretUnsafe<T>();
	const auto elementsRead = size_t(ReadWrite(src, dst1))*(sizeof(T)/sizeof(U));
	dst.Begin += elementsRead;
	return index_t(elementsRead);
}

template<typename R, typename R1,
    typename = Requires<CInputRange<R> && COutputRange<R1> && CArrayClass<R1> && CTriviallyCopyable<TValueTypeOf<R1>>>>
constexpr index_t RawReadWrite(R& src, R1& dst, Size maxElementsToRead)
{
	Span<U> dst1 = dst;
	const auto result = RawReadWrite(src, dst1, maxElementsToRead);
	PopFirstExactly(dst, result);
	return result;
}

template<typename R, typename R1, typename U = TArrayElement<R1>,
    typename = Requires<CInputRange<R> && COutputRange<R1> && CTriviallyCopyable<U>>>
constexpr index_t RawReadWrite(R& src, R1& dst) {return RawReadWrite(src, dst, dst.Length());}


template<typename T, typename R, typename = Requires<CInputRange<R> && !CConst<R> && !CConst<T>>>
constexpr RawRead(R& srcRange, T& dstElement)
{
	static_assert(sizeof(T) % sizeof(srcRange.First()) == 0);
	RawReadTo(srcRange, &dstElement, sizeof(T)/sizeof(srcRange.First()));
}


template<typename T, typename R, typename = Requires<CInputRange<R>>>
constexpr T RawRead(R& src)
{
	T result{};
	RawRead(src, result);
	return result;
}

template<typename R, typename = Requires<CInputRange<R>>>
constexpr byte RawReadByte(R& src)
{
	static_assert(sizeof(src.First()) == 1);
	const auto result = src.First();
	src.PopFirst();
	return reinterpret_cast<const byte&>(result);
}

template<class R, class = Requires<sizeof(TValueTypeOf<R>) == 1 && !CConst<R>>>
constexpr uint64 ParseVarUInt(R& src)
{
	uint64 result = 0;
	byte l = 0;
	do
	{
		if(src.Empty()) return result;
		l = byte(src.First());
		src.PopFirst();
		result = (result << 7) | (l & 0x7F);
	} while(l & 0x80);
	return result;
}

template<class R, class = Requires<sizeof(TValueTypeOf<R>) == 1 && !CConst<R>>>
constexpr uint64 ParseVarUInt(R& src, size_t& ioBytesRead)
{
	uint64 result = 0;
	byte l = 0;
	do
	{
		if(src.Empty()) return result;
		l = byte(src.First());
		src.PopFirst();
		ioBytesRead++;
		result = (result << 7) | (l & 0x7F);
	} while(l & 0x80);
	return result;
}
INTRA_END
