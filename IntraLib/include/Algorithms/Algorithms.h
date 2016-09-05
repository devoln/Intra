#pragma once

#include "Algorithms/RangeConcept.h"
#include "Range.h"

namespace Intra { namespace Algo {

template<typename T> void Add(ArrayRange<T> dstOp1, ArrayRange<const T> op2);
template<typename T> void Add(ArrayRange<T> dest, ArrayRange<const T> op1, ArrayRange<const T> op2);

template<typename T> void Multiply(ArrayRange<T> dstOp1, ArrayRange<const T> op2);
template<typename T> void Multiply(ArrayRange<T> dest, ArrayRange<const T> op1, ArrayRange<const T> op2);
template<typename T> void Divide(ArrayRange<T> dstOp1, ArrayRange<const T> op2);
template<typename T> void Divide(ArrayRange<T> dest, ArrayRange<const T> op1, ArrayRange<const T> op2);

template<typename T> void Subtract(ArrayRange<T> dstOp1, ArrayRange<const T> op2);
template<typename T> void Subtract(ArrayRange<T> dest, ArrayRange<const T> op1, ArrayRange<const T> op2);



template<typename T> void Add(ArrayRange<T> dstOp1, T op2);
template<typename T> void Add(ArrayRange<T> dest, ArrayRange<const T> op1, T op2);
template<typename T> void Multiply(ArrayRange<T> dstOp1, T op2);
template<typename T> void Multiply(ArrayRange<T> dest, ArrayRange<const T> op1, T op2);
template<typename T> void Divide(ArrayRange<T> dstOp1, T op2);
template<typename T> void Divide(ArrayRange<T> dest, ArrayRange<const T> op1, T op2);
template<typename T> void MulAdd(ArrayRange<T> dstOp1, T mul, T add);
template<typename T> void MulAdd(ArrayRange<T> dest, ArrayRange<const T> op1, T mul, T add);



template<typename T> T Minimum(ArrayRange<const T> arr);
template<typename T> T Maximum(ArrayRange<const T> arr);
template<typename T> void MiniMax(ArrayRange<const T> arr, T* minimum, T* maximum);


template<typename T, typename C> auto Find(const C& container, const T& element) -> decltype(begin(container))
{
	auto it = begin(container);
	while(it != end(container))
	{
		if(*it==element) return it;
		++it;
	}
	return it;
}



//! Скопировать нескольких массивов src в один массив dst с чередующимися элементами:
//! dst = {src[0][0], ..., src.Last()[0], src[0][1], ..., src.Last()[1], ...}
template<typename T> void Interleave(ArrayRange<T> dst, ArrayRange<const ArrayRange<const T>> src);

//! Скопировать массив src с чередующимися элементами в несколько отдельных массивов:
//! dst = {src[0][0], ..., src.Last()[0], src[0][1], ..., src.Last()[1], ...}
template<typename T> void Deinterleave(ArrayRange<const ArrayRange<T>> dst, ArrayRange<const T> src);

//! Скопировать элементы массива src в массив dst с приведением типов из From в To
template<typename To, typename From> void Cast(ArrayRange<To> dst, ArrayRange<const From> src);

//! Нормировать массив делением каждого элемента на NumericLimits<From>::Max
template<typename To, typename From> Meta::EnableIf<
	Meta::IsFloatType<To>::_ && Meta::IsIntegralType<From>::_
> CastToNormalized(ArrayRange<To> dst, ArrayRange<const From> src);

//! Заполнить массив dst элементами из src, умноженными на NumericLimits<To>::Max.
//! Предполагается, что -1.0 <= src[i] <= 1.0. Иначе произойдёт переполнение.
template<typename To, typename From> Meta::EnableIf<
	Meta::IsFloatType<From>::_ && Meta::IsIntegralType<To>::_
> CastFromNormalized(ArrayRange<To> dst, ArrayRange<const From> src);






//Оптимизированные специализации:

template<> void Add(ArrayRange<float> dstOp1, ArrayRange<const float> op2);
template<> void MulAdd(ArrayRange<float> dstOp1, float mul, float add);

template<> float Minimum(ArrayRange<const float> arr);
template<> float Maximum(ArrayRange<const float> arr);
template<> void MiniMax(ArrayRange<const float> arr, float* minimum, float* maximum);

template<> void Cast(ArrayRange<short> dst, ArrayRange<const float> src);

}}


#include "Algorithms.inl"
