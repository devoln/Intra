#pragma once

#include "Intra/Concepts.h"
#include "Intra/Range/Span.h"
#include "Intra/Range/StringView.h"
#include "Intra/Range/Concepts.h"
#include "Intra/Range/Mutation/Fill.h"
#include "Intra/Range/Mutation/Copy.h"
#include "Intra/Range/Mutation/CopyUntil.h"
#include "Intra/Range/Operations.h"
#include "Intra/Range/ByLine.h"
#include "Intra/Range/Stream/RawRead.h"

INTRA_BEGIN
template<typename R, typename T> struct InputStreamMixin
{
	typedef TRemoveConstRef<T> ElementType;

	template<typename U> Requires<
		CTriviallyCopyable<U>,
	index_t> RawReadWrite(Span<U>& dst, ClampedSize maxElementsToRead)
	{
		auto dst1 = dst.Take(maxElementsToRead).template ReinterpretUnsafe<T>();
		const auto elementsRead = index_t(size_t(ReadWrite(*static_cast<R*>(this), dst1))*sizeof(T)/sizeof(U));
		dst.Begin += elementsRead;
		return elementsRead;
	}

	template<typename R1, typename U = TValueTypeOf<R1>> Requires<
		COutputRange<R1> &&
		CArrayClass<R1> &&
		CTriviallyCopyable<U>,
	index_t> RawReadWrite(R1& dst, ClampedSize maxElementsToRead)
	{
		Span<U> dst1 = dst;
		const auto result = RawReadWrite(dst1, maxElementsToRead);
		PopFirstExactly(dst, result);
		return result;
	}

	template<typename R1,
		typename U = TArrayElement<R1>
	> Requires<
		COutputRange<R1> &&
		CTriviallyCopyable<U>,
	index_t> RawReadWrite(R1& dst)
	{return RawReadWrite(dst, dst.Length());}

	index_t RawReadTo(void* dst, Size bytes)
	{return RawReadTo(SpanOfRaw<char>(dst, bytes));}


	template<typename R1,
		typename AsR1 = TRangeOfRef<R1>,
		typename U = TValueTypeOf<AsR1>
	> Requires<
		COutputRange<AsR1> &&
		CArrayClass<AsR1> &&
		CTriviallyCopyable<U>,
	index_t> RawReadTo(R1&& dst)
	{
		Span<U> range = ForwardAsRange<R1>(dst);
		return RawReadWrite(range);
	}

	template<typename U> Requires<
		CTriviallyCopyable<U>
	> RawRead(U& dst)
	{RawReadTo(SpanOfPtr(&dst, 1u));}

	template<typename U> U RawRead()
	{
		U result;
		RawRead(result);
		return result;
	}

	template<typename X> constexpr GenericStringView<const ElementType> ReadUntilAdvance(const X& x, Span<ElementType>& buf)
	{
		const auto oldBuf = buf;
		const auto len = ReadWriteUntil(*static_cast<R*>(this), buf, x).ElementsRead;
		return SpanOfPtr(oldBuf.Data(), len);
	}

	template<typename X> [[nodiscard]] constexpr GenericStringView<const ElementType> ReadUntil(const X& x, Span<ElementType> dstBuf)
	{return ReadUntilAdvance(x, dstBuf);}

	template<typename STR, typename X> Requires<
		!CCallable<X, T>,
	STR> ReadUntil(const X& x)
	{
		STR result;
		auto& me = *static_cast<R*>(this);
		while(!me.Empty() && me.First()!=x)
		{
			ElementType buf[64];
			auto bufR = SpanBuffer(buf);
			const auto len = ReadWriteUntil(me, bufR, x).ElementsRead;
			result += GenericStringView<const ElementType>::FromPointerAndLength(buf, len);
		}
		return result;
	}

	template<typename STR, typename P> Requires<
		CCallable<P, T>,
	STR> ReadUntil(const P& pred)
	{
		STR result;
		auto& me = *static_cast<R*>(this);
		while(!me.Empty() && !pred(me.First()))
		{
			ElementType buf[64];
			auto bufR = SpanBuffer(buf);
			const index_t len = ReadWriteUntil(me, bufR, pred).ElementsRead;
			result += GenericStringView<const ElementType>(buf, len);
		}
		return result;
	}


	template<typename STR> STR ReadLine()
	{
		auto result = ReadUntil(IsLineSeparator);
		auto& me = *static_cast<R*>(this);
		if(!me.Empty())
		{
			auto c = me.First();
			me.PopFirst();
			if(c == '\n') return result;
		}
		if(!me.Empty() && me.First()=='\n') me.PopFirst();
		return result;
	}

	[[nodiscard]] constexpr GenericStringView<const ElementType> ReadLine(Span<ElementType> dstBuf)
	{
		auto result = ReadUntil('\r', dstBuf);
		auto& me = *static_cast<R*>(this);
		if(!me.Empty() && me.First() == '\r') me.PopFirst();
		if(!me.Empty() && me.First() == '\n') me.PopFirst();
		return result;
	}

	[[nodiscard]] constexpr RByLine<R> ByLine(Span<char> buf)
	{return {Move(*static_cast<R*>(this)), buf, false};}

	RByLine<R> ByLine(Span<char> buf, Tags::TKeepTerminator)
	{return {Move(*static_cast<R*>(this)), buf, true};}

	template<index_t N> [[nodiscard]] constexpr RByLine<R> ByLine(char(&buf)[N])
	{return ByLine(SpanOfBuffer(buf));}

	template<index_t N> [[nodiscard]] constexpr RByLine<R> ByLine(char(&buf)[N], Tags::TKeepTerminator)
	{return ByLine(SpanOfBuffer(buf), Tags::KeepTerminator);}
};
INTRA_END
