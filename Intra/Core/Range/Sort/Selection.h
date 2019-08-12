#pragma once

#include "Core/Functional.h"
#include "Core/Range/Span.h"
#include "Core/Range/Operations.h"

INTRA_CORE_RANGE_BEGIN
/** Sort ``range`` using selection sort algorithm using ``comparer`` predicate.
  1) The worst, average and the best times are O(n^2);
  2) Unstable.
*/
template<typename R, typename C = TFLess> INTRA_CONSTEXPR2 Requires<
	CRandomAccessRangeWithLength<R> &&
	CAssignableRange<R>
> SelectionSort(const R& range, C comparer = FLess)
{
	const size_t count = Count(range);
	for(size_t i=0; i<count; i++)
	{
		size_t minPos = i;
		for(size_t j = i+1; j < count; j++)
		{
			if(!comparer(range[j], range[minPos])) continue;
			minPos = j;
		}
		Swap(range[i], range[minPos]);
	}
}

template<typename R, typename C = TFLess,
	typename AsR = TRangeOfType<R>
> INTRA_CONSTEXPR2 forceinline Requires<
	!CInputRange<R> &&
	CRandomAccessRangeWithLength<AsR> &&
	CAssignableRange<AsR>
> SelectionSort(R&& range, C comparer = FLess)
{SelectionSort(ForwardAsRange<R>(range), comparer);}
INTRA_CORE_RANGE_END
