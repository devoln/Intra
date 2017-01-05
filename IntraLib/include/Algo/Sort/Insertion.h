#pragma once

#include "Range/ArrayRange.h"
#include "Platform/CppWarnings.h"

namespace Intra { namespace Algo {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

//! Сортировка массива array вставками с предикатом сравнения comparer.
//! Характеристики алгоритма:
//! - Худшее время O(n^2) достигается, когда исходный массив отсортирован в обратном порядке;
//! - Среднее время О(n^2);
//! - Лучшее время O(n) достигается, когда исходный массив уже отсортирован;
//! - Самый эффективный алгоритм для сортировки до нескольких десятков элементов;
//! - Эффективен, если массив уже частично отсортирован;
//! - Устойчив.
template<typename RandomAccessRange, typename C = Comparers::Function<Range::ValueTypeOf<RandomAccessRange>>> Meta::EnableIf<
	Range::IsFiniteRandomAccessRange<RandomAccessRange>::_ &&
	Range::IsAssignableInputRange<RandomAccessRange>::_
> InsertionSort(const RandomAccessRange& range, C comparer = Op::Less<Range::ValueTypeOf<RandomAccessRange>>)
{
	const size_t count = Range::Count(range);
	for(size_t x=1; x<count; x++)
	{
		for(size_t y=x; y!=0 && comparer(range[y], range[y-1]); y--)
			Meta::Swap(range[y], range[y-1]);
	}
}

//! Сортировка Шелла массива array с предикатом сравнения comparer.
template<typename RandomAccessRange, typename C = Comparers::Function<Range::ValueTypeOf<RandomAccessRange>>> Meta::EnableIf<
	Range::IsFiniteRandomAccessRange<RandomAccessRange>::_ &&
	Range::IsAssignableInputRange<RandomAccessRange>::_
> ShellSort(const RandomAccessRange& range, C comparer = Op::Less<Range::ValueTypeOf<RandomAccessRange>>)
{
	const size_t count = Range::Count(range);
	for(size_t d=count/2; d!=0; d/=2)
		for(size_t i=d; i<count; i++)
			for(size_t j=i; j>=d && comparer(range[j], range[j-d]); j-=d)
				Meta::Swap(range[j], range[j-d]);
}

INTRA_WARNING_POP

}}
