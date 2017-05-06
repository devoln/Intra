#pragma once

#include "Cpp/Warnings.h"
#include "Utils/Span.h"
#include "Concepts/Range.h"
#include "Concepts/RangeOf.h"

namespace Intra { namespace Range {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename T> T Minimum(CSpan<T> arr)
{
	INTRA_DEBUG_ASSERT(!arr.Empty());
	T result = arr.First();
	arr.PopFirst();
	while(!arr.Empty())
	{
		if(arr.First()<result) result = arr.First();
		arr.PopFirst();
	}
	return result;
}

template<typename T> T Maximum(CSpan<T> arr)
{
	INTRA_DEBUG_ASSERT(!arr.Empty());
	T result = arr.First();
	arr.PopFirst();
	while(!arr.Empty())
	{
		if(result<arr.First()) result = arr.First();
		arr.PopFirst();
	}
	return result;
}

template<typename T> void MiniMax(CSpan<T> arr, T* oMinimum, T* oMaximum)
{
	if(oMinimum==null)
	{
		if(oMaximum!=null) *oMaximum = Maximum(arr);
		return;
	}
	if(oMaximum==null)
	{
		*oMinimum = Minimum(arr);
		return;
	}

	INTRA_DEBUG_ASSERT(!arr.Empty());
	*oMaximum = *oMinimum = arr.First();
	arr.PopFirst();

	while(!arr.Empty())
	{
		if(arr.First()<*oMinimum) *oMinimum = arr.First();
		if(*oMaximum<arr.First()) *oMaximum = arr.First();
		arr.PopFirst();
	}
}

//Оптимизированные специализации
template<> float Minimum(CSpan<float> arr);
template<> float Maximum(CSpan<float> arr);
template<> void MiniMax(CSpan<float> arr, float* minimum, float* maximum);


template<typename R, typename F, typename S> Meta::EnableIf<
	Concepts::IsConsumableRange<R>::_,
S> ReduceAdvance(R& range, const F& func, const S& seed)
{
	auto result = seed;
	while(!range.Empty())
	{
		result = func(result, range.First());
		range.PopFirst();
	}
	return result;
}

template<typename R, typename F,
	typename T = Concepts::ValueTypeOf<R>
> Meta::EnableIf<
	Concepts::IsInputRange<R>::_ &&
	!Meta::IsConst<R>::_,
Meta::ResultOf<F, T, T>> ReduceAdvance(R& range, F func)
{
	Meta::ResultOf<F, T, T> seed = range.First();
	range.PopFirst();
	return ReduceAdvance(range, func, seed);
}

template<typename R, typename F, typename S> forceinline Meta::EnableIf<
	Concepts::IsAsConsumableRange<R>::_,
S> Reduce(R&& range, const F& func, const S& seed)
{
	auto rangeCopy = Range::Forward<R>(range);
	return ReduceAdvance(rangeCopy, func, seed);
}

template<typename R, typename F,
	typename AsR = Concepts::RangeOfType<R>,
	typename T = Concepts::ValueTypeOf<AsR>,	
typename = Meta::EnableIf<
	Concepts::IsAsForwardRange<R>::_
>> forceinline Meta::ResultOf<F, T, T> Reduce(R&& range, F func)
{
	auto rangeCopy = Range::Forward<R>(range);
	return ReduceAdvance(rangeCopy, func);
}

INTRA_WARNING_POP

}}
