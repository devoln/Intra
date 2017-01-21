#pragma once

#include "Range/Concepts.h"
#include "Algo/Op.h"
#include "Platform/CppWarnings.h"

namespace Intra { namespace Algo {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename R, typename P=bool(*)(const Range::ValueTypeOf<R>&, const Range::ValueTypeOf<R>&)> Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_,
bool> IsSorted(const R& range, P comparer=&Op::Less<Range::ValueTypeOf<R>>)
{
	if(range.Empty()) return true;
	R rangeCopy = range;
	Range::ValueTypeOf<R> prev, cur = rangeCopy.First();
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

INTRA_WARNING_POP

}}
