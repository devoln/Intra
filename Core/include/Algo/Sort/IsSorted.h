#pragma once

#include "Platform/CppWarnings.h"
#include "Algo/Op.h"
#include "Range/Concepts.h"
#include "Range/AsRange.h"

namespace Intra { namespace Algo {

using namespace Range::Concepts;

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename R, typename P=bool(*)(const ValueTypeOf<R>&, const ValueTypeOf<R>&)> Meta::EnableIf<
	Range::IsConsumableRange<R>::_,
bool> IsSorted(R&& range, P comparer=&Op::Less<ValueTypeOf<R>>)
{
	if(range.Empty()) return true;
	R rangeCopy = Meta::Forward<R>(range);
	ValueTypeOf<R> prev, cur = rangeCopy.First();
	rangeCopy.PopFirst();
	while(!rangeCopy.Empty())
	{
		prev = Meta::Move(cur);
		cur = rangeCopy.First();
		if(comparer(cur, prev)) return false;
		rangeCopy.PopFirst();
	}
	return true;
}

template<typename R, typename P=bool(*)(const ValueTypeOfAs<R>&, const ValueTypeOfAs<R>&)> Meta::EnableIf<
	!IsInputRange<R>::_ &&
	IsAsNonInfiniteForwardRange<R>::_,
bool> IsSorted(R&& range, P comparer=&Op::Less<ValueTypeOfAs<R>>)
{return IsSorted(Range::Forward<R>(range), comparer);}

INTRA_WARNING_POP

}}
