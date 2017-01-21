#pragma once

#include "Range/ArrayRange.h"
#include "Algo/Op.h"

namespace Intra { namespace Algo {

//! Сортировка выбором диапазона range с предикатом сравнения comparer.
//! Характеристики алгоритма:
//! - Худшее, среднее и лучшее время - O(n^2);
//! - Неустойчив.
template<typename RandomAccessRange, typename C = Comparers::Function<Range::ValueTypeOf<RandomAccessRange>>> Meta::EnableIf<
	Range::IsFiniteRandomAccessRange<RandomAccessRange>::_ &&
	Range::IsAssignableInputRange<RandomAccessRange>::_
> SelectionSort(const RandomAccessRange& range, C comparer = Op::Less<Range::ValueTypeOf<RandomAccessRange>>)
{
	const size_t count = Range::Count(range);
	for(size_t i=0; i<count; i++)
	{
		size_t minPos = i;
		for(size_t j=i+1; j<count; j++)
		{
			if(!comparer(range[j], range[minPos])) continue;
			minPos = j;
		}
		Meta::Swap(range[i], range[minPos]);
	}
}

}}

