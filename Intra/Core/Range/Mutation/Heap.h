#pragma once

#include "Core/CContainer.h"
#include "Core/Range/Concepts.h"

#include "Core/Functional.h"

//! This header file contains algorithms working with binary heaps.
/*!
Binary heap makes possible to push and pop elements with maximal priorities in O(log N) time.
Binary heap is a binary tree that can be built on any container or random access range.
The main property of a heap is that a parent element's priority cannot be less than the priorities of its children
and right element's priority cannot be less than of the left one.
The parameter comparer is a predicate that must return true if the first argument's priority is less than the second one's.
The default comparer Less defines the heap that retrieves elements with the maximum priority.
You must consistently use the same predicate with the heap after it is built.
If you want to change the predicate, first call HeapBuild to reorder its elements.
*/

INTRA_BEGIN
//! Add a new value into the heap built on a random access container.
template<typename C, typename T, typename P = const TFLess&,
	typename VT = TValueTypeOfAs<C>> constexpr Requires<
	CSequentialContainer<C> &&
	CHasIndex<C> &&
	!CConst<C> &&
	CCallable<P, VT, VT>
> HeapContainerPush(C& container, T&& value, P&& comparer = FLess)
{
	container.push_back(Forward<T>(value));
	HeapPush(RangeOf(container), Forward<P>(comparer));
}

//! Add a new value into the heap built on a random access range.
//! The algorithm assumes the heap to be built on the first range.Length() - 1 range elements.
//! This range reorders elements so that the result will be a valid heap built on all elements of the range, containing the value of range.Last().
template<typename R, typename P = const TFLess&,
	typename AsR = TRangeOfType<R>, typename VT = TValueTypeOf<AsR>> constexpr Requires<
	CFiniteRandomAccessRange<AsR> &&
	CAssignableRange<AsR> &&
	CCallable<P, VT, VT>
> HeapPush(R&& range, P&& comparer = FLess)
{
	auto r = ForwardAsRange<R>(range);
	size_t i = r.Length() - 1;
	size_t parent = (i - 1) / 2;
	while(i > 0 && comparer(r[parent], r[i]))
	{
		Swap(r[i], r[parent]);
		i = parent;
		parent = (i - 1) / 2;
	}
}

//! Order the subtree of the element in position index.
template<typename R, typename P = const TFLess&,
	typename AsR = TRangeOfType<R>, typename VT = TValueTypeOf<AsR>> constexpr Requires<
	CFiniteRandomAccessRange<AsR> &&
	CAssignableRange<AsR> &&
	CCallable<P, VT, VT>
> HeapOrder(R&& range, size_t index, P&& comparer = FLess)
{
	auto r = ForwardAsRange<R>(range);
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

		Swap(r[index], r[largestIndex]);
		index = largestIndex;
	}
}

//! Build a binary heap on the provided random access range by reordering its elements.
template<typename R, typename P = const TFLess&,
	typename AsR = TRangeOfType<R>,
	typename VT = TValueTypeOf<AsR>
> constexpr Requires<
	CFiniteRandomAccessRange<AsR> &&
	CAssignableRange<AsR> &&
	CCallable<P, VT, VT>
> HeapBuild(R&& range, P&& comparer = FLess)
{
	auto r = ForwardAsRange<R>(range);
	size_t i = r.Length() / 2;
	while(i --> 0) HeapOrder(Move(r), i, Forward<P>(comparer));
}

//! Extract the first element from the heap.
//! The result of calling this algorithm is a heap, built on the first range.Length() - 1 elements.
//! The range.Last() will be equal to the removed element, it will not be a part of the heap.
//! @returns a reference to range.Last() - removed element.
template<typename R, typename P = const TFLess&,
	typename AsR = TRangeOfType<R>,
	typename T = TValueTypeOf<AsR>
> constexpr Requires<
	CFiniteRandomAccessRange<AsR> &&
	CAssignableRange<AsR> &&
	CCallable<P, T, T>,
T&> HeapPop(R&& range, P&& comparer = FLess)
{
	Swap(range.First(), range.Last());
	HeapOrder(DropLast(range), 0, Forward<P>(comparer));
	return range.Last();
}

//! Extract a maximum priority element from the heap.
//! The result of calling this algorithm is a heap, built on the first container.Length() - 1 elements,
//! and the resulting heap will be properly ordered. The removed element will be removed from it.
template<typename C, typename P = const TFLess&,
	typename AsR = TRangeOfType<C>,
	typename T = TValueTypeOf<AsR>
> constexpr Requires<
	CSequentialContainer<C> &&
	CHasIndex<C> &&
	!CConst<C> &&
	CCallable<P, T, T>,
T> HeapContainerPop(C& container, P&& comparer = FLess)
{
	auto result = HeapPop(RangeOf(container), Forward<P>(comparer));
	container.pop_back();
	return result;
}
INTRA_END
