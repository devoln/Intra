#pragma once

#include "Intra/Range/Concepts.h"
#include "Intra/Range/Mutation/Copy.h"
#include "Intra/Range/Span.h"

namespace Intra { INTRA_BEGIN
template<typename R, typename T> struct OutputStreamMixin
{
	template<typename U> constexpr Requires<
		CTriviallyCopyable<U>,
	index_t> RawWriteFromAdvance(Span<U>& src, ClampedSize maxElementsToRead)
	{
		auto src1 = src.Take(maxElementsToRead).template ReinterpretUnsafe<const T>();
		const auto elementsRead = index_t(size_t(ReadWrite(src1, *static_cast<R*>(this)))*sizeof(T)/sizeof(U));
		src.Begin += elementsRead;
		return elementsRead;
	}

	template<typename U> constexpr Requires<
		CTriviallyCopyable<U>,
	index_t> RawWriteFromAdvance(Span<U>& src)
	{return RawWriteFromAdvance(src, src.Length());}

	template<typename U> constexpr Requires<
		CTriviallyCopyable<U>,
	index_t> RawWriteFrom(Span<U> src)
	{return RawWriteFromAdvance(src);}

	template<typename U> constexpr Requires<
		CTriviallyCopyable<U>,
	index_t> RawWriteFrom(U* src, Size n)
	{return RawWriteFrom(Span<U>(src, n));}

	template<typename R1> constexpr Requires<
		!CRange<R1> &&
		CArrayList<R1> &&
		CTriviallyCopyable<TArrayListValue<R1>>,
	index_t> RawWriteFrom(R1&& src)
	{return RawWriteFrom(CSpanOf(src));}

	template<typename U> constexpr Requires<
		CTriviallyCopyable<U>
	> RawWrite(const U& dst)
	{RawWriteFrom(SpanOfPtr(&dst, 1u));}

	template<typename... Args> constexpr R& Print(Args&&... args)
	{
		auto& me = *static_cast<R*>(this);
		(void)TExpand{(static_cast<void>(me << Forward<Args>(args)), '\0')...};
		return me;
	}

	template<typename... Args> constexpr R& PrintLine(Args&&... args)
	{return Print(Forward<Args>(args)..., "\r\n");}
};
} INTRA_END
