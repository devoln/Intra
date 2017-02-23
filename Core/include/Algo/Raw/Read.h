#pragma once

#include "Range/Concepts.h"
#include "Algo/Mutation/Copy.h"

namespace Intra { namespace Algo {

using namespace Range::Concepts;

template<typename R> forceinline Meta::EnableIf<
	IsInputRange<R>::_,
size_t> CopyAdvanceToRaw(R& src, void* dst, size_t n)
{
	typedef ValueTypeOf<R> T;
	return Algo::CopyAdvanceTo(src, ArrayRange<T>(reinterpret_cast<T*>(dst), n));
}

template<typename R> forceinline Meta::EnableIf<
	IsAsInputRange<R>::_,
size_t> CopyToRaw(R&& src, void* dst, size_t n)
{
	auto srcCopy = Range::Forward<R>(src);
	return Algo::CopyAdvanceToRaw(srcCopy, dst, n);
}

template<typename R, typename T> Meta::EnableIf<
	IsInputRange<R>::_ && !Meta::IsConst<T>::_,
size_t> CopyAdvanceToRaw(R& src, ArrayRange<T> dst)
{
	size_t dstLen = dst.Length()*sizeof(T)/sizeof(src.First());
	return CopyAdvanceToRaw(src, dst.Data(), dstLen);
}

template<typename R, typename T> Meta::EnableIf<
	IsInputRange<R>::_ && !Meta::IsConst<T>::_,
size_t> CopyAdvanceToRaw(R& src, ArrayRange<T> dst, size_t n)
{
	size_t dstLen = dst.Length()*sizeof(T)/sizeof(src.First());
	if(n>dstLen) n = dstLen;
	return CopyAdvanceToRaw(src, dst.Data(), n);
}

template<typename R, typename T> Meta::EnableIf<
	IsInputRange<R>::_ && !Meta::IsConst<T>::_,
size_t> CopyAdvanceToRawOne(R& src, T& dst)
{
	static_assert(sizeof(T)%sizeof(src.First())==0, "Error!");
	return CopyAdvanceToRaw(src, &dst, sizeof(T)/sizeof(src.First()));
}


template<typename T, typename R> Meta::EnableIf<
	IsInputRange<R>::_,
T> ReadRaw(R& src)
{
	T result;
	CopyAdvanceToRawOne(src, result);
	return result;
}

}}
