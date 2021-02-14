#pragma once

#include "Intra/Functional.h"
#include "Intra/Concepts.h"
#include "Intra/Misc/RawMemory.h"
#include "Intra/Range/Operations.h"

namespace Intra { INTRA_BEGIN

/** Checks whether ``range`` starts with ``what`` prefix and consume it if it does.

  If ``range`` starts with ``what``, then ``range``'s start moves to the position immediately after occurence of ``what``.
  Otherwise ``range`` remains unchanged.
  @returns true if prefix was consumed.
*/
template<typename R, typename RW> constexpr Requires<
	CForwardRange<R> &&
	CConsumableList<RW>,
bool> ConsumePrefix(R& range, RW&& prefix)
{
	bool result = range|StartsWith(prefix);
	if(result) range|PopFirstExactly{Count(prefix)};
	return result;
}

template<typename R, typename RWs> constexpr Requires<
	CForwardRange<R> &&
	CNonInfiniteRange<RWs> &&
	CNonInfiniteForwardRange<TRangeValue<RWs>> &&
	CConvertibleTo<TRangeValue<TRangeValue<RWs>>, TRangeValue<R>>,
bool> StartsWithAnyAdvance(const R& range, RWs& subranges, Optional<index_t&> oSubrangeIndex = nullptr)
{
	if(oSubrangeIndex) oSubrangeIndex.Unwrap() = 0;
	while(!subranges.Empty())
	{
		if(StartsWith(range, subranges.First())) return true;
		if(oSubrangeIndex) oSubrangeIndex.Unwrap()++;
		subranges.PopFirst();
	}
	return false;
}

template<typename R, typename RWs> [[nodiscard]] constexpr Requires<
	CForwardList<R> &&
	CConsumableList<RWs> &&
	CNonInfiniteForwardList<TListValue<RWs>> &&
	CConvertibleTo<TListValue<TListValue<RWs>>, TListValue<R>>,
bool> StartsWithAny(R&& range, RWs&& subranges, Optional<index_t&> oSubrangeIndex = nullptr)
{
	auto subrangesCopy = ForwardAsRange<RWs>(subranges);
	return StartsWithAnyAdvance(ForwardAsRange<R>(range), subrangesCopy, oSubrangeIndex);
}

template<typename R, typename RWs,
	typename W = TListValue<RWs>
> [[nodiscard]] constexpr Requires<
	CForwardRange<R> &&
	CConsumableList<RWs> &&
	CNonInfiniteForwardList<W> &&
	CConvertibleTo<TListValue<W>, TRangeValue<R>>,
bool> StartsAdvanceWithAny(R& range, RWs&& subranges, Optional<index_t&> oSubrangeIndex = nullptr)
{
	auto subrangesCopy = ForwardAsRange<RWs>(subranges);
	bool result = StartsWithAnyAdvance(range, subrangesCopy, oSubrangeIndex);
	if(result) range|PopFirstExactly(Count(subrangesCopy.First()));
	return result;
}
} INTRA_END
