#pragma once

#include "Core/Range/Concepts.h"
#include "Core/Range/Search/Single.h"

INTRA_BEGIN
template<typename R> constexpr Requires<
	CAssignableRange<R> &&
	!CConst<R>
> ReplaceOneAdvance(R& range, const TValueTypeOf<R>& from, const TValueTypeOf<R>& to)
{
	while(!range.Empty())
	{
		auto& v = range.First();
		if(v == from)
		{
			v = to;
			range.PopFirst();
			return;
		}
		range.PopFirst();
	}
}

template<typename R,
	typename AsR = TRangeOfType<R>,
	typename T = TValueTypeOf<AsR>
> constexpr forceinline Requires<
	CAsAssignableRange<R>
> ReplaceOne(R&& range, const T& from, const T& to)
{
	auto rangeCopy = ForwardAsRange<R>(range);
	ReplaceOneAdvance(rangeCopy, from, to);
}

template<typename R,
	typename T = TValueTypeOf<R>
> Requires<
	CAssignableRange<R> &&
	!CConst<R>
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
	typename AsR = TRangeOfType<R>,
	typename T = TValueTypeOf<AsR>
> forceinline Requires<
	CAssignableRange<AsR> &&
	!CConst<R>
> Replace(R&& range, const T& from, const T& to)
{
	auto rangeCopy = ForwardAsRange<R>(range);
	ReplaceAdvance(rangeCopy, from, to);
}
INTRA_END
