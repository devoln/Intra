#pragma once

#include "Range/Concepts.h"
#include "Algo/Search/Single.h"
#include "Platform/CppWarnings.h"

namespace Intra { namespace Algo {

using namespace Range::Concepts;

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename R> Meta::EnableIf<
	IsAssignableRange<R>::_ && !Meta::IsConst<R>::_
> ReplaceOneAdvance(R&& range, const ValueTypeOf<R>& from, const ValueTypeOf<R>& to)
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
	IsAsAssignableRange<R>::_
> ReplaceOne(R&& range, const ValueTypeOf<R>& from, const ValueTypeOf<R>& to)
{
	auto rangeCopy = Range::Forward<R>(range);
	ReplaceOneAdvance(rangeCopy, from, to);
}

template<typename R> Meta::EnableIf<
	IsAssignableRange<R>::_ && !Meta::IsConst<R>::_
> ReplaceAdvance(R&& range, const ValueTypeOf<R>& from, const ValueTypeOf<R>& to)
{
	while(!range.Empty())
	{
		auto& v = range.First();
		if(v==from) v = to;
		range.PopFirst();
	}
}

template<typename R> forceinline Meta::EnableIf<
	IsAsAssignableRange<R>::_ && !Meta::IsConst<R>::_
> Replace(R&& range, const ValueTypeOf<R>& from, const ValueTypeOf<R>& to)
{
	auto rangeCopy = Range::Forward<R>(range);
	ReplaceAdvance(rangeCopy, from, to);
}

INTRA_WARNING_POP

}}
