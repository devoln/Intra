#pragma once

#include "Range/ForwardDecls.h"
#include "Range/Decorators/Cycle.h"
#include "Cpp/Warnings.h"
#include "Concepts/Range.h"
#include "Cpp/Intrinsics.h"

namespace Intra { namespace Range {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename T, typename R> Meta::EnableIf<
	Concepts::IsAssignableRange<R>::_ &&
	!Concepts::IsInfiniteRange<R>::_ &&
	!Meta::IsConst<R>::_ &&
	Meta::IsConvertible<T, Concepts::ValueTypeOf<R>>::_
> FillAdvance(R& range, const T& value)
{
	while(!range.Empty())
	{
		range.First() = value;
		range.PopFirst();
	}
}



template<typename R, typename PR,
	typename AsPR = Concepts::RangeOfType<PR>
> Meta::EnableIf<
	Concepts::IsAssignableRange<R>::_ &&
	!Concepts::IsInfiniteRange<R>::_ &&
	!Meta::IsConst<R>::_ &&
	(Concepts::IsForwardRange<AsPR>::_ ||
		Concepts::IsInfiniteRange<AsPR>::_)
> FillPatternAdvance(R& range, PR&& pattern)
{
	auto patternCopy = Cycle(Range::Forward<PR>(pattern));
	while(!range.Empty())
	{
		range.First() = patternCopy.First();
		range.PopFirst();
		patternCopy.PopFirst();
	}
}

template<typename T, typename R,
	typename AsR = Concepts::RangeOfType<R>
> forceinline Meta::EnableIf<
	Concepts::IsConsumableRange<AsR>::_ &&
	Concepts::IsAssignableRange<AsR>::_ &&
	Meta::IsConvertible<T, Concepts::ValueTypeOf<AsR>>::_
> Fill(R&& range, const T& value)
{
	auto dst = Range::Forward<R>(range);
	FillAdvance(dst, value);
}


template<typename R, typename AsR> forceinline Meta::EnableIf<
	Concepts::IsConsumableRange<AsR>::_ &&
	Concepts::IsAssignableRange<AsR>::_ &&
	!(Concepts::IsArrayClass<R>::_ &&
		Meta::IsAlmostPod<Concepts::ElementTypeOfArray<R>>::_)
> FillZeros(R&& range)
{Fill(Range::Forward<R>(range), Concepts::ValueTypeOf<AsR>(0));}

template<typename R> forceinline Meta::EnableIf<
	Concepts::IsArrayClass<R>::_ &&
	Meta::IsAlmostPod<Concepts::ElementTypeOfArray<R>>::_
> FillZeros(R&& dst)
{
	auto data = Concepts::DataOf(dst);
	C::memset(data, 0, Concepts::LengthOf(dst)*sizeof(*data));
}

template<typename R, typename PR> forceinline Meta::EnableIf<
	Concepts::IsAsConsumableRange<R>::_ &&
	Concepts::IsAsAccessibleRange<PR>::_
> FillPattern(R&& range, PR&& pattern)
{
	auto dst = Range::Forward<R>(range);
	FillPatternAdvance(dst, Range::Forward<PR>(pattern));
}

INTRA_WARNING_POP

}}

