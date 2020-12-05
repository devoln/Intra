#pragma once

#include "Intra/TypeSafe.h"
#include "Intra/Concepts.h"
#include "Intra/Range/Concepts.h"
#include "Intra/Container/Concepts.h"
#include "Intra/Functional.h"

/** This header file contains algorithms working with binary heaps.

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
/// Add a new value into the heap built on a random access container.
template<CSequentialContainer C, typename T, CCallable<TListValue<C>, TListValue<C>> P = const decltype(Less)&>
requires CHasIndex<C> && (!CConst<C>)
constexpr void HeapContainerPush(C& container, T&& value, P&& comparer = Less)
{
	container.push_back(Forward<T>(value));
	HeapPush(RangeOf(container), Forward<P>(comparer));
}

/// Add a new value into the heap built on a random access range.
/// The algorithm assumes the heap to be built on the first range.Length() - 1 range elements.
/// This range reorders elements so that the result will be a valid heap built on all elements of the range, containing the value of range.Last().
template<CFiniteRandomAccessList R, CCallable<TListValue<R>, TListValue<R>> P = const decltype(Less)&>
requires CAssignableList<R>
constexpr void HeapPush(R&& range, P&& comparer = Less)
{
	auto r = ForwardAsRange<R>(range);
	INTRA_PRECONDITION(r.Length() >= 1);
	auto i = r.Length() - 1;
	auto parent = (i - 1) >> 1;
	while(i > 0 && comparer(r[parent], r[i]))
	{
		Swap(r[i], r[parent]);
		i = parent;
		parent = (i - 1) >> 1;
	}
}

/// Order the subtree of the element in position index.
template<CFiniteRandomAccessList R, CCallable<TListValue<R>, TListValue<R>> P = const decltype(Less)&>
requires CAssignableList<R>
constexpr void HeapOrder(R&& range, Index index, P&& comparer = FLess)
{
	auto r = ForwardAsRange<R>(range);
	auto ind = size_t(index);
	for(;;)
	{
		const auto leftIndex = 2*ind + 1;
		const auto rightIndex = 2*ind + 2;
		auto largestIndex = ind;

		if(leftIndex < size_t(r.Length()) &&
			comparer(r[largestIndex], r[leftIndex]))
				largestIndex = leftIndex;

		if(rightIndex < size_t(r.Length()) &&
			comparer(r[largestIndex], r[rightIndex]))
				largestIndex = rightIndex;

		if(largestIndex == ind) break;

		Swap(r[ind], r[largestIndex]);
		ind = largestIndex;
	}
}

/// Build a binary heap on the provided random access range by reordering its elements.
template<CFiniteRandomAccessList R, CCallable<TListValue<R>, TListValue<R>> P = const decltype(Less)&>
constexpr void HeapBuild(R&& range, P&& comparer = Less) requires CAssignableList<R>
{
	auto r = ForwardAsRange<R>(range);
	auto i = size_t(r.Length()) / 2;
	while(i--) HeapOrder(r, i, INTRA_FWD(comparer));
}

/// Extract the first element from the heap.
/// The result of calling this algorithm is a heap, built on the first range.Length() - 1 elements.
/// The range.Last() will be equal to the removed element, it will not be a part of the heap.
/// @returns a reference to range.Last() - removed element.
template<CFiniteRandomAccessList R, CCallable<TListValue<R>, TListValue<R>> P = const decltype(Less)&>
constexpr decltype(auto) HeapPop(R&& range, P&& comparer = Less) requires CAssignableList<R>
{
	Swap(range.First(), range.Last());
	HeapOrder(DropLast(range), 0, Forward<P>(comparer));
	return range.Last();
}

/// Extract a maximum priority element from the heap.
/// The result of calling this algorithm is a heap, built on the first container.Length() - 1 elements,
/// and the resulting heap will be properly ordered. The removed element will be removed from it.
template<CSequentialContainer C, CCallable<TListValue<C>, TListValue<C>> P = const decltype(Less)&>
constexpr auto HeapContainerPop(C& container, P&& comparer = Less) requires CHasIndex<C> && !CConst<C>
{
	auto result = HeapPop(RangeOf(container), INTRA_FWD(comparer));
	container.pop_back();
	return result;
}
INTRA_END
