#pragma once
#include "Meta/Type.h"
#include "Algorithms/Operations.h"

namespace Intra { namespace Algo {

namespace Comparers
{
	template<typename T> using Function = bool(*)(const T& a, const T& b);

	template<typename COMPARER, typename T, typename I> struct Indexed
	{
		COMPARER comparer;
		ArrayRange<T> values;
		Indexed(COMPARER c, ArrayRange<T> vals): comparer(c), values(vals) {}
		bool operator()(const I& a, const I& b) const { return comparer(values[a], values[b]); }
	};

	template<typename T, typename I> using IndexedFunction = Indexed<Function<T>, T, I>;
}

//! Сортировка массива array вставками с предикатом сравнения comparer.
//! Характеристики алгоритма:
//! - Худшее время O(n^2) достигается, когда исходный массив отсортирован в обратном порядке;
//! - Среднее время О(n^2);
//! - Лучшее время O(n) достигается, когда исходный массив уже отсортирован;
//! - Самый эффективный алгоритм для сортировки до нескольких десятков элементов;
//! - Эффективен, если массив уже частично отсортирован;
//! - Устойчив.
template<typename RandomAccessRange, typename C = Comparers::Function<typename RandomAccessRange::value_type>> Meta::EnableIf<
	Range::IsFiniteRandomAccessRange<RandomAccessRange>::_ &&
	Range::IsRangeElementAssignable<RandomAccessRange>::_
> InsertionSort(const RandomAccessRange& range, C comparer = Op::Less<typename RandomAccessRange::value_type>)
{
	const size_t count = range.Count();
	for(size_t x=1; x<count; x++)
	{
		for(size_t y=x; y!=0 && comparer(range[y], range[y-1]); y--)
			core::swap(range[y], range[y-1]);
	}
}

//! Сортировка Шелла массива array с предикатом сравнения comparer.
template<typename RandomAccessRange, typename C = Comparers::Function<typename RandomAccessRange::value_type>> Meta::EnableIf<
	Range::IsFiniteRandomAccessRange<RandomAccessRange>::_ &&
	Range::IsRangeElementAssignable<RandomAccessRange>::_
> ShellSort(const RandomAccessRange& range, C comparer = Op::Less<typename RandomAccessRange::value_type>)
{
	const size_t count = range.Count();
	for(size_t d=count/2; d!=0; d/=2)
		for(size_t i=d; i<count; i++)
			for(size_t j=i; j>=d && comparer(range[j], range[j-d]); j-=d)
				core::swap(range[j], range[j-d]);
}

//! Сортировка выбором диапазона range с предикатом сравнения comparer.
//! Характеристики алгоритма:
//! - Худшее, среднее и лучшее время - O(n^2);
//! - Неустойчив.
template<typename RandomAccessRange, typename C = Comparers::Function<typename RandomAccessRange::value_type>> Meta::EnableIf<
	Range::IsFiniteRandomAccessRange<RandomAccessRange>::_ &&
	Range::IsRangeElementAssignable<RandomAccessRange>::_
> SelectionSort(const RandomAccessRange& range, C comparer = Op::Less<typename RandomAccessRange::value_type>)
{
	const size_t count = range.Count();
	for(size_t i=0; i<count; i++)
	{
		size_t minPos = i;
		for(size_t j=i+1; j<count; j++)
		{
			if(!comparer(range[j], range[minPos])) continue;
			minPos = j;
		}
		core::swap(range[i], range[minPos]);
	}
}

template<typename ArrRange, typename C = Comparers::Function<typename ArrRange::value_type>> Meta::EnableIf<
	Range::IsArrayRange<ArrRange>::_ &&
	Range::IsRangeElementAssignable<ArrRange>::_
> QuickSort(const ArrRange& range, C comparer = Op::Less<typename ArrRange::value_type>);



template<typename T> forceinline Meta::EnableIf<
	Meta::IsSignedIntegralType<T>::_,
T> ExtractKey(T t) {return t ^ T(1ull << (sizeof(T)*8-1));}

template<typename T> forceinline Meta::EnableIf<
	Meta::IsUnsignedIntegralType<T>::_,
T> ExtractKey(T t) {return t;}

template<typename T> forceinline size_t ExtractKey(T* t) {return (size_t)t;}

template<typename T, typename ExtractKeyFunc = T(*)(T), size_t RadixLog=8>
void RadixSort(ArrayRange<T> arr, ExtractKeyFunc extractKey = &ExtractKey<T>);

template<typename T, typename ExtractKeyFunc = size_t(*)(T*), size_t RadixLog=8>
void RadixSort(ArrayRange<T*> arr, ExtractKeyFunc extractKey = &ExtractKey<T>)
{
	RadixSort(arr.Reinterpret<size_t>(), extractKey);
}

//! Пирамидальная сортировка массива array с предикатом сравнения comparer.
//! Характеристики алгоритма:
//! - Гарантированная сложность: O(n Log n);
//! - Неустойчив;
//! - На почти отсортированных массивах работает так же долго, как и для хаотичных данных;
//! - Для N меньше нескольких тысяч ShellSort быстрее.
template<typename ArrRange, typename C = Comparers::Function<typename ArrRange::value_type>> Meta::EnableIf<
	Range::IsArrayRange<ArrRange>::_ &&
	Range::IsRangeElementAssignable<ArrRange>::_
> HeapSort(const ArrRange& range, C comparer = Op::Less<typename ArrRange::value_type>);

//! Сортировка слиянием
//! Особенности алгоритма:
//! - Лучшее, среднее и худшее время: O(n Log n);
//! - На «почти отсортированных» массивах работает столь же долго, как на хаотичных;
//! - Требует дополнительной памяти по размеру исходного массива.
template<typename ArrRange, typename C = Comparers::Function<typename ArrRange::value_type>> Meta::EnableIf<
	Range::IsArrayRange<ArrRange>::_ &&
	Range::IsRangeElementAssignable<ArrRange>::_
> MergeSort(const ArrRange& arr, C comparer = Op::Less<typename ArrRange::value_type>);

}}


#include "Sort.inl"
