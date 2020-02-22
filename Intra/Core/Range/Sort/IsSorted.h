#pragma once

#include "Core/Functional.h"
#include "Core/Range/Concepts.h"

INTRA_BEGIN
template<typename R, typename P = TFLess> INTRA_NODISCARD constexpr Requires<
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
	typename AsR = TRangeOfType<R>
> INTRA_NODISCARD constexpr forceinline Requires<
	!CInputRange<R> &&
	CNonInfiniteForwardRange<AsR>,
bool> IsSorted(R&& range, P comparer = FLess)
{return IsSorted(ForwardAsRange<R>(range), comparer);}
INTRA_END
