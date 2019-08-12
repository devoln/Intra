#pragma once

#include "Core/Range/Concepts.h"
#include "Core/Tuple.h"

#include "Core/Range/Operations.h"
#include "Core/Range/Search/Single.h"
#include "Core/Range/Search/Subrange.h"
#include "Core/Range/Zip.h"
#include "Core/Range/TakeUntil.h"
#include "Core/Range/TakeUntilAny.h"
#include "Core/Range/Search/RecursiveBlock.h"
#include "Core/Range/Mutation/Copy.h"

INTRA_CORE_RANGE_BEGIN
template<class R, class OR, class RR> INTRA_CONSTEXPR2 Requires<
	CNonInfiniteForwardRange<R> &&
	COutputRange<OR> &&
	CNonInfiniteForwardRange<RR>,
TTakeResult<OR>> MultiReplaceAdvanceToAdvance(R& src, OR& dstBuffer, const RR& replacementSubranges)
{
	size_t index = 0;
	auto resultStart = Forward<OR>(dstBuffer);
	size_t substrIndex = 0;
	while(WriteTo(
		TakeUntilAdvanceAny(src, Unzip<0>(replacementSubranges), &index, &substrIndex),
			dstBuffer), !src.Empty())
	{
		auto&& replacement = AtIndex(replacementSubranges, substrIndex);
		WriteTo(GetField<1>(replacement), dstBuffer);
		PopFirstExactly(src, Count(GetField<0>(replacement)));
		index += Count(GetField<1>(replacement));
	}
	return Take(resultStart, index);
}

template<class R, class OR, class RR> INTRA_CONSTEXPR2 Requires<
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

template<class R, class OR, class RR> INTRA_CONSTEXPR2 inline Requires<
	CAsNonInfiniteForwardRange<R> &&
	CAsOutputRange<OR> &&
	CAsNonInfiniteForwardRange<RR>,
TTakeResult<TRangeOfType<OR&&>>> MultiReplaceTo(R&& range,
	OR&& dstBuffer, RR&& replacementSubranges)
{
	auto dstRangeCopy = ForwardAsRange<OR>(dstBuffer);
	return MultiReplaceToAdvance(ForwardAsRange<R>(range),
		dstRangeCopy, ForwardAsRange<RR>(replacementSubranges));
}

template<typename R, typename OR, typename LR, typename RR,
	typename SubstitutionRangeOfTupleOfRanges, typename UnknownSubstitutionRange>
INTRA_CONSTEXPR2 Requires<
	CAsNonInfiniteForwardRange<R> &&
	COutputRange<OR> &&
	CCopyConstructible<OR> &&
	CNonInfiniteForwardRange<LR> &&
	CNonInfiniteForwardRange<RR> &&
	CNonInfiniteForwardRange<SubstitutionRangeOfTupleOfRanges> &&
	CNonInfiniteForwardRange<UnknownSubstitutionRange>,
TTakeResult<TRangeOfType<R>>> MultiSubstituteTo(R&& range, OR& dstBuffer,
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
INTRA_CORE_RANGE_END
