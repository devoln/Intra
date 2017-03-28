#pragma once

#include "Platform/CppFeatures.h"
#include "Meta/Type.h"
#include "Algo/Mutation/Fill.h"
#include "Algo/Mutation/Copy.h"
#include "Algo/Mutation/CopyUntil.h"
#include "Range/Generators/ArrayRange.h"
#include "Range/Generators/StringView.h"
#include "Container/Sequential/String.h"
#include "Range/Concepts.h"
#include "Range/AsRange.h"
#include "Range/Operations.h"

namespace Intra { namespace Range {

template<typename R, typename T> struct InputStreamMixin
{
	typedef Meta::RemoveConstRef<T> ElementType;

	template<typename U> Meta::EnableIf<
		Meta::IsTriviallySerializable<U>::_,
	size_t> ReadRawToAdvance(ArrayRange<U>& dst, size_t maxElementsToRead)
	{
		auto dst1 = dst.Take(maxElementsToRead).template Reinterpret<T>();
		size_t elementsRead = Algo::CopyAdvanceToAdvance(*static_cast<R*>(this), dst1)*sizeof(T)/sizeof(U);
		dst.Begin += elementsRead;
		return elementsRead;
	}

	template<typename R1, typename U=ValueTypeOf<R1>> Meta::EnableIf<
		IsOutputRange<R1>::_ && IsArrayClass<R1>::_ &&
		Meta::IsTriviallySerializable<U>::_,
	size_t> ReadRawToAdvance(R1& dst, size_t maxElementsToRead)
	{
		ArrayRange<U> dst1 = dst;
		size_t result = ReadRawToAdvance(dst1, maxElementsToRead);
		PopFirstExactly(dst, result);
		return result;
	}

	template<typename R1, typename U=ValueTypeOf<R1>> forceinline Meta::EnableIf<
		IsOutputRange<R1>::_ && IsArrayClass<R1>::_ &&
		Meta::IsTriviallySerializable<U>::_,
	size_t> ReadRawToAdvance(R1& dst)
	{return ReadRawToAdvance(dst, dst.Length());}

	forceinline size_t ReadRawTo(void* dst, size_t bytes)
	{return ReadRawTo(ArrayRange<char>(reinterpret_cast<char*>(dst), bytes));}


	template<typename R1, typename AsR1=AsRangeResult<R1>, typename U=ValueTypeOf<AsR1>> forceinline Meta::EnableIf<
		IsOutputRange<AsR1>::_ && IsArrayClass<AsR1>::_ &&
		Meta::IsTriviallySerializable<U>::_,
	size_t> ReadRawTo(R1&& dst)
	{
		ArrayRange<U> range = Range::Forward<R1>(dst);
		return ReadRawToAdvance(range);
	}

	template<typename U> forceinline Meta::EnableIf<
		Meta::IsTriviallySerializable<U>::_
	> ReadRaw(U& dst)
	{ReadRawTo(ArrayRange<U>(&dst, 1u));}

	template<typename U> forceinline U ReadRaw()
	{
		U result;
		ReadRaw(result);
		return result;
	}

	template<typename X> GenericStringView<const ElementType> ReadUntilAdvance(const X& x, ArrayRange<ElementType>& buf)
	{
		const auto oldBuf = buf;
		const size_t len = Algo::CopyAdvanceToAdvanceUntil(*static_cast<R*>(this), buf, x);
		return {oldBuf.Begin, oldBuf.Begin + len};
	}

	template<typename X> GenericStringView<const ElementType> ReadUntil(const X& x, ArrayRange<ElementType> dstBuf)
	{return ReadUntilAdvance(x, dstBuf);}

	template<typename X> Meta::EnableIf<
		!Meta::IsCallable<X, T>::_,
	GenericString<ElementType>> ReadUntil(const X& x)
	{
		GenericString<ElementType> result;
		while(!static_cast<R*>(this)->Empty() && static_cast<R*>(this)->First()!=x)
		{
			ElementType buf[64];
			ArrayRange<ElementType> bufR = buf;
			const size_t len = Algo::CopyAdvanceToAdvanceUntil(*static_cast<R*>(this), bufR, x);
			result += GenericStringView<const ElementType>(buf, len);
		}
		return result;
	}

	template<typename P> Meta::EnableIf<
		Meta::IsCallable<P, T>::_,
	GenericString<ElementType>> ReadUntil(const P& pred)
	{
		GenericString<ElementType> result;
		auto& me = *static_cast<R*>(this);
		while(!me.Empty() && !pred(me.First()))
		{
			ElementType buf[64];
			ArrayRange<ElementType> bufR = buf;
			const size_t len = Algo::CopyAdvanceToAdvanceUntil(me, bufR, pred);
			result += GenericStringView<const ElementType>(buf, len);
		}
		return result;
	}


	GenericString<ElementType> ReadLine()
	{
		auto result = ReadUntil('\r');
		auto& me = *static_cast<R*>(this);
		if(!me.Empty()) me.PopFirst();
		if(!me.Empty() && me.First()=='\n') me.PopFirst();
		return result;
	}

	GenericStringView<const ElementType> ReadLine(ArrayRange<ElementType> dstBuf)
	{
		auto result = ReadUntil('\r', dstBuf);
		auto& me = *static_cast<R*>(this);
		if(!me.Empty() && me.First()=='\r') me.PopFirst();
		if(!me.Empty() && me.First()=='\n') me.PopFirst();
		return result;
	}
};

}}
