#pragma once

#include "Intra/Functional.h"
#include "Intra/Concepts.h"
#include "Intra/Range/Concepts.h"
#include "Intra/Misc/RawMemory.h"
#include "Intra/Range/Operations.h"

INTRA_BEGIN

/** Checks whether ``range`` starts with ``what`` prefix and consume it if it does.

  If ``range`` starts with ``what``, then ``range``'s start moves to the position immediately after occurence of ``what``.
  Otherwise ``range`` remains unchanged.
  @returns true if prefix was consumed.
*/
template<typename R, typename RW> constexpr Requires<
	CForwardRange<R> &&
	CAsConsumableRange<RW>,
bool> ConsumePrefix(R& range, RW&& prefix)
{
	bool result = range|StartsWith(prefix);
	if(result) range|PopFirstExactly{Count(prefix)};
	return result;
}

template<typename R, typename RWs> constexpr Requires<
	CForwardRange<R> &&
	CNonInfiniteInputRange<RWs> &&
	CNonInfiniteForwardRange<TValueTypeOf<RWs>> &&
	CConvertibleTo<TValueTypeOf<TValueTypeOf<RWs>>, TValueTypeOf<R>>,
bool> StartsWithAnyAdvance(const R& range, RWs& subranges, Optional<index_t&> oSubrangeIndex = null)
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
	CAsForwardRange<R> &&
	CAsConsumableRange<RWs> &&
	CAsNonInfiniteForwardRange<TValueTypeOfAs<RWs>> &&
	CConvertibleTo<TValueTypeOfAs<TValueTypeOfAs<RWs>>, TValueTypeOfAs<R>>,
bool> StartsWithAny(R&& range, RWs&& subranges, Optional<index_t&> oSubrangeIndex = null)
{
	auto subrangesCopy = ForwardAsRange<RWs>(subranges);
	return StartsWithAnyAdvance(ForwardAsRange<R>(range), subrangesCopy, oSubrangeIndex);
}

template<typename R, typename RWs,
	typename W = TValueTypeOfAs<RWs>
> [[nodiscard]] constexpr Requires<
	CForwardRange<R> &&
	CAsConsumableRange<RWs> &&
	CAsNonInfiniteForwardRange<W> &&
	CConvertibleTo<TValueTypeOfAs<W>, TValueTypeOf<R>>,
bool> StartsAdvanceWithAny(R& range, RWs&& subranges, Optional<index_t&> oSubrangeIndex = null)
{
	auto subrangesCopy = ForwardAsRange<RWs>(subranges);
	bool result = StartsWithAnyAdvance(range, subrangesCopy, oSubrangeIndex);
	if(result) range|PopFirstExactly(Count(subrangesCopy.First()));
	return result;
}
INTRA_END
