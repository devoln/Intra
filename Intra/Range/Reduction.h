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

template<typename T> Pair<T> MiniMax(CSpan<T> arr)
{
	Pair<T> minmax;
	auto& arr0 = *arr.Begin++;
	if((arr.Length() & 1) == 0)
	{
		const auto& arr1 = *arr.Begin++;
		const bool less = arr1 < arr0;
		minmax.first = less? arr1: arr0;
		minmax.second = less? arr0: arr1;
	}
	else minmax.first = minmax.second = arr0;

	const auto end = arr.End - 1;
	while(arr.Begin < end)
	{
		if(arr.Begin[1] < arr.Begin[0])
		{
			if(minmax.second < arr.Begin[0]) minmax.second = *arr.Begin++;
			if(arr[1] < minmax.first) minmax.first = *++arr.Begin;
		}
		else
		{
			if(minmax.second < arr.Begin[1]) minmax.second = ++arr.Begin;
			if(arr.Begin[0] < minmax.first) minmax.first = *arr.Begin++;
		}
	}

	return minmax;
}

//Оптимизированные специализации
template<> float Minimum(CSpan<float> arr);
template<> float Maximum(CSpan<float> arr);
template<> Pair<float> MiniMax(CSpan<float> arr);


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
