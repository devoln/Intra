#pragma once

#include "Core/CArray.h"
#include "Core/Range/Concepts.h"
#include "Core/Range/Span.h"
#include "Core/Range/Mutation/Copy.h"

INTRA_BEGIN
inline namespace Range {

template<typename R> INTRA_CONSTEXPR2 forceinline Requires<
	CInputRange<R>,
size_t> RawReadTo(R& src, void* dst, size_t n)
{
	return ReadTo(src, SpanOfPtr(static_cast<TValueTypeOf<R>*>(dst), n));
}

template<typename R,
	typename AsR = TRangeOfType<R>
> INTRA_CONSTEXPR2 forceinline Requires<
	CInputRange<AsR>,
size_t> RawCopyTo(R&& src, void* dst, size_t n)
{
	auto srcCopy = ForwardAsRange<R>(src);
	return RawReadTo(srcCopy, dst, n);
}

template<typename R, typename T> INTRA_CONSTEXPR2 Requires<
	CInputRange<R> &&
	!CConst<T>,
size_t> RawReadTo(R& src, Span<T> dst)
{
	const size_t dstLen = dst.Length()*sizeof(T)/sizeof(src.First());
	return RawReadTo(src, dst.Data(), dstLen);
}

template<typename R, typename T> INTRA_CONSTEXPR2 Requires<
	CInputRange<R> &&
	!CConst<T>,
size_t> RawReadTo(R& src, Span<T> dst, size_t n)
{
	const size_t dstLen = dst.Length()*sizeof(T)/sizeof(src.First());
	if(n > dstLen) n = dstLen;
	return RawReadTo(src, dst.Data(), n);
}


template<typename R, typename U> INTRA_CONSTEXPR2 Requires<
	CInputRange<R> &&
	CPod<U>,
size_t> RawReadWrite(R& src, Span<U>& dst, size_t maxElementsToRead)
{
	typedef TValueTypeOf<R> T;
	auto dst1 = dst.Take(maxElementsToRead).template Reinterpret<T>();
	size_t elementsRead = ReadWrite(src, dst1)*sizeof(T)/sizeof(U);
	dst.Begin += elementsRead;
	return elementsRead;
}

template<typename R, typename R1, typename U = TValueTypeOf<R1>> INTRA_CONSTEXPR2 Requires<
	CInputRange<R> &&
	COutputRange<R1> &&
	CArrayClass<R1> &&
	CPod<U>,
size_t> RawReadWrite(R& src, R1& dst, size_t maxElementsToRead)
{
	Span<U> dst1 = dst;
	size_t result = RawReadWrite(src, dst1, maxElementsToRead);
	PopFirstExactly(dst, result);
	return result;
}

template<typename R, typename R1,
	typename U = TArrayElement<R1>
> INTRA_CONSTEXPR2 forceinline Requires<
	CInputRange<R> &&
	COutputRange<R1> &&
	CPod<U>,
size_t> RawReadWrite(R& src, R1& dst)
{return RawReadWrite(src, dst, dst.Length());}


template<typename T, typename R> INTRA_CONSTEXPR2 forceinline Requires<
	CInputRange<R> &&
	!CConst<R> &&
	!CConst<T>
> RawRead(R& srcRange, T& dstElement)
{
	static_assert(sizeof(T) % sizeof(srcRange.First()) == 0, "Error!");
	RawReadTo(srcRange, &dstElement, sizeof(T)/sizeof(srcRange.First()));
}


template<typename T, typename R> INTRA_CONSTEXPR2 Requires<
	CInputRange<R>,
T> RawRead(R& src)
{
	T result{};
	RawRead(src, result);
	return result;
}

template<typename R> INTRA_CONSTEXPR2 Requires<
	CInputRange<R>,
byte> RawReadByte(R& src)
{
	static_assert(sizeof(src.First()) == 1, "Error!");
	const auto result = src.First();
	src.PopFirst();
	return reinterpret_cast<const byte&>(result);
}

template<typename R> INTRA_CONSTEXPR2 Requires<
	sizeof(TValueTypeOf<R>) == 1 && !CConst<R>,
uint64> ParseVarUInt(R& src)
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

template<typename R> INTRA_CONSTEXPR2 Requires<
	sizeof(TValueTypeOf<R>) == 1 && !CConst<R>,
uint64> ParseVarUInt(R& src, size_t& ioBytesRead)
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

}
INTRA_END
