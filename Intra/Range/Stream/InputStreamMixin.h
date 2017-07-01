#pragma once

#include "Cpp/Warnings.h"
#include "Cpp/Features.h"

#include "Utils/Span.h"
#include "Utils/StringView.h"

#include "Concepts/Array.h"
#include "Concepts/Range.h"
#include "Concepts/RangeOf.h"

#include "Meta/Type.h"

#include "Range/Mutation/Fill.h"
#include "Range/Mutation/Copy.h"
#include "Range/Mutation/CopyUntil.h"
#include "Range/Operations.h"
#include "Range/Decorators/ByLine.h"
#include "Range/Decorators/ByLineTo.h"
#include "Range/Stream/RawRead.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Range {

template<typename R, typename T> struct InputStreamMixin
{
	typedef Meta::RemoveConstRef<T> ElementType;

	template<typename U> Meta::EnableIf<
		Meta::IsTriviallySerializable<U>::_,
	size_t> RawReadWrite(Span<U>& dst, size_t maxElementsToRead)
	{
		auto dst1 = dst.Take(maxElementsToRead).template Reinterpret<T>();
		size_t elementsRead = ReadWrite(*static_cast<R*>(this), dst1)*sizeof(T)/sizeof(U);
		dst.Begin += elementsRead;
		return elementsRead;
	}

	template<typename R1, typename U = Concepts::ValueTypeOf<R1>> Meta::EnableIf<
		Concepts::IsOutputRange<R1>::_ &&
		Concepts::IsArrayClass<R1>::_ &&
		Meta::IsTriviallySerializable<U>::_,
	size_t> RawReadWrite(R1& dst, size_t maxElementsToRead)
	{
		Span<U> dst1 = dst;
		size_t result = RawReadWrite(dst1, maxElementsToRead);
		PopFirstExactly(dst, result);
		return result;
	}

	template<typename R1,
		typename U = Concepts::ElementTypeOfArray<R1>
	> forceinline Meta::EnableIf<
		Concepts::IsOutputRange<R1>::_ &&
		Meta::IsTriviallySerializable<U>::_,
	size_t> RawReadWrite(R1& dst)
	{return RawReadWrite(dst, dst.Length());}

	forceinline size_t RawReadTo(void* dst, size_t bytes)
	{return RawReadTo(SpanOfRaw<char>(dst, bytes));}


	template<typename R1,
		typename AsR1 = Concepts::RangeOfType<R1>,
		typename U = Concepts::ValueTypeOf<AsR1>
	> forceinline Meta::EnableIf<
		Concepts::IsOutputRange<AsR1>::_ &&
		Concepts::IsArrayClass<AsR1>::_ &&
		Meta::IsTriviallySerializable<U>::_,
	size_t> RawReadTo(R1&& dst)
	{
		Span<U> range = Range::Forward<R1>(dst);
		return RawReadWrite(range);
	}

	template<typename U> forceinline Meta::EnableIf<
		Meta::IsTriviallySerializable<U>::_
	> RawRead(U& dst)
	{RawReadTo(Span<U>(&dst, 1u));}

	template<typename U> forceinline U RawRead()
	{
		U result;
		RawRead(result);
		return result;
	}

	template<typename X> GenericStringView<const ElementType> ReadUntilAdvance(const X& x, Span<ElementType>& buf)
	{
		const auto oldBuf = buf;
		const size_t len = ReadWriteUntil(*static_cast<R*>(this), buf, x);
		return {oldBuf.Data(), len};
	}

	template<typename X> GenericStringView<const ElementType> ReadUntil(const X& x, Span<ElementType> dstBuf)
	{return ReadUntilAdvance(x, dstBuf);}

	template<typename STR, typename X> Meta::EnableIf<
		!Meta::IsCallable<X, T>::_,
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

	template<typename STR, typename P> Meta::EnableIf<
		Meta::IsCallable<P, T>::_,
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
		auto result = ReadUntil(Funal::IsLineSeparator);
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

	GenericStringView<const ElementType> ReadLine(Span<ElementType> dstBuf)
	{
		auto result = ReadUntil('\r', dstBuf);
		auto& me = *static_cast<R*>(this);
		if(!me.Empty() && me.First() == '\r') me.PopFirst();
		if(!me.Empty() && me.First() == '\n') me.PopFirst();
		return result;
	}

	template<typename STR> forceinline RByLine<R, STR> ByLine()
	{return {Cpp::Move(*static_cast<R*>(this)), false};}

	template<typename STR> forceinline RByLine<R, STR> ByLine(Tags::TKeepTerminator)
	{return {Cpp::Move(*static_cast<R*>(this)), true};}

	forceinline RByLineTo<R> ByLine(GenericStringView<char> buf)
	{return {Cpp::Move(*static_cast<R*>(this)), buf, false};}

	forceinline RByLineTo<R> ByLine(GenericStringView<char> buf, Tags::TKeepTerminator)
	{return {Cpp::Move(*static_cast<R*>(this)), buf, true};}

	template<size_t N> forceinline RByLineTo<R> ByLine(char(&buf)[N])
	{return ByLine(GenericStringView<char>::FromBuffer(buf));}

	template<size_t N> forceinline RByLineTo<R> ByLine(char(&buf)[N], Tags::TKeepTerminator)
	{return ByLine(GenericStringView<char>::FromBuffer(buf), Tags::KeepTerminator);}
};

}}

INTRA_WARNING_POP
