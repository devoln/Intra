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

namespace Intra { INTRA_BEGIN
template<class R, class OR, class RR> constexpr Requires<
	CNonInfiniteForwardRange<R> &&
	COutput<OR> &&
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

template<CNonInfiniteForwardList R, COutput OR, CNonInfiniteForwardList RR>
constexpr TTakeResult<OR> MultiReplaceToAdvance(R&& range, OR& dstBuffer, RR&& replacementSubranges)
{
	auto rangeCopy = RangeOf(INTRA_FWD(range));
	return MultiReplaceAdvanceToAdvance(rangeCopy,
		dstBuffer, ForwardAsRange<RR>(replacementSubranges));
}

template<CNonInfiniteForwardList R, CAsOutputRange OR, CNonInfiniteForwardList RR>
constexpr TTakeResult<TRangeOfRef<OR&&>> MultiReplaceTo(R&& range, OR&& dstBuffer, RR&& replacementSubranges)
{
	auto dstRangeCopy = RangeOf(INTRA_FWD(dstBuffer));
	return MultiReplaceToAdvance(RangeOf(INTRA_FWD(range)), dstRangeCopy, RangeOf(INTRA_FWD(replacementSubranges)));
}

template<CNonInfiniteForwardList R, COutput OR, CNonInfiniteForwardRange LR, CNonInfiniteForwardRange RR,
	CNonInfiniteForwardRange SubstitutionRangeOfTupleOfRanges, CNonInfiniteForwardRange UnknownSubstitutionRange> requires CCopyConstructible<OR>
constexpr TTakeResult<TRangeOfRef<R>>> MultiSubstituteTo(R&& range, OR& dstBuffer,
	const LR& entryStart, const RR& entryEnd,
	const SubstitutionRangeOfTupleOfRanges& substitutions,
	const UnknownSubstitutionRange& unknownSubstitution)
{
	auto src = RangeOf(INTRA_FWD(range));
	size_t index = 0;
	auto resultBufferStart = dstBuffer;
	while(WriteTo(TakeUntilAdvance(src, entryStart, &index), dstBuffer), !src.Empty())
	{
		PopFirstExactly(src, Count(entryStart));
		int counter = 1;
		auto entryStr = TakeRecursiveBlockAdvance(src, counter, &index, entryStart, entryEnd, nullptr, nullptr, nullptr);
		if(counter>0)
		{
			INTRA_DEBUG_ASSERT(src.Empty());
			WriteTo(entryStr, dstBuffer);
			index += Count(entryStr);
			break;
		}
		entryStr|PopLastExactly(Count(entryEnd));
		auto substituionsCopy = substitutions;
		FindAdvance(substituionsCopy, [&](auto&& t) {return AtField<0>(t) == entryStr;});
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
} INTRA_END
