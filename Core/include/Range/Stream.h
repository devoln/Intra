#pragma once

#include "Algo/String/ToString.h"
#include "Algo/String/Parse.h"
#include "Algo/Mutation/Copy.h"
#include "Algo/Mutation/Fill.h"
#include "Range/Generators/ArrayRange.h"
#include "Range/Concepts.h"
#include "Range/AsRange.h"

namespace Intra { namespace Range {

using Algo::operator<<;
using Algo::operator>>;

template<typename R, typename T> struct StreamMixin
{
	template<typename U> Meta::EnableIf<
		Meta::IsTriviallySerializable<U>::_,
	size_t> ReadRawToAdvance(ArrayRange<U>& dst, size_t maxElementsToRead)
	{
		auto dst1 = dst.Take(maxElementsToRead).template Reinterpret<T>();
		size_t elementsRead = Algo::CopyAdvanceToAdvance(*static_cast<R*>(this), dst1)*sizeof(T)/sizeof(U);
		dst.Begin += elementsRead;
		return elementsRead;
	}

	template<typename U> forceinline Meta::EnableIf<
		Meta::IsTriviallySerializable<U>::_,
	size_t> ReadRawToAdvance(ArrayRange<U>& dst)
	{return ReadRawToAdvance(dst, dst.Length());}

	template<typename U> forceinline Meta::EnableIf<
		Meta::IsTriviallySerializable<U>::_,
	size_t> ReadRawTo(ArrayRange<U> dst)
	{
		size_t result = ReadRawToAdvance(dst);
		Algo::FillZeros(dst);
		return result;
	}

	template<typename R1, typename AsR1=AsRangeResult<R1>> forceinline Meta::EnableIf<
		!IsInputRange<R1>::_ && IsArrayRange<AsR1>::_ &&
		Meta::IsTriviallySerializable<ValueTypeOf<AsR1>>::_,
	size_t> ReadRawTo(R1&& dst)
	{return ReadRawTo(Range::Forward<R1>(dst));}

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
};

}}
