#pragma once

#include "Cpp/Warnings.h"

#include "Funal/Op.h"

#include "Concepts/Range.h"
#include "Concepts/RangeOf.h"

namespace Intra { namespace Range {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename R, typename P = Funal::TLess> Meta::EnableIf<
	Concepts::IsConsumableRange<R>::_,
bool> IsSorted(R&& range, P comparer = Funal::Less)
{
	if(range.Empty()) return true;
	R rangeCopy = Cpp::Forward<R>(range);
	Concepts::ValueTypeOf<R> prev;
	auto cur = rangeCopy.First();
	rangeCopy.PopFirst();
	while(!rangeCopy.Empty())
	{
		prev = Cpp::Move(cur);
		cur = rangeCopy.First();
		if(comparer(cur, prev)) return false;
		rangeCopy.PopFirst();
	}
	return true;
}

template<typename R, typename P = Funal::TLess,
	typename AsR = Concepts::RangeOfType<R>
> Meta::EnableIf<
	!Concepts::IsInputRange<R>::_ &&
	Concepts::IsNonInfiniteForwardRange<AsR>::_,
bool> IsSorted(R&& range, P comparer = Funal::Less)
{return IsSorted(Range::Forward<R>(range), comparer);}

INTRA_WARNING_POP

}}
