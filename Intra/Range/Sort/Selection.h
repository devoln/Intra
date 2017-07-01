#pragma once

#include "Cpp/Features.h"
#include "Cpp/Warnings.h"

#include "Utils/Span.h"

#include "Funal/Op.h"

#include "Range/Operations.h"

namespace Intra { namespace Range {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

//! Сортировка выбором диапазона range с предикатом сравнения comparer.
//! Характеристики алгоритма:
//! - Худшее, среднее и лучшее время - O(n^2);
//! - Неустойчив.
template<typename R, typename C = Funal::TLess> Meta::EnableIf<
	Concepts::IsRandomAccessRangeWithLength<R>::_ &&
	Concepts::IsAssignableRange<R>::_
> SelectionSort(const R& range, C comparer = Funal::Less)
{
	const size_t count = Count(range);
	for(size_t i=0; i<count; i++)
	{
		size_t minPos = i;
		for(size_t j=i+1; j<count; j++)
		{
			if(!comparer(range[j], range[minPos])) continue;
			minPos = j;
		}
		Cpp::Swap(range[i], range[minPos]);
	}
}

template<typename R, typename C = Funal::TLess,
	typename AsR = Concepts::RangeOfType<R>
> forceinline Meta::EnableIf<
	!Concepts::IsInputRange<R>::_ &&
	Concepts::IsRandomAccessRangeWithLength<AsR>::_ &&
	Concepts::IsAssignableRange<AsR>::_
> SelectionSort(R&& range, C comparer = Funal::Less)
{SelectionSort(Range::Forward<R>(range), comparer);}

INTRA_WARNING_POP

}}
