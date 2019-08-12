#pragma once

#include "Funal/Op.h"

#include "Core/Range/Concepts.h"


INTRA_BEGIN
inline namespace Range {

template<typename R, typename P = TLess> INTRA_NODISCARD INTRA_CONSTEXPR2 Requires<
	CConsumableRange<R>,
bool> IsSorted(R&& range, P comparer = Less)
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

template<typename R, typename P = TLess,
	typename AsR = TRangeOfType<R>
> INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline Requires<
	!CInputRange<R> &&
	CNonInfiniteForwardRange<AsR>,
bool> IsSorted(R&& range, P comparer = Less)
{return IsSorted(ForwardAsRange<R>(range), comparer);}

}
INTRA_END
