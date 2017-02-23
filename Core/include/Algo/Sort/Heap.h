#pragma once

#include "Platform/CppFeatures.h"
#include "Platform/CppWarnings.h"
#include "Algo/Op.h"
#include "Range/Concepts.h"
#include "Range/AsRange.h"

namespace Intra { namespace Algo {

using namespace Range::Concepts;

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace D {

template<typename T, typename C> void heap_shift_down(T arr[], size_t i, size_t j, C comparer)
{
	while(i*2+1<j)
	{
		size_t maxNodeId = i*2+2;
		if(i*2+1==j-1 || comparer(arr[i*2+2], arr[i*2+1])) maxNodeId--;

		if(!comparer(arr[i], arr[maxNodeId])) break;

		Meta::Swap(arr[i], arr[maxNodeId]);
		i = maxNodeId;
	}
}

}

//! Пирамидальная сортировка массива array с предикатом сравнения comparer.
//! Характеристики алгоритма:
//! - Гарантированная сложность: O(n Log n);
//! - Неустойчив;
//! - На почти отсортированных массивах работает так же долго, как и для хаотичных данных;
//! - Для N меньше нескольких тысяч ShellSort быстрее.
template<typename R, typename C=Comparers::Function<ValueTypeOf<R>>> Meta::EnableIf<
	Range::IsArrayRange<R>::_ &&
	Range::IsAssignableRange<R>::_
> HeapSort(const R& range, C comparer=Op::Less<ValueTypeOf<R>>)
{
	const size_t count = range.Length();

	//Строим дерево поиска
	for(size_t i=count/2; i>0; i--)
		D::heap_shift_down(range.Data(), i-1, count, comparer);

	//Забираем максимальный (0) элемент дерева в i-ю позицию
	//Перемещаем новый 0 элемент на правильную позицию в дереве
	for(size_t i=count-1; i>0; i--)
	{
		Meta::Swap(range[0], range[i]);
		D::heap_shift_down(range.Data(), 0, i, comparer);
	}
}

template<typename R, typename C=Comparers::Function<ValueTypeOfAs<R>>,
typename AsR=AsRangeResult<R>> forceinline Meta::EnableIf<
	!IsInputRange<R>::_ &&
	Range::IsArrayRange<AsR>::_ &&
	Range::IsAssignableRange<AsR>::_
> HeapSort(R&& range, C comparer=Op::Less<ValueTypeOf<AsR>>)
{HeapSort(Range::Forward<R>(range), comparer);}

INTRA_WARNING_POP

}}
