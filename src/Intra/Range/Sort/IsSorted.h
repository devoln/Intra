#pragma once

#include "Intra/Functional.h"
#include "Intra/Range/Concepts.h"

INTRA_BEGIN
template<typename R, typename P = TFLess> [[nodiscard]] constexpr Requires<
	CConsumableRange<R>,
bool> IsSorted(R&& range, P comparer = FLess)
{
	if(range.Empty()) return true;
	R rangeCopy = Forward<R>(range);
	TValueTypeOf<R> prev;
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

template<typename R, typename P = TFLess,
	typename AsR = TRangeOfRef<R>
> [[nodiscard]] constexpr Requires<
	!CInputRange<R> &&
	CNonInfiniteForwardRange<AsR>,
bool> IsSorted(R&& range, P comparer = FLess)
{return IsSorted(ForwardAsRange<R>(range), comparer);}
INTRA_END
