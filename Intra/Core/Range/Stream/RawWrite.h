#pragma once

#include "Core/Range/Concepts.h"
#include "Core/Range/Mutation/Copy.h"

INTRA_BEGIN
template<typename OR> constexpr forceinline Requires<
	COutputRangeOf<OR, char> ||
	COutputRangeOf<OR, byte>,
size_t> RawWriteFrom(OR& dst, const char* src, size_t n)
{
	return WriteTo(CSpan<char>(src, n), dst);
}

template<typename OR> constexpr forceinline Requires<
	COutputRangeOf<OR, char> ||
	COutputRangeOf<OR, byte>,
size_t> RawWriteFrom(OR& dst, const byte* src, size_t n)
{
	return WriteTo(CSpan<byte>(src, n), dst);
}

template<typename OR> forceinline Requires<
	COutputRangeOf<OR, char> ||
	COutputRangeOf<OR, byte>,
	size_t> RawWriteFrom(OR& dst, const void* src, size_t n)
{
	return WriteTo(CSpanOfRaw<char>(src, n), dst);
}

template<typename OR, typename T> constexpr Requires<
	COutputRangeOf<OR, char> ||
	COutputRangeOf<OR, byte>,
size_t> RawWriteFrom(OR& dst, CSpan<T> src)
{
	const size_t srcLen = src.Length()*sizeof(T);
	return RawWriteFrom(dst, src.Data(), srcLen);
}

template<typename OR, typename T> constexpr Requires<
	COutputRangeOf<OR, char> ||
	COutputRangeOf<OR, byte>,
size_t> RawWriteFrom(OR& dst, Span<T> src, size_t n)
{
	const size_t srcLen = src.Length()*sizeof(T);
	if(n > srcLen) n = srcLen;
	return RawWriteFrom(dst, src.Data(), n);
}

template<typename OR, typename T> constexpr Requires<
	COutputRangeOf<OR, char> ||
	COutputRangeOf<OR, byte>,
size_t> RawWriteFrom(OR& dst, const T& src)
{
	const size_t srcLen = src.Length()*sizeof(T);
	return RawWriteFrom(dst, &src, srcLen);
}


template<typename T, typename OR> constexpr Requires<
	COutputRangeOf<OR, char> ||
	COutputRangeOf<OR, byte>
> RawWrite(OR& dst, const T& value)
{
	RawWriteFrom(dst, &value, sizeof(T));
}
INTRA_END
