#pragma once

#include "Intra/Range.h"

namespace Intra { INTRA_BEGIN
template<typename R, typename T> struct OutputStreamMixin
{
	template<CTriviallyCopyable U> constexpr index_t RawWriteFromAdvance(Span<U>& src, ClampedSize maxElementsToRead)
	{
		auto src1 = src.Take(maxElementsToRead).template ReinterpretUnsafe<const T>();
		const auto elementsRead = index_t(size_t(src1|StreamTo(*static_cast<R*>(this)))*sizeof(T)/sizeof(U));
		src.Begin += elementsRead;
		return elementsRead;
	}

	template<CTriviallyCopyable U> constexpr index_t RawWriteFromAdvance(Span<U>& src)
	{return RawWriteFromAdvance(src, src.Length());}

	template<CTriviallyCopyable U> constexpr index_t RawWriteFrom(Span<U> src)
	{return RawWriteFromAdvance(src);}

	template<CTriviallyCopyable U> constexpr index_t RawWriteFrom(U* src, Size n)
	{return RawWriteFrom(Span<U>(src, n));}

	template<CConvertibleToSpan R1> requires (!CRange<R1>) && CTriviallyCopyable<TArrayListValue<R1>>
	constexpr index_t RawWriteFrom(R1&& src) {return RawWriteFrom(CSpanOf(src));}

	template<CTriviallyCopyable U> constexpr void RawWrite(const U& dst)
	{RawWriteFrom(SpanOfPtr(&dst, 1u));}

	template<typename... Args> constexpr R& Print(Args&&... args)
	{
		auto& me = *static_cast<R*>(this);
		(void)TExpand{(static_cast<void>(me << INTRA_FWD(args)), '\0')...};
		return me;
	}

	template<typename... Args> constexpr R& PrintLine(Args&&... args)
	{return Print(INTRA_FWD(args)..., "\r\n");}
};
} INTRA_END
