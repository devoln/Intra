#pragma once

#include "Intra/Range/Concepts.h"
#include "Intra/Functional.h"

INTRA_BEGIN
namespace z_D {

template<typename T, typename C> constexpr void heap_shift_down(T arr[], size_t i, size_t j, C comparer)
{
	while(i*2+1 < j)
	{
		size_t maxNodeId = i*2+2;
		if(i*2+1 == j-1 || comparer(arr[i*2+2], arr[i*2+1])) maxNodeId--;

		if(!comparer(arr[i], arr[maxNodeId])) break;

		Swap(arr[i], arr[maxNodeId]);
		i = maxNodeId;
	}
}

}

/** Heap sort ``range`` using ``comparer`` predicate.
  1) Guaranteed complexity: O(n Log n);
  2) Unstable;
  3) Almost sorted ranges are sorted as slow as randomly ordered ranges;
  4) For Count(range) < few thousands ShellSort is faster.
*/
template<typename R, typename C = TFLess> constexpr Requires<
	CAssignableArrayClass<R>
> HeapSort(R&& range, C comparer = FLess)
{
	// TODO: support any random access range
	const auto count = size_t(LengthOf(range));

	// Build search tree
	for(size_t i = count/2; i > 0; i--)
		z_D::heap_shift_down(DataOf(range), i-1, count, comparer);

	// Take maximum (0) and move it to i'th position
	// Move element 0 to the right position in the tree
	for(size_t i = count-1; i>0; i--)
	{
		Swap(range[0], range[i]);
		z_D::heap_shift_down(DataOf(range), 0, i, comparer);
	}
}
INTRA_END
