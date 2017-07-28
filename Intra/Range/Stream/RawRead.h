#pragma once

#include "Concepts/Range.h"
#include "Utils/Span.h"
#include "Range/Mutation/Copy.h"

namespace Intra { namespace Range {

template<typename R> forceinline Meta::EnableIf<
	Concepts::IsInputRange<R>::_,
size_t> RawReadTo(R& src, void* dst, size_t n)
{
	return ReadTo(src, SpanOfPtr(static_cast<Concepts::ValueTypeOf<R>*>(dst), n));
}

template<typename R,
	typename AsR = Concepts::RangeOfType<R>
> forceinline Meta::EnableIf<
	Concepts::IsInputRange<AsR>::_,
size_t> RawCopyTo(R&& src, void* dst, size_t n)
{
	auto srcCopy = Range::Forward<R>(src);
	return RawReadTo(srcCopy, dst, n);
}

template<typename R, typename T> Meta::EnableIf<
	Concepts::IsInputRange<R>::_ &&
	!Meta::IsConst<T>::_,
size_t> RawReadTo(R& src, Span<T> dst)
{
	const size_t dstLen = dst.Length()*sizeof(T)/sizeof(src.First());
	return RawReadTo(src, dst.Data(), dstLen);
}

template<typename R, typename T> Meta::EnableIf<
	Concepts::IsInputRange<R>::_ &&
	!Meta::IsConst<T>::_,
size_t> RawReadTo(R& src, Span<T> dst, size_t n)
{
	const size_t dstLen = dst.Length()*sizeof(T)/sizeof(src.First());
	if(n > dstLen) n = dstLen;
	return RawReadTo(src, dst.Data(), n);
}


template<typename R, typename U> Meta::EnableIf<
	Concepts::IsInputRange<R>::_ &&
	Meta::IsTriviallySerializable<U>::_,
size_t> RawReadWrite(R& src, Span<U>& dst, size_t maxElementsToRead)
{
	typedef Concepts::ValueTypeOf<R> T;
	auto dst1 = dst.Take(maxElementsToRead).template Reinterpret<T>();
	size_t elementsRead = ReadWrite(src, dst1)*sizeof(T)/sizeof(U);
	dst.Begin += elementsRead;
	return elementsRead;
}

template<typename R, typename R1, typename U = Concepts::ValueTypeOf<R1>> Meta::EnableIf<
	Concepts::IsInputRange<R>::_ &&
	Concepts::IsOutputRange<R1>::_ &&
	Concepts::IsArrayClass<R1>::_ &&
	Meta::IsTriviallySerializable<U>::_,
size_t> RawReadWrite(R& src, R1& dst, size_t maxElementsToRead)
{
	Span<U> dst1 = dst;
	size_t result = RawReadWrite(src, dst1, maxElementsToRead);
	PopFirstExactly(dst, result);
	return result;
}

template<typename R, typename R1,
	typename U = Concepts::ElementTypeOfArray<R1>
> forceinline Meta::EnableIf<
	Concepts::IsInputRange<R>::_ &&
	Concepts::IsOutputRange<R1>::_ &&
	Meta::IsTriviallySerializable<U>::_,
size_t> RawReadWrite(R& src, R1& dst)
{return RawReadWrite(src, dst, dst.Length());}


template<typename T, typename R> forceinline Meta::EnableIf<
	Concepts::IsInputRange<R>::_ &&
	!Meta::IsConst<R>::_ &&
	!Meta::IsConst<T>::_
> RawRead(R& srcRange, T& dstElement)
{
	static_assert(sizeof(T) % sizeof(srcRange.First()) == 0, "Error!");
	RawReadTo(srcRange, &dstElement, sizeof(T)/sizeof(srcRange.First()));
}


template<typename T, typename R> Meta::EnableIf<
	Concepts::IsInputRange<R>::_,
T> RawRead(R& src)
{
	T result;
	RawRead(src, result);
	return result;
}

template<typename R> Meta::EnableIf<
	Concepts::IsInputRange<R>::_,
byte> RawReadByte(R& src)
{
	static_assert(sizeof(src.First()) == 1, "Error!");
	const auto result = src.First();
	src.PopFirst();
	return reinterpret_cast<const byte&>(result);
}

template<typename R> Meta::EnableIf<
	sizeof(Concepts::ValueTypeOf<R>) == 1 && !Meta::IsConst<R>::_,
ulong64> ParseVarUInt(R& src)
{
	ulong64 result = 0;
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

template<typename R> Meta::EnableIf<
	sizeof(Concepts::ValueTypeOf<R>) == 1 && !Meta::IsConst<R>::_,
ulong64> ParseVarUInt(R& src, size_t& ioBytesRead)
{
	ulong64 result = 0;
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

}}
