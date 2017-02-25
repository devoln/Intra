#pragma once

#include "Range/ForwardDecls.h"
#include "Range/Decorators/Cycle.h"
#include "Platform/CppWarnings.h"
#include "Range/Concepts.h"
#include "Platform/Intrinsics.h"

namespace Intra { namespace Algo {

using namespace Range::Concepts;

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename T, typename R> Meta::EnableIf<
	IsAssignableRange<R>::_ && !IsInfiniteRange<R>::_ && !Meta::IsConst<R>::_ &&
	Meta::IsConvertible<T, ValueTypeOf<R>>::_
> FillAdvance(R& range, const T& value)
{
	while(!range.Empty())
	{
		range.First() = value;
		range.PopFirst();
	}
}



template<typename R, typename PR> Meta::EnableIf<
	IsAssignableRange<R>::_ && !IsInfiniteRange<R>::_ && !Meta::IsConst<R>::_ &&
	(IsAsForwardRange<PR>::_ || IsAsInfiniteRange<PR>::_)
> FillPatternAdvance(R& range, PR&& pattern)
{
	auto patternCopy = Range::Cycle(Range::Forward<PR>(pattern));
	while(!range.Empty())
	{
		range.First() = patternCopy.First();
		range.PopFirst();
		patternCopy.PopFirst();
	}
}

template<typename T, typename R, typename AsR = AsRangeResult<R>> forceinline Meta::EnableIf<
	IsConsumableRange<AsR>::_ && IsAssignableRange<AsR>::_ &&
	Meta::IsConvertible<T, ValueTypeOf<AsR>>::_
> Fill(R&& range, const T& value)
{
	auto dst = Range::Forward<R>(range);
	FillAdvance(dst, value);
}


template<typename R, typename AsR> forceinline Meta::EnableIf<
	IsConsumableRange<AsR>::_ && IsAssignableRange<AsR>::_ &&
	!(IsArrayRange<AsR>::_ && Meta::IsAlmostPod<ValueTypeOf<AsR>>::_)
> FillZeros(R&& range)
{Fill(Range::Forward<R>(range), ValueTypeOf<AsR>(0));}

template<typename R, typename AsR=AsRangeResult<R>> forceinline Meta::EnableIf<
	IsArrayRange<AsR>::_ && Meta::IsAlmostPod<ValueTypeOf<AsR>>::_
> FillZeros(R&& range)
{
	auto dst = Range::Forward<R>(range);
	C::memset(dst.Data(), 0, dst.Length()*sizeof(dst.First()));
}

template<typename R, typename PR> forceinline Meta::EnableIf<
	IsAsConsumableRange<R>::_ && IsAsAccessibleRange<PR>::_
> FillPattern(R&& range, PR&& pattern)
{
	auto dst = Range::Forward<R>(range);
	FillPatternAdvance(dst, Range::Forward<PR>(pattern));
}

INTRA_WARNING_POP

}}

