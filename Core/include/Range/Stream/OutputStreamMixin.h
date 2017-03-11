#pragma once

#include "Platform/CppFeatures.h"
#include "Meta/Type.h"
#include "Algo/Mutation/Copy.h"
#include "Range/Generators/ArrayRange.h"
#include "Range/Concepts.h"
#include "Range/AsRange.h"

namespace Intra { namespace Range {

template<typename R, typename T> struct OutputStreamMixin
{
	template<typename U> Meta::EnableIf<
		Meta::IsTriviallySerializable<U>::_,
	size_t> WriteRawFromAdvance(ArrayRange<U>& src, size_t maxElementsToRead)
	{
		auto src1 = src.Take(maxElementsToRead).template Reinterpret<const T>();
		size_t elementsRead = Algo::CopyAdvanceToAdvance(src1, *static_cast<R*>(this))*sizeof(T)/sizeof(U);
		src.Begin += elementsRead;
		return elementsRead;
	}

	template<typename U> forceinline Meta::EnableIf<
		Meta::IsTriviallySerializable<U>::_,
	size_t> WriteRawFromAdvance(ArrayRange<U>& src)
	{return WriteRawFromAdvance(src, src.Length());}

	template<typename U> forceinline Meta::EnableIf<
		Meta::IsTriviallySerializable<U>::_,
	size_t> WriteRawFrom(ArrayRange<U> src)
	{return WriteRawFromAdvance(src);}

	template<typename R1, typename AsR1=AsRangeResult<R1>> forceinline Meta::EnableIf<
		!IsInputRange<R1>::_ && IsArrayRange<AsR1>::_ &&
		Meta::IsTriviallySerializable<ValueTypeOf<AsR1>>::_,
	size_t> WriteRawFrom(R1&& dst)
	{return WriteRawFrom(Range::Forward<R1>(dst));}

	template<typename U> forceinline Meta::EnableIf<
		Meta::IsTriviallySerializable<U>::_
	> WriteRaw(const U& dst)
	{return WriteRawFrom(ArrayRange<const U>(&dst, 1u));}


	template<typename Arg0> forceinline void Print(Arg0&& t)
	{*static_cast<R*>(this) << Meta::Forward<Arg0>(t);}

	template<typename Arg0, typename Arg1, typename... Args>
	void Print(Arg0&& arg0, Arg1&& arg1, Args&&... args)
	{
		Print(Meta::Forward<Arg0>(arg0));
		Print(Meta::Forward<Arg1>(arg1), Meta::Forward<Args>(args)...);
	}

	template<typename... Args>
	void PrintLine(Args&&... args)
	{
		Print(Meta::Forward<Args>(args)..., "\r\n");
	}
};


}}
