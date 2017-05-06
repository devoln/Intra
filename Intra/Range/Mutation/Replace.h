#pragma once

#include "Concepts/Range.h"
#include "Range/Search/Single.h"
#include "Cpp/Warnings.h"

namespace Intra { namespace Range {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename R> Meta::EnableIf<
	Concepts::IsAssignableRange<R>::_ &&
	!Meta::IsConst<R>::_
> ReplaceOneAdvance(R& range, const Concepts::ValueTypeOf<R>& from, const Concepts::ValueTypeOf<R>& to)
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

template<typename R,
	typename AsR = Concepts::RangeOfType<R>,
	typename T = Concepts::ValueTypeOf<AsR>
> Meta::EnableIf<
	Concepts::IsAsAssignableRange<R>::_
> ReplaceOne(R&& range, const T& from, const T& to)
{
	auto rangeCopy = Range::Forward<R>(range);
	ReplaceOneAdvance(rangeCopy, from, to);
}

template<typename R,
	typename T = Concepts::ValueTypeOf<R>
> Meta::EnableIf<
	Concepts::IsAssignableRange<R>::_ &&
	!Meta::IsConst<R>::_
> ReplaceAdvance(R& range, const T& from, const T& to)
{
	while(!range.Empty())
	{
		auto& v = range.First();
		if(v==from) v = to;
		range.PopFirst();
	}
}

template<typename R,
	typename AsR = Concepts::RangeOfType<R>,
	typename T = Concepts::ValueTypeOf<AsR>
> forceinline Meta::EnableIf<
	Concepts::IsAssignableRange<AsR>::_ &&
	!Meta::IsConst<R>::_
> Replace(R&& range, const T& from, const T& to)
{
	auto rangeCopy = Range::Forward<R>(range);
	ReplaceAdvance(rangeCopy, from, to);
}

INTRA_WARNING_POP

}}
