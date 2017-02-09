#pragma once

#include "Range/ForwardDecls.h"
#include "Range/Decorators/Cycle.h"
#include "Platform/CppWarnings.h"

namespace Intra { namespace Algo {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename T, typename R> Meta::EnableIf<
	Range::IsAssignableRange<R>::_ && !Range::IsInfiniteRange<R>::_ && !Meta::IsConst<R>::_ &&
	Meta::IsConvertible<T, Range::ValueTypeOf<R>>::_
> FillAdvance(R& range, const T& value)
{
	while(!range.Empty())
	{
		range.First() = value;
		range.PopFirst();
	}
}



template<typename R, typename PR> Meta::EnableIf<
	Range::IsAssignableRange<R>::_ && !Range::IsInfiniteRange<R>::_ && !Meta::IsConst<R>::_ &&
	(Range::IsAsForwardRange<PR>::_ || Range::IsAsInfiniteRange<PR>::_)
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

template<typename T, typename R> forceinline Meta::EnableIf<
	Range::IsAsNonInfiniteForwardRange<R>::_ &&
	Meta::IsConvertible<T, Range::ValueTypeOf<R>>::_
> Fill(R&& range, const T& value)
{
	auto dst = Range::Forward<R>(range);
	FillAdvance(dst, value);
}


template<typename R> forceinline Meta::EnableIf<
	Range::IsAsNonInfiniteForwardRange<R>::_ &&
	!(Range::IsAsArrayRange<R>::_ && Meta::IsAlmostPod<Range::ValueTypeOfAs<R>>::_)
> FillZeros(R&& range)
{Fill(Range::Forward<R>(range), Range::ValueTypeOf<R>(0));}

template<typename R> forceinline Meta::EnableIf<
	Range::IsAsArrayRange<R>::_ && Meta::IsAlmostPod<Range::ValueTypeOfAs<R>>::_
> FillZeros(R&& range)
{
	auto dst = Range::Forward<R>(range);
	C::memset(dst.Data(), 0, dst.Length()*sizeof(dst.First()));
}

template<typename R, typename PR> forceinline Meta::EnableIf<
	Range::IsAsConsumableRange<R>::_ &&
	Range::IsAsAccessibleRange<PR>::_
> FillPattern(R&& range, PR&& pattern)
{
	auto dst = Range::Forward<R>(range);
	FillPatternAdvance(dst, Range::Forward<PR>(pattern));
}

INTRA_WARNING_POP

}}

