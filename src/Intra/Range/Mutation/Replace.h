#pragma once

#include "Intra/Range/Concepts.h"
#include "Intra/Range/Search/Single.h"

INTRA_BEGIN
template<typename R> constexpr Requires<
	CAssignableRange<R> &&
	!CConst<R>
> ReplaceOneAdvance(R& range, const TRangeValue<R>& from, const TRangeValue<R>& to)
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
	typename AsR = TRangeOfRef<R>,
	typename T = TRangeValue<AsR>
> constexpr Requires<
	CAssignableList<R>
> ReplaceOne(R&& range, const T& from, const T& to)
{
	auto rangeCopy = ForwardAsRange<R>(range);
	ReplaceOneAdvance(rangeCopy, from, to);
}

template<typename R, typename T = TRangeValue<R>
> constexpr Requires<
	CAssignableRange<R> &&
	!CConst<R>
> ReplaceAdvance(R& range, const T& from, const T& to)
{
	while(!range.Empty())
	{
		auto& v = range.First();
		if(v == from) v = to;
		range.PopFirst();
	}
}

template<typename R,
	typename AsR = TRangeOfRef<R>,
	typename T = TRangeValue<AsR>
> constexpr Requires<
	CAssignableRange<AsR> &&
	!CConst<R>
> Replace(R&& range, const T& from, const T& to)
{
	auto rangeCopy = ForwardAsRange<R>(range);
	ReplaceAdvance(rangeCopy, from, to);
}
INTRA_END
