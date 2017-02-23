#pragma once

#include "Range/Concepts.h"
#include "Range/Operations.h"
#include "Algo/Search/Single.h"
#include "Algo/Search/Subrange.h"
#include "Range/Compositors/Zip.h"
#include "Range/Decorators/TakeUntil.h"
#include "Range/Decorators/TakeUntilAny.h"
#include "Algo/Search/Recursive.h"
#include "Algo/Mutation/Copy.h"

namespace Intra { namespace Algo {

using namespace Range::Concepts;

template<class R, class OR, class RR> Meta::EnableIf<
	IsNonInfiniteForwardRange<R>::_ &&
	IsOutputRange<OR>::_ &&
	IsNonInfiniteForwardRange<RR>::_,
TakeResult<OR>> MultiReplaceAdvanceToAdvance(R& src, OR& dstBuffer, const RR& replacementSubranges)
{
	using namespace Range;

	size_t index = 0;
	auto resultStart = Meta::Forward<OR>(dstBuffer);
	size_t substrIndex = 0;
	while(CopyToAdvance(
		TakeUntilAdvanceAny(src, Unzip<0>(replacementSubranges), &index, &substrIndex),
			dstBuffer), !src.Empty())
	{
		auto&& replacement = AtIndex(replacementSubranges, substrIndex);
		CopyToAdvance(Meta::Get<1>(replacement), dstBuffer);
		PopFirstExactly(src, Count(Meta::Get<0>(replacement)));
		index += Count(Meta::Get<1>(replacement));
	}
	return Take(resultStart, index);
}

template<class R, class OR, class RR> Meta::EnableIf<
	IsAsNonInfiniteForwardRange<R>::_ &&
	IsOutputRange<OR>::_ &&
	IsAsNonInfiniteForwardRange<RR>::_,
TakeResult<OR>> MultiReplaceToAdvance(R&& range,
	OR& dstBuffer, RR&& replacementSubranges)
{
	auto rangeCopy = Range::Forward<R>(range);
	return MultiReplaceAdvanceToAdvance(rangeCopy,
		dstBuffer, Range::Forward<RR>(replacementSubranges));
}

template<class R, class OR, class RR> inline Meta::EnableIf<
	IsAsNonInfiniteForwardRange<R>::_ &&
	IsAsOutputRange<OR>::_ &&
	IsAsNonInfiniteForwardRange<RR>::_,
TakeResult<AsRangeResult<OR&&>>> MultiReplaceTo(R&& range,
	OR&& dstBuffer, RR&& replacementSubranges)
{
	auto dstRangeCopy = Range::Forward<OR>(dstBuffer);
	return MultiReplaceToAdvance(Range::Forward<R>(range),
		dstRangeCopy, Range::Forward<RR>(replacementSubranges));
}

template<typename R, typename OR, typename LR, typename RR,
	typename SubstitutionRangeOfTupleOfRanges, typename UnknownSubstitutionRange> Meta::EnableIf<
	IsAsNonInfiniteForwardRange<R>::_ &&
	IsOutputRange<OR>::_ && Meta::IsCopyConstructible<OR>::_ &&
	IsNonInfiniteForwardRange<LR>::_ &&
	IsNonInfiniteForwardRange<RR>::_ &&
	IsNonInfiniteForwardRange<SubstitutionRangeOfTupleOfRanges>::_ &&
	IsNonInfiniteForwardRange<UnknownSubstitutionRange>::_,
TakeResult<AsRangeResult<R>>> MultiSubstituteTo(R&& range, OR& dstBuffer,
	const LR& entryStart, const RR& entryEnd,
	const SubstitutionRangeOfTupleOfRanges& substitutions,
	const UnknownSubstitutionRange& unknownSubstitution)
{
	using namespace Range;

	auto src = Range::Forward<R>(range);
	size_t index = 0;
	auto resultBufferStart = dstBuffer;
	while(CopyToAdvance(TakeUntilAdvance(src, entryStart, &index), dstBuffer), !src.Empty())
	{
		PopFirstExactly(src, Count(entryStart));
		int counter = 1;
		auto entryStr = TakeRecursiveBlockAdvance(src, counter, &index, entryStart, entryEnd, null, null, null);
		if(counter>0)
		{
			INTRA_ASSERT(src.Empty());
			CopyToAdvance(entryStr, dstBuffer);
			index += Count(entryStr);
			break;
		}
		PopLastExactly(entryStr, Count(entryEnd));
		auto substituionsCopy = substitutions;
		Algo::FindAdvance(substituionsCopy, Meta::TupleElementEquals<0>(entryStr));
		if(substituionsCopy.Empty())
		{
			CopyToAdvance(unknownSubstitution, dstBuffer);
			index += Count(unknownSubstitution);
		}
		else
		{
			CopyToAdvance(substituionsCopy.First(), dstBuffer);
			index += Count(substituionsCopy.First());
		}
	}
	return Take(resultBufferStart, index);
}

}}
