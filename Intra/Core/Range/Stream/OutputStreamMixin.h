#pragma once

#include "Core/Range/Concepts.h"
#include "Core/Range/Mutation/Copy.h"
#include "Core/Range/Span.h"

INTRA_BEGIN
template<typename R, typename T> struct OutputStreamMixin
{
	template<typename U> constexpr Requires<
		CPod<U>,
	index_t> RawWriteFromAdvance(Span<U>& src, index_t maxElementsToRead)
	{
		auto src1 = src.Take(maxElementsToRead).template Reinterpret<const T>();
		index_t elementsRead = ReadWrite(src1, *static_cast<R*>(this))*sizeof(T)/sizeof(U);
		src.Begin += elementsRead;
		return elementsRead;
	}

	template<typename U> constexpr forceinline Requires<
		CPod<U>,
	index_t> RawWriteFromAdvance(Span<U>& src)
	{return RawWriteFromAdvance(src, src.Length());}

	template<typename U> constexpr forceinline Requires<
		CPod<U>,
	index_t> RawWriteFrom(Span<U> src)
	{return RawWriteFromAdvance(src);}

	template<typename U> constexpr forceinline Requires<
		CPod<U>,
	index_t> RawWriteFrom(U* src, index_t n)
	{return RawWriteFrom(Span<U>(src, n));}

	template<typename R1> constexpr forceinline Requires<
		!CInputRange<R1> &&
		CArrayClass<R1> &&
		CPod<TArrayElement<R1>>,
	index_t> RawWriteFrom(R1&& src)
	{return RawWriteFrom(CSpanOf(src));}

	template<typename U> constexpr forceinline Requires<
		CPod<U>
	> RawWrite(const U& dst)
	{RawWriteFrom(CSpan<U>(&dst, 1u));}

	template<typename... Args> constexpr R& Print(Args&&... args)
	{
		auto& me = *static_cast<R*>(this);
		TExpand{(static_cast<void>(me << Forward<Args>(args)), '\0')...};
		return me;
	}

	template<typename... Args> constexpr R& PrintLine(Args&&... args)
	{return Print(Forward<Args>(args)..., "\r\n");}
};
INTRA_END
