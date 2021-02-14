﻿#pragma once

#include "Intra/Functional.h"
#include "Intra/Range/Span.h"
#include "Intra/Range/Operations.h"

namespace Intra { INTRA_BEGIN
/** Sort ``range`` using selection sort algorithm using ``comparer`` predicate.
  1) The worst, average and the best times are O(n^2);
  2) Unstable.
*/
template<typename R, typename C = decltype(Less)> constexpr Requires<
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

template<typename R, typename C = decltype(Less),
	typename AsR = TRangeOfRef<R>
> constexpr Requires<
	!CRange<R> &&
	CRandomAccessRangeWithLength<AsR> &&
	CAssignableRange<AsR>
> SelectionSort(R&& range, C comparer = FLess)
{SelectionSort(ForwardAsRange<R>(range), comparer);}
} INTRA_END
