#pragma once

#include "Intra/Range/Concepts.h"
#include "Intra/Range/Mutation/Copy.h"

INTRA_BEGIN
template<typename OR> constexpr Requires<
	COutputOf<OR, char> ||
	COutputOf<OR, byte>,
index_t> RawWriteFrom(OR& dst, const char* src, Size n)
{
	return WriteTo(SpanOfPtr(src, n), dst);
}

template<typename OR> constexpr Requires<
	COutputOf<OR, char> ||
	COutputOf<OR, byte>,
index_t> RawWriteFrom(OR& dst, const byte* src, Size n)
{
	return WriteTo(SpanOfPtr(src, n), dst);
}

template<typename OR> Requires<
	COutputOf<OR, char> ||
	COutputOf<OR, byte>,
index_t> RawWriteFrom(OR& dst, const void* src, Size n)
{
	return WriteTo(CSpanOfRaw<char>(src, size_t(n)), dst);
}

template<typename OR, typename T> constexpr Requires<
	COutputOf<OR, char> ||
	COutputOf<OR, byte>,
index_t> RawWriteFrom(OR& dst, CSpan<T> src)
{
	return RawWriteFrom(dst, src.Data(), size_t(src.Length())*sizeof(T));
}

template<typename OR, typename T> constexpr Requires<
	COutputOf<OR, char> ||
	COutputOf<OR, byte>,
index_t> RawWriteFrom(OR& dst, Span<T> src, Size n)
{
	const auto srcLen = src.Length()*sizeof(T);
	if(n > srcLen) n = srcLen;
	return RawWriteFrom(dst, src.Data(), n);
}

template<typename OR, typename T> constexpr Requires<
	COutputOf<OR, char> ||
	COutputOf<OR, byte>,
index_t> RawWriteFrom(OR& dst, const T& src)
{
	const auto srcLen = src.Length()*sizeof(T);
	return RawWriteFrom(dst, &src, srcLen);
}


template<typename T, typename OR> constexpr Requires<
	COutputOf<OR, char> ||
	COutputOf<OR, byte>
> RawWrite(OR& dst, const T& value)
{
	RawWriteFrom(dst, &value, sizeof(T));
}
INTRA_END
