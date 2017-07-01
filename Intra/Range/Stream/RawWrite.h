#pragma once

#include "Concepts/Range.h"
#include "Range/Mutation/Copy.h"

namespace Intra { namespace Range {

template<typename OR> forceinline Meta::EnableIf<
	Concepts::IsOutputRangeOf<OR, char>::_ ||
	Concepts::IsOutputRangeOf<OR, byte>::_,
size_t> RawWriteFrom(OR& dst, const void* src, size_t n)
{
	return WriteTo(CSpan<char>(reinterpret_cast<const char*>(src), n), dst);
}

template<typename OR, typename T> Meta::EnableIf<
	Concepts::IsOutputRangeOf<OR, char>::_ ||
	Concepts::IsOutputRangeOf<OR, byte>::_,
size_t> RawWriteFrom(OR& dst, CSpan<T> src)
{
	const size_t srcLen = src.Length()*sizeof(T);
	return RawWriteFrom(dst, src.Data(), srcLen);
}

template<typename OR, typename T> Meta::EnableIf<
	Concepts::IsOutputRangeOf<OR, char>::_ ||
	Concepts::IsOutputRangeOf<OR, byte>::_,
size_t> RawWriteFrom(OR& dst, Span<T> src, size_t n)
{
	const size_t srcLen = src.Length()*sizeof(T);
	if(n > srcLen) n = srcLen;
	return RawWriteFrom(dst, src.Data(), n);
}

template<typename OR, typename T> Meta::EnableIf<
	Concepts::IsOutputRangeOf<OR, char>::_ ||
	Concepts::IsOutputRangeOf<OR, byte>::_,
size_t> RawWriteFrom(OR& dst, const T& src)
{
	const size_t srcLen = src.Length()*sizeof(T);
	return RawWriteFrom(dst, &src, srcLen);
}


template<typename T, typename OR> Meta::EnableIf<
	Concepts::IsOutputRangeOf<OR, char>::_ ||
	Concepts::IsOutputRangeOf<OR, byte>::_
> RawWrite(OR& dst, const T& value)
{
	RawWriteFrom(dst, &value, sizeof(T));
}

}}
