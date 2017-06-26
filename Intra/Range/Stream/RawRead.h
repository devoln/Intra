#pragma once

#include "Concepts/Range.h"
#include "Utils/Span.h"
#include "Range/Mutation/Copy.h"

namespace Intra { namespace Range {

template<typename R> forceinline Meta::EnableIf<
	Concepts::IsInputRange<R>::_,
size_t> RawReadTo(R& src, void* dst, size_t n)
{
	return ReadTo(src, Take(static_cast<Concepts::ValueTypeOf<R>*>(dst), n));
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

template<typename T, typename R> forceinline Meta::EnableIf<
	Concepts::IsInputRange<R>::_ &&
	!Meta::IsConst<R>::_ &&
	!Meta::IsConst<T>::_,
size_t> RawRead(R& srcRange, T& dstElement)
{
	static_assert(sizeof(T) % sizeof(srcRange.First())==0, "Error!");
	return RawReadTo(srcRange, &dstElement, sizeof(T)/sizeof(srcRange.First()));
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

}}
