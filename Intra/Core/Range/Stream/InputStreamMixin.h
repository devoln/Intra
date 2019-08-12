#pragma once

#include "Core/Range/Span.h"
#include "Core/Range/StringView.h"

#include "Core/CArray.h"
#include "Core/Range/Concepts.h"


#include "Core/Type.h"

#include "Core/Range/Mutation/Fill.h"
#include "Core/Range/Mutation/Copy.h"
#include "Core/Range/Mutation/CopyUntil.h"
#include "Core/Range/Operations.h"
#include "Core/Range/ByLine.h"
#include "Core/Range/ByLineTo.h"
#include "Core/Range/Stream/RawRead.h"

INTRA_BEGIN
namespace Range {

template<typename R, typename T> struct InputStreamMixin
{
	typedef TRemoveConstRef<T> ElementType;

	template<typename U> Requires<
		CPod<U>,
	size_t> RawReadWrite(Span<U>& dst, size_t maxElementsToRead)
	{
		auto dst1 = dst.Take(maxElementsToRead).template Reinterpret<T>();
		size_t elementsRead = ReadWrite(*static_cast<R*>(this), dst1)*sizeof(T)/sizeof(U);
		dst.Begin += elementsRead;
		return elementsRead;
	}

	template<typename R1, typename U = TValueTypeOf<R1>> Requires<
		COutputRange<R1> &&
		CArrayClass<R1> &&
		CPod<U>,
	size_t> RawReadWrite(R1& dst, size_t maxElementsToRead)
	{
		Span<U> dst1 = dst;
		size_t result = RawReadWrite(dst1, maxElementsToRead);
		PopFirstExactly(dst, result);
		return result;
	}

	template<typename R1,
		typename U = TArrayElement<R1>
	> forceinline Requires<
		COutputRange<R1> &&
		CPod<U>,
	size_t> RawReadWrite(R1& dst)
	{return RawReadWrite(dst, dst.Length());}

	forceinline size_t RawReadTo(void* dst, size_t bytes)
	{return RawReadTo(SpanOfRaw<char>(dst, bytes));}


	template<typename R1,
		typename AsR1 = TRangeOfType<R1>,
		typename U = TValueTypeOf<AsR1>
	> forceinline Requires<
		COutputRange<AsR1> &&
		CArrayClass<AsR1> &&
		CPod<U>,
	size_t> RawReadTo(R1&& dst)
	{
		Span<U> range = ForwardAsRange<R1>(dst);
		return RawReadWrite(range);
	}

	template<typename U> forceinline Requires<
		CPod<U>
	> RawRead(U& dst)
	{RawReadTo(Span<U>(&dst, 1u));}

	template<typename U> forceinline U RawRead()
	{
		U result;
		RawRead(result);
		return result;
	}

	template<typename X> INTRA_CONSTEXPR2 GenericStringView<const ElementType> ReadUntilAdvance(const X& x, Span<ElementType>& buf)
	{
		const auto oldBuf = buf;
		const size_t len = ReadWriteUntil(*static_cast<R*>(this), buf, x);
		return {oldBuf.Data(), len};
	}

	template<typename X> INTRA_NODISCARD INTRA_CONSTEXPR2 GenericStringView<const ElementType> ReadUntil(const X& x, Span<ElementType> dstBuf)
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
			const size_t len = ReadWriteUntil(me, bufR, x);
			result += GenericStringView<const ElementType>(buf, len);
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
			const size_t len = ReadWriteUntil(me, bufR, pred);
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

	INTRA_NODISCARD INTRA_CONSTEXPR2 GenericStringView<const ElementType> ReadLine(Span<ElementType> dstBuf)
	{
		auto result = ReadUntil('\r', dstBuf);
		auto& me = *static_cast<R*>(this);
		if(!me.Empty() && me.First() == '\r') me.PopFirst();
		if(!me.Empty() && me.First() == '\n') me.PopFirst();
		return result;
	}

	template<typename STR> INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline RByLine<R, STR> ByLine()
	{return {Move(*static_cast<R*>(this)), false};}

	template<typename STR> INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline RByLine<R, STR> ByLine(Tags::TKeepTerminator)
	{return {Move(*static_cast<R*>(this)), true};}

	INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline RByLineTo<R> ByLine(GenericStringView<char> buf)
	{return {Move(*static_cast<R*>(this)), buf, false};}

	forceinline RByLineTo<R> ByLine(GenericStringView<char> buf, Tags::TKeepTerminator)
	{return {Move(*static_cast<R*>(this)), buf, true};}

	template<size_t N> INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline RByLineTo<R> ByLine(char(&buf)[N])
	{return ByLine(GenericStringView<char>::FromBuffer(buf));}

	template<size_t N> INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline RByLineTo<R> ByLine(char(&buf)[N], Tags::TKeepTerminator)
	{return ByLine(GenericStringView<char>::FromBuffer(buf), Tags::KeepTerminator);}
};

}
INTRA_END
