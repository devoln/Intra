#pragma once

#include "Concepts/Range.h"
#include "Utils/Span.h"
#include "Range/Mutation/Copy.h"

namespace Intra { namespace Range {

template<typename R> forceinline Meta::EnableIf<
	Concepts::IsInputRange<R>::_,
size_t> CopyAdvanceToRaw(R& src, void* dst, size_t n)
{
	return CopyAdvanceTo(src, Take(reinterpret_cast<Concepts::ValueTypeOf<R>*>(dst), n));
}

template<typename R> forceinline Meta::EnableIf<
	Concepts::IsAsInputRange<R>::_,
size_t> CopyToRaw(R&& src, void* dst, size_t n)
{
	auto srcCopy = Range::Forward<R>(src);
	return CopyAdvanceToRaw(srcCopy, dst, n);
}

template<typename R, typename T> Meta::EnableIf<
	Concepts::IsInputRange<R>::_ &&
	!Meta::IsConst<T>::_,
size_t> CopyAdvanceToRaw(R& src, Span<T> dst)
{
	size_t dstLen = dst.Length()*sizeof(T)/sizeof(src.First());
	return CopyAdvanceToRaw(src, dst.Data(), dstLen);
}

template<typename R, typename T> Meta::EnableIf<
	Concepts::IsInputRange<R>::_ &&
	!Meta::IsConst<T>::_,
size_t> CopyAdvanceToRaw(R& src, Span<T> dst, size_t n)
{
	const size_t dstLen = dst.Length()*sizeof(T)/sizeof(src.First());
	if(n > dstLen) n = dstLen;
	return CopyAdvanceToRaw(src, dst.Data(), n);
}

template<typename R, typename T> Meta::EnableIf<
	Concepts::IsInputRange<R>::_ &&
	!Meta::IsConst<T>::_,
size_t> CopyAdvanceToRawOne(R& src, T& dst)
{
	static_assert(sizeof(T)%sizeof(src.First())==0, "Error!");
	return CopyAdvanceToRaw(src, &dst, sizeof(T)/sizeof(src.First()));
}


template<typename T, typename R> Meta::EnableIf<
	Concepts::IsInputRange<R>::_,
T> ReadRaw(R& src)
{
	T result;
	CopyAdvanceToRawOne(src, result);
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
