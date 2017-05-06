#pragma once

#include "Utils/Span.h"
#include "Cpp/Warnings.h"
#include "Concepts/Range.h"
#include "Concepts/RangeOf.h"
#include "Range/Operations.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Range {

//! Сортировка массива array вставками с предикатом сравнения comparer.
//! Характеристики алгоритма:
//! - Худшее время O(n^2) достигается, когда исходный массив отсортирован в обратном порядке;
//! - Среднее время О(n^2);
//! - Лучшее время O(n) достигается, когда исходный массив уже отсортирован;
//! - Самый эффективный алгоритм для сортировки до нескольких десятков элементов;
//! - Эффективен, если массив уже частично отсортирован;
//! - Устойчив.
template<typename R, typename C=Comparers::Function<Concepts::ValueTypeOf<R>>> Meta::EnableIf<
	Concepts::IsRandomAccessRangeWithLength<R>::_ &&
	Concepts::IsAssignableRange<R>::_
> InsertionSort(const R& range, C comparer = Op::Less<Concepts::ValueTypeOf<R>>)
{
	const size_t count = Count(range);
	for(size_t x=1; x<count; x++)
	{
		for(size_t y=x; y!=0 && comparer(range[y], range[y-1]); y--)
			Cpp::Swap(range[y], range[y-1]);
	}
}

template<typename R, typename C=Comparers::Function<Concepts::ValueTypeOfAs<R>>,
	typename AsR = Concepts::RangeOfType<R>
> forceinline Meta::EnableIf<
	!Concepts::IsInputRange<R>::_ &&
	Concepts::IsRandomAccessRangeWithLength<AsR>::_ &&
	Concepts::IsAssignableRange<AsR>::_
> InsertionSort(R&& range, C comparer=Op::Less<Concepts::ValueTypeOf<R>>)
{InsertionSort(Range::Forward<R>(range), comparer);}

//! Сортировка Шелла массива array с предикатом сравнения comparer.
template<typename R, typename C=Comparers::Function<Concepts::ValueTypeOf<R>>>
Meta::EnableIf<
	Concepts::IsRandomAccessRangeWithLength<R>::_ &&
	Concepts::IsAssignableRange<R>::_
> ShellSort(const R& range, C comparer=Op::Less<Concepts::ValueTypeOf<R>>)
{
	const size_t count = Count(range);
	for(size_t d=count/2; d!=0; d/=2)
		for(size_t i=d; i<count; i++)
			for(size_t j=i; j>=d && comparer(range[j], range[j-d]); j-=d)
				Cpp::Swap(range[j], range[j-d]);
}

template<typename R, typename C=Comparers::Function<Concepts::ValueTypeOfAs<R>>,
	typename AsR=Concepts::RangeOfType<R>
> forceinline Meta::EnableIf<
	!Concepts::IsInputRange<R>::_ &&
	Concepts::IsRandomAccessRangeWithLength<AsR>::_ &&
	Concepts::IsAssignableRange<AsR>::_
> ShellSort(R&& range, C comparer=Op::Less<Concepts::ValueTypeOf<R>>)
{ShellSort(Range::Forward<R>(range), comparer);}

}}

INTRA_WARNING_POP
