#pragma once

#include <Intra/Functional.h>
#include <Intra/Range.h>

namespace Intra { INTRA_BEGIN

/// Check whether range starts with prefix prefix and consume it if it does.
/// If range starts with prefix, then range's start moves to the position immediately after occurence of prefix.
/// Otherwise range remains unchanged.
/// @return true if prefix has been consumed.
template<CForwardRange R, CConsumableList RW> constexpr bool ConsumePrefix(R& range, RW&& prefix)
{
	bool result = range|StartsWith(prefix);
	if(result) range|PopFirstExactly(prefix|Count);
	return result;
}

template<CForwardRange R, CNonInfiniteRange RWs> requires CNonInfiniteForwardRange<TRangeValue<RWs>> && CConvertibleTo<TRangeValue<TRangeValue<RWs>>, TRangeValue<R>>
constexpr bool StartsWithAnyAdvance(const R& range, RWs& subranges, Optional<index_t&> oSubrangeIndex = {})
{
	if(oSubrangeIndex) oSubrangeIndex.Unwrap() = 0;
	while(!subranges.Empty())
	{
		if(range|StartsWith(subranges.First())) return true;
		if(oSubrangeIndex) oSubrangeIndex.Unwrap()++;
		subranges.PopFirst();
	}
	return false;
}

template<CForwardList R, CConsumableList RWs> requires CNonInfiniteForwardList<TListValue<RWs>> && CConvertibleTo<TListValue<TListValue<RWs>>, TListValue<R>>
[[nodiscard]] constexpr bool StartsWithAny(R&& range, RWs&& subranges, Optional<index_t&> oSubrangeIndex = {})
{
	auto subrangesCopy = RangeOf(INTRA_FWD(subranges));
	return StartsWithAnyAdvance(RangeOf(INTRA_FWD(range)), subrangesCopy, oSubrangeIndex);
}

template<CForwardRange R, CConsumableList RWs, typename W = TListValue<RWs>> requires CNonInfiniteForwardList<W> && CConvertibleTo<TListValue<W>, TRangeValue<R>>
[[nodiscard]] constexpr bool StartsAdvanceWithAny(R& range, RWs&& subranges, Optional<index_t&> oSubrangeIndex = {})
{
	auto subrangesCopy = RangeOf(INTRA_FWD(subranges));
	bool result = StartsWithAnyAdvance(range, subrangesCopy, oSubrangeIndex);
	if(result) range|PopFirstExactly(Count(subrangesCopy.First()));
	return result;
}
} INTRA_END
