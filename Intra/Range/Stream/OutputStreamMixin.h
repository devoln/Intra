#pragma once

#include "Cpp/Features.h"
#include "Meta/Type.h"
#include "Range/Mutation/Copy.h"
#include "Utils/Span.h"
#include "Concepts/Range.h"
#include "Concepts/RangeOf.h"

namespace Intra { namespace Range {

template<typename R, typename T> struct OutputStreamMixin
{
	template<typename U> Meta::EnableIf<
		Meta::IsTriviallySerializable<U>::_,
	size_t> WriteRawFromAdvance(Span<U>& src, size_t maxElementsToRead)
	{
		auto src1 = src.Take(maxElementsToRead).template Reinterpret<const T>();
		size_t elementsRead = ReadToAdvance(src1, *static_cast<R*>(this))*sizeof(T)/sizeof(U);
		src.Begin += elementsRead;
		return elementsRead;
	}

	template<typename U> forceinline Meta::EnableIf<
		Meta::IsTriviallySerializable<U>::_,
	size_t> WriteRawFromAdvance(Span<U>& src)
	{return WriteRawFromAdvance(src, src.Length());}

	template<typename U> forceinline Meta::EnableIf<
		Meta::IsTriviallySerializable<U>::_,
	size_t> WriteRawFrom(Span<U> src)
	{return WriteRawFromAdvance(src);}

	template<typename U> forceinline Meta::EnableIf<
		Meta::IsTriviallySerializable<U>::_,
	size_t> WriteRawFrom(U* src, size_t n)
	{return WriteRawFrom(Span<U>(src, n));}

	template<typename R1> forceinline Meta::EnableIf<
		!Concepts::IsInputRange<R1>::_ &&
		Concepts::IsArrayClass<R1>::_ &&
		Meta::IsTriviallySerializable<Concepts::ElementTypeOfArray<R1>>::_,
	size_t> WriteRawFrom(R1&& dst)
	{return WriteRawFrom(CSpanOf(dst));}

	template<typename U> forceinline Meta::EnableIf<
		Meta::IsTriviallySerializable<U>::_
	> WriteRaw(const U& dst)
	{WriteRawFrom(CSpan<U>(&dst, 1u));}


	template<typename Arg0> forceinline R& Print(Arg0&& t)
	{return *static_cast<R*>(this) << Cpp::Forward<Arg0>(t);}

	template<typename Arg0, typename Arg1, typename... Args>
	R& Print(Arg0&& arg0, Arg1&& arg1, Args&&... args)
	{
		Print(Cpp::Forward<Arg0>(arg0));
		return Print(Cpp::Forward<Arg1>(arg1), Cpp::Forward<Args>(args)...);
	}

	template<typename... Args>
	R& PrintLine(Args&&... args)
	{
		return Print(Cpp::Forward<Args>(args)..., "\r\n");
	}
};


}}
