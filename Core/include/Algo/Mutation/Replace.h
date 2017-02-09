#pragma once

#include "Range/Concepts.h"
#include "Algo/Search/Single.h"
#include "Platform/CppWarnings.h"

namespace Intra { namespace Algo {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename R> Meta::EnableIf<
	Range::IsAssignableRange<R>::_ && !Meta::IsConst<R>::_
> ReplaceOneAdvance(R&& range, const Range::ValueTypeOf<R>& from, const Range::ValueTypeOf<R>& to)
{
	while(!range.Empty())
	{
		auto& v = range.First();
		if(v==from)
		{
			v=to;
			range.PopFirst();
			return;
		}
		range.PopFirst();
	}
}

template<typename R> Meta::EnableIf<
	Range::IsAsAssignableRange<R>::_
> ReplaceOne(R&& range, const Range::ValueTypeOf<R>& from, const Range::ValueTypeOf<R>& to)
{
	auto rangeCopy = Range::Forward<R>(range);
	ReplaceOneAdvance(rangeCopy, from, to);
}

template<typename R> Meta::EnableIf<
	Range::IsAssignableRange<R>::_ && !Meta::IsConst<R>::_
> ReplaceAdvance(R&& range, const Range::ValueTypeOf<R>& from, const Range::ValueTypeOf<R>& to)
{
	while(!range.Empty())
	{
		auto& v = range.First();
		if(v==from) v = to;
		range.PopFirst();
	}
}

template<typename R> forceinline Meta::EnableIf<
	Range::IsAsAssignableRange<R>::_ && !Meta::IsConst<R>::_
> Replace(R&& range, const Range::ValueTypeOf<R>& from, const Range::ValueTypeOf<R>& to)
{
	auto rangeCopy = Range::Forward<R>(range);
	ReplaceAdvance(rangeCopy, from, to);
}

INTRA_WARNING_POP

}}
