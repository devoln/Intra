#pragma once

#include "Concepts/Container.h"
#include "Concepts/Range.h"
#include "Concepts/RangeOf.h"

#include "Funal/Op.h"

//! Данный заголовочный файл содержит алгоритмы для работы с бинарной кучей.
/*!
Бинарная куча позволяет за логарифмическое время O(log N) добавлять или извлекать элементы с максимальным приоритетом.
Куча представляет собой бинарное дерево, которые может быть построено на любом контейнере или диапазоне произвольного доступа.
Основным свойством кучи является то, что приоритет родительского элемента не может быть меньше дочернего,
а дочерние элементы расположены так, что приоритет правого не меньше левого.
Предикат, передаваемый во все алгоритмы работы с кучей, возвращает, является ли приоритет первого аргумента меньшим, чем приоритет второго.
То есть предикат Less, используемый по умолчанию, приведёт к куче, первым элементом которой является максимальный.
*/

namespace Intra { namespace Range {

//! Добавить новый элемент в кучу, построенную на контейнере произвольного доступа.
template<typename C, typename T, typename P = const Funal::TLess&,
	typename VT = Concepts::ValueTypeOfAs<C>> Meta::EnableIf<
	Concepts::IsSequentialContainer<C>::_ &&
	Concepts::HasIndex<C>::_ &&
	!Meta::IsConst<C>::_ &&
	Meta::IsCallable<P, VT, VT>::_
> HeapContainerPush(C& container, T&& value, P&& comparer = Funal::Less)
{
	container.push_back(Cpp::Forward<T>(value));
	HeapPush(Concepts::RangeOf(container), Cpp::Forward<P>(comparer));
}

//! Добавить элемент в кучу, построенную на диапазоне произвольного доступа.
//! Алгоритм предполагает, что куча построена на первых range.Length() - 1 элементах диапазона.
//! Тогда результатом вызова алгоритма будет куча на всём диапазоне, содержащая элемент range.Last().
template<typename R, typename P = const Funal::TLess&,
	typename AsR = Concepts::RangeOfType<R>, typename VT = Concepts::ValueTypeOf<AsR>> Meta::EnableIf<
	Concepts::IsFiniteRandomAccessRange<AsR>::_ &&
	Concepts::IsAssignableRange<AsR>::_ &&
	Meta::IsCallable<P, VT, VT>::_
> HeapPush(R&& range, P&& comparer = Funal::Less)
{
	auto r = Range::Forward<R>(range);
	size_t i = r.Length() - 1;
	size_t parent = (i - 1) / 2;
	while(i > 0 && comparer(r[parent], r[i]))
	{
		Cpp::Swap(r[i], r[parent]);
		i = parent;
		parent = (i - 1) / 2;
	}
}

//! Упорядочить поддерево элемента с индексом index.
template<typename R, typename P = const Funal::TLess&,
	typename AsR = Concepts::RangeOfType<R>, typename VT = Concepts::ValueTypeOf<AsR>> Meta::EnableIf<
	Concepts::IsFiniteRandomAccessRange<AsR>::_ &&
	Concepts::IsAssignableRange<AsR>::_ &&
	Meta::IsCallable<P, VT, VT>::_
> HeapOrder(R&& range, size_t index, P&& comparer = Funal::Less)
{
	auto r = Range::Forward<R>(range);
	for(;;)
	{
		const size_t leftIndex = 2*index + 1;
		const size_t rightIndex = 2*index + 2;
		size_t largestIndex = index;

		if(leftIndex < r.Length() &&
			comparer(r[largestIndex], r[leftIndex]))
				largestIndex = leftIndex;

		if(rightIndex < r.Length() &&
			comparer(r[largestIndex], r[rightIndex]))
				largestIndex = rightIndex;

		if(largestIndex == index) break;

		Cpp::Swap(r[index], r[largestIndex]);
		index = largestIndex;
	}
}

//! Построить бинарную кучу на указанном диапазоне произвольного доступа.
template<typename R, typename P = const Funal::TLess&,
	typename AsR = Concepts::RangeOfType<R>,
	typename VT = Concepts::ValueTypeOf<AsR>
> Meta::EnableIf<
	Concepts::IsFiniteRandomAccessRange<AsR>::_ &&
	Concepts::IsAssignableRange<AsR>::_ &&
	Meta::IsCallable<P, VT, VT>::_
> HeapBuild(R&& range, P&& comparer = Funal::Less)
{
	auto r = Range::Forward<R>(range);
	size_t i = r.Length() / 2;
	while(i --> 0) HeapOrder(Cpp::Move(r), i, Cpp::Forward<P>(comparer));
}

//! Извлечь элемент с макимальным приоритетом из кучи.
//! В результате выполнения алгоритма получится куча, построенная на первых range.Length() - 1 элементах.
//! Последний элемент не будет являться частью кучи.
template<typename R, typename P = const Funal::TLess&,
	typename AsR = Concepts::RangeOfType<R>,
	typename T = Concepts::ValueTypeOf<AsR>
> Meta::EnableIf<
	Concepts::IsFiniteRandomAccessRange<AsR>::_ &&
	Concepts::IsAssignableRange<AsR>::_ &&
	Meta::IsCallable<P, T, T>::_,
T&> HeapPop(R&& range, P&& comparer = Funal::Less)
{
	Cpp::Swap(range.First(), range.Last());
	HeapOrder(Range::DropLast(range), 0, Cpp::Forward<P>(comparer));
	return range.Last();
}

//! Извлечь элемент с макимальным приоритетом из кучи.
//! В результате выполнения алгоритма максимальный элемент будет удалён из контейнера,
//! и куча будет упорядочена для оставшихся элементов.
template<typename C, typename P = const Funal::TLess&,
	typename AsR = Concepts::RangeOfType<C>,
	typename T = Concepts::ValueTypeOf<AsR>
> Meta::EnableIf<
	Concepts::IsSequentialContainer<C>::_ &&
	Concepts::HasIndex<C>::_ &&
	!Meta::IsConst<C>::_ &&
	Meta::IsCallable<P, T, T>::_,
T> HeapContainerPop(C& container, P&& comparer = Funal::Less)
{
	auto result = HeapPop(Concepts::RangeOf(container), Cpp::Forward<P>(comparer));
	container.pop_back();
	return result;
}

}}
