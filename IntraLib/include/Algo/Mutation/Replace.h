#pragma once

#include "Range/Concepts.h"
#include "Algo/Search/Single.h"

namespace Intra { namespace Algo {

template<typename R> Meta::EnableIf<
	Range::IsAssignableInputRange<R>::_ && !Meta::IsConst<R>::_
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
	Range::IsAssignableInputRange<R>::_ && !Meta::IsConst<R>::_
> ReplaceOne(const R& range, const Range::ValueTypeOf<R>& from, const Range::ValueTypeOf<R>& to)
{ReplaceOneAdvance(R(range), from, to);}

template<typename R> Meta::EnableIf<
	Range::IsAssignableInputRange<R>::_ && !Meta::IsConst<R>::_
> ReplaceAdvance(R&& range, const Range::ValueTypeOf<R>& from, const Range::ValueTypeOf<R>& to)
{
	while(!range.Empty())
	{
		auto& v = range.First();
		if(v==from) v=to;
		range.PopFirst();
	}
}

template<typename R> Meta::EnableIf<
	Range::IsAssignableInputRange<R>::_ && !Meta::IsConst<R>::_
> Replace(const R& range, const Range::ValueTypeOf<R>& from, const Range::ValueTypeOf<R>& to)
{ReplaceAdvance(R(range), from, to);}



}}

