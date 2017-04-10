#pragma once

#include "Platform/CppFeatures.h"
#include "Platform/CppWarnings.h"
#include "Range/Generators/Span.h"
#include "Algo/Op.h"
#include "Range/Operations.h"

namespace Intra { namespace Algo {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

//! Сортировка выбором диапазона range с предикатом сравнения comparer.
//! Характеристики алгоритма:
//! - Худшее, среднее и лучшее время - O(n^2);
//! - Неустойчив.
template<typename R, typename C=Comparers::Function<ValueTypeOf<R>>> Meta::EnableIf<
	IsRandomAccessRangeWithLength<R>::_ &&
	IsAssignableRange<R>::_
> SelectionSort(const R& range, C comparer = Op::Less<ValueTypeOf<R>>)
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

template<typename R, typename C=Comparers::Function<ValueTypeOfAs<R>>,
typename AsR=AsRangeResult<R>> forceinline Meta::EnableIf<
	!IsInputRange<R>::_ &&
	IsRandomAccessRangeWithLength<AsR>::_ &&
	IsAssignableRange<AsR>::_
> SelectionSort(R&& range, C comparer=Op::Less<ValueTypeOf<R>>)
{SelectionSort(Range::Forward<R>(range), comparer);}

INTRA_WARNING_POP

}}
