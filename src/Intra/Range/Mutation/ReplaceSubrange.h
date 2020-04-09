#pragma once

#include "Intra/Range/Concepts.h"
#include "Intra/Container/Tuple.h"

#include "Intra/Range/Operations.h"
#include "Intra/Range/Search/Single.h"
#include "Intra/Range/Search/Subrange.h"
#include "Intra/Range/Zip.h"
#include "Intra/Range/TakeUntil.h"
#include "Intra/Range/TakeUntilAny.h"
#include "Intra/Range/Search/RecursiveBlock.h"
#include "Intra/Range/Mutation/Copy.h"

INTRA_BEGIN
template<class R, class OR, class RR> constexpr Requires<
	CNonInfiniteForwardRange<R> &&
	COutputRange<OR> &&
	CNonInfiniteForwardRange<RR>,
TTakeResult<OR>> MultiReplaceAdvanceToAdvance(R& src, OR& dstBuffer, const RR& replacementSubranges)
{
	index_t index = 0;
	auto resultStart = Forward<OR>(dstBuffer);
	index_t substrIndex = 0;
	while(WriteTo(
		TakeUntilAdvanceAny(src, Unzip<0>(replacementSubranges), OptRef(index), OptRef(substrIndex)),
			dstBuffer), !src.Empty())
	{
		auto&& replacement = AtIndex(replacementSubranges, substrIndex);
		WriteTo(get<1>(replacement), dstBuffer);
		PopFirstExactly(src, Count(get<0>(replacement)));
		index += Count(get<1>(replacement));
	}
	return Take(resultStart, index);
}

template<class R, class OR, class RR> constexpr Requires<
	CAsNonInfiniteForwardRange<R> &&
	COutputRange<OR> &&
	CAsNonInfiniteForwardRange<RR>,
TTakeResult<OR>> MultiReplaceToAdvance(R&& range,
	OR& dstBuffer, RR&& replacementSubranges)
{
	auto rangeCopy = ForwardAsRange<R>(range);
	return MultiReplaceAdvanceToAdvance(rangeCopy,
		dstBuffer, ForwardAsRange<RR>(replacementSubranges));
}

template<class R, class OR, class RR> constexpr inline Requires<
	CAsNonInfiniteForwardRange<R> &&
	CAsOutputRange<OR> &&
	CAsNonInfiniteForwardRange<RR>,
TTakeResult<TRangeOfRef<OR&&>>> MultiReplaceTo(R&& range,
	OR&& dstBuffer, RR&& replacementSubranges)
{
	auto dstRangeCopy = ForwardAsRange<OR>(dstBuffer);
	return MultiReplaceToAdvance(ForwardAsRange<R>(range),
		dstRangeCopy, ForwardAsRange<RR>(replacementSubranges));
}

template<typename R, typename OR, typename LR, typename RR,
	typename SubstitutionRangeOfTupleOfRanges, typename UnknownSubstitutionRange>
constexpr Requires<
	CAsNonInfiniteForwardRange<R> &&
	COutputRange<OR> &&
	CCopyConstructible<OR> &&
	CNonInfiniteForwardRange<LR> &&
	CNonInfiniteForwardRange<RR> &&
	CNonInfiniteForwardRange<SubstitutionRangeOfTupleOfRanges> &&
	CNonInfiniteForwardRange<UnknownSubstitutionRange>,
TTakeResult<TRangeOfRef<R>>> MultiSubstituteTo(R&& range, OR& dstBuffer,
	const LR& entryStart, const RR& entryEnd,
	const SubstitutionRangeOfTupleOfRanges& substitutions,
	const UnknownSubstitutionRange& unknownSubstitution)
{
	auto src = ForwardAsRange<R>(range);
	size_t index = 0;
	auto resultBufferStart = dstBuffer;
	while(WriteTo(TakeUntilAdvance(src, entryStart, &index), dstBuffer), !src.Empty())
	{
		PopFirstExactly(src, Count(entryStart));
		int counter = 1;
		auto entryStr = TakeRecursiveBlockAdvance(src, counter, &index, entryStart, entryEnd, null, null, null);
		if(counter>0)
		{
			INTRA_DEBUG_ASSERT(src.Empty());
			WriteTo(entryStr, dstBuffer);
			index += Count(entryStr);
			break;
		}
		PopLastExactly(entryStr, Count(entryEnd));
		auto substituionsCopy = substitutions;
		FindAdvance(substituionsCopy, TupleElementEquals<0>(entryStr));
		if(substituionsCopy.Empty())
		{
			WriteTo(unknownSubstitution, dstBuffer);
			index += Count(unknownSubstitution);
		}
		else
		{
			WriteTo(substituionsCopy.First(), dstBuffer);
			index += Count(substituionsCopy.First());
		}
	}
	return Take(resultBufferStart, index);
}
INTRA_END
