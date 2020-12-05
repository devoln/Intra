#pragma once

#include "Intra/Functional.h"
#include "Intra/Range/Concepts.h"

INTRA_BEGIN
template<typename R, typename P = decltype(Less)> [[nodiscard]] constexpr Requires<
	CConsumableRange<R>,
bool> IsSorted(R&& range, P comparer = FLess)
{
	if(range.Empty()) return true;
	R rangeCopy = Forward<R>(range);
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

template<typename R, typename P = decltype(Less),
	typename AsR = TRangeOfRef<R>
> [[nodiscard]] constexpr Requires<
	!CRange<R> &&
	CNonInfiniteForwardRange<AsR>,
bool> IsSorted(R&& range, P comparer = FLess)
{return IsSorted(ForwardAsRange<R>(range), comparer);}
INTRA_END
