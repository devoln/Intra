#pragma once

#include "Concepts/Range.h"
#include "Range/Mutation/Copy.h"

namespace Intra { namespace Range {

template<typename OR> forceinline Meta::EnableIf<
	Concepts::IsOutputRange<OR>::_,
size_t> CopyToRawAdvance(const void* src, OR& dst, size_t n)
{
	typedef Concepts::ValueTypeOf<OR> T;
	return CopyToAdvance(CSpan<T>(reinterpret_cast<const T*>(src), n), dst);
}

template<typename OR> forceinline Meta::EnableIf<
	Concepts::IsAsOutputRange<OR>::_,
size_t> CopyToRaw(const void* src, OR&& dst, size_t n)
{
	auto dstCopy = Range::Forward<OR>(dst);
	return CopyToRawAdvance(src, dstCopy, n);
}

template<typename OR, typename T> Meta::EnableIf<
	Concepts::IsOutputRange<OR>::_ &&
	!Meta::IsConst<T>::_,
size_t> CopyToRawAdvance(CSpan<T> src, OR& dst)
{
	size_t srcLen = src.Length()*sizeof(T)/sizeof(dst.First());
	return CopyToRawAdvance(src.Data(), dst, srcLen);
}

template<typename R, typename T> Meta::EnableIf<
	Concepts::IsOutputRange<R>::_ &&
	!Meta::IsConst<T>::_,
size_t> CopyToRawAdvance(Span<T> src, R& dst, size_t n)
{
	size_t srcLen = src.Length()*sizeof(T)/sizeof(dst.First());
	if(n>srcLen) n = srcLen;
	return CopyToRawAdvance(src.Data(), dst, n);
}

template<typename R, typename T> Meta::EnableIf<
	Concepts::IsOutputRange<R>::_ &&
	!Meta::IsConst<T>::_,
size_t> CopyOneToRawAdvance(const T& src, R& dst)
{
	static_assert(sizeof(T)%sizeof(dst.First())==0, "Error!");
	return CopyToRawAdvance(&src, dst, sizeof(T)/sizeof(dst.First()));
}


template<typename T, typename R> void WriteRaw(R& dst, const T& value)
{
	static_assert(sizeof(T)%sizeof(dst.First())==0, "Error!");
	CopyToRawAdvance(&value, dst, sizeof(T)/sizeof(dst.First()));
}

}}
