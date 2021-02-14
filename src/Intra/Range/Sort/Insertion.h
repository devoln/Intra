#pragma once

#include "Intra/Range/Span.h"
#include "Intra/Functional.h"
#include "Intra/Range/Concepts.h"
#include "Intra/Range/Operations.h"

namespace Intra { INTRA_BEGIN
/** Sort ``range`` using insertion sort algorithm using ``comparer`` predicate.
  1) The worst time O(n^2) is reached when source range has reverse order;
  2) Average time О(n^2);
  3) The best time O(n) is reached when source ``range`` is already sorted;
  4) The most efficient sorting algorithm to sort a few dozens of elements;
  5) Efficient if source ``range`` is already almost sorted;
  6) Stable - keeps source element order for equal elements.
*/
template<typename R, typename C = decltype(Less)> constexpr Requires<
	CRandomAccessRangeWithLength<R> &&
	CAssignableRange<R>
> InsertionSort(const R& range, C comparer = FLess)
{
	const auto count = Count(range);
	for(index_t x = 1; x < count; x++)
	{
		for(index_t y = x; y != 0 && comparer(range[y], range[y-1]); y--)
			Swap(range[y], range[y-1]);
	}
}

template<typename R, typename C = decltype(Less),
	typename AsR = TRangeOfRef<R>
> constexpr Requires<
	!CRange<R> &&
	CRandomAccessRangeWithLength<AsR> &&
	CAssignableRange<AsR>
> InsertionSort(R&& range, C comparer = FLess)
{InsertionSort(ForwardAsRange<R>(range), comparer);}

/// Sort ``range`` using Shell sort algorithm and using comparison predicate ``comparer``.
template<typename R, typename C = decltype(Less)>
constexpr Requires<
	CRandomAccessRangeWithLength<R> &&
	CAssignableRange<R>
> ShellSort(const R& range, C comparer = FLess)
{
	const auto count = size_t(Count(range));
	for(size_t d = count/2; d != 0; d /= 2)
		for(size_t i = d; i < count; i++)
			for(size_t j = i; j >= d && comparer(range[j], range[j-d]); j -= d)
				Swap(range[j], range[j-d]);
}

template<typename R, typename C = decltype(Less),
	typename AsR=TRangeOfRef<R>
> constexpr Requires<
	!CRange<R> &&
	CRandomAccessRangeWithLength<AsR> &&
	CAssignableRange<AsR>
> ShellSort(R&& range, C comparer = FLess)
{ShellSort(ForwardAsRange<R>(range), comparer);}
} INTRA_END
