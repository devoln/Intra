#pragma once

#include <Intra/Core.h>
#include <Intra/Concepts.h>

namespace Intra { INTRA_BEGIN
template<CConsumableRange R, class P = decltype(Less)> [[nodiscard]] constexpr bool IsSorted(R&& range, P comparer = Less)
{
	if(range.Empty()) return true;
	R rangeCopy = INTRA_FWD(range);
	TRangeValue<R> prev;
	auto cur = rangeCopy.First();
	rangeCopy.PopFirst();
	while(!rangeCopy.Empty())
	{
		prev = Move(cur);
		cur = rangeCopy.First();
		if(comparer(cur, prev)) return false;
		rangeCopy.PopFirst();
	}
	return true;
}
} INTRA_END
