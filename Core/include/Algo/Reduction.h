#pragma once

#include "Platform/CppWarnings.h"
#include "Range/Generators/ArrayRange.h"
#include "Range/Concepts.h"
#include "Range/AsRange.h"

namespace Intra { namespace Algo {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename T> T Minimum(ArrayRange<const T> arr)
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

template<typename T> T Maximum(ArrayRange<const T> arr)
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

template<typename T> void MiniMax(ArrayRange<const T> arr, T* oMinimum, T* oMaximum)
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
template<> float Minimum(ArrayRange<const float> arr);
template<> float Maximum(ArrayRange<const float> arr);
template<> void MiniMax(ArrayRange<const float> arr, float* minimum, float* maximum);


template<typename R, typename F, typename S> Meta::EnableIf<
	Range::IsFiniteInputRange<R>::_ && !Meta::IsConst<R>::_,
S> ReduceAdvance(R&& range, const F& func, const S& seed)
{
	auto result = seed;
	while(!range.Empty())
	{
		result = func(result, range.First());
		range.PopFirst();
	}
	return result;
}

template<typename R, typename F> Meta::EnableIf<
	IsInputRange<R>::_ && !Meta::IsConst<R>::_,
Meta::ResultOf<F, ValueTypeOf<R>, ValueTypeOf<R>>> ReduceAdvance(R& range, F func)
{
	typedef Meta::ResultOf<F, ValueTypeOf<R>, ValueTypeOf<R>> ResultType;
	ResultType seed = range.First();
	range.PopFirst();
	return ReduceAdvance(range, func, seed);
}

template<typename R, typename F, typename S> forceinline Meta::EnableIf<
	IsAsConsumableRange<R>::_,
S> Reduce(R&& r, const F& func, const S& seed)
{
	auto range = Range::Forward<R>(r);
	return ReduceAdvance(range, func, seed);
}

template<typename R, typename F, typename = Meta::EnableIf<Range::IsForwardRange<R>::_>> forceinline
Meta::ResultOf<F, ValueTypeOf<R>, ValueTypeOf<R>> Reduce(const R& range, F func)
{return ReduceAdvance(R(range), func);}

template<typename T, size_t N, typename F, typename S> forceinline
S Reduce(const T(&arr)[N], const F& func, const S& seed)
{return Reduce(AsRange(arr), func, seed);}

template<typename T, size_t N, typename F> forceinline
Meta::ResultOf<F, T, T> Reduce(const T(&arr)[N], F func)
{return Reduce(AsRange(arr), func);}

INTRA_WARNING_POP

}}
