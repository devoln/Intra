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

template<class R, class OR, class RR> Meta::EnableIf<
	Range::IsNonInfiniteForwardRange<R>::_ &&
	Range::IsOutputRange<OR>::_ &&
	Range::IsNonInfiniteForwardRange<RR>::_,
Range::ResultOfTake<OR>> MultiReplaceAdvanceToAdvance(R& src, OR& dstBuffer, const RR& replacementSubranges)
{
	size_t index = 0;
	auto resultStart = Meta::Forward<OR>(dstBuffer);
	size_t substrIndex = 0;
	while(Algo::CopyToAdvance(
		Range::TakeUntilAdvanceAny(src, Range::Unzip<0>(replacementSubranges), &index, &substrIndex),
			dstBuffer), !src.Empty())
	{
		auto&& replacement = Range::AtIndex(replacementSubranges, substrIndex);
		Algo::CopyToAdvance(Meta::Get<1>(replacement), dstBuffer);
		Range::PopFirstExactly(src, Range::Count(Meta::Get<0>(replacement)));
		index += Range::Count(Meta::Get<1>(replacement));
	}
	return Range::Take(resultStart, index);
}

template<class R, class OR, class RR> Meta::EnableIf<
	Range::IsAsNonInfiniteForwardRange<R>::_ &&
	Range::IsOutputRange<OR>::_ &&
	Range::IsAsNonInfiniteForwardRange<RR>::_,
Range::ResultOfTake<OR>> MultiReplaceToAdvance(R&& range,
	OR& dstBuffer, RR&& replacementSubranges)
{
	auto rangeCopy = Range::Forward<R>(range);
	return Algo::MultiReplaceAdvanceToAdvance(rangeCopy,
		dstBuffer, Range::Forward<RR>(replacementSubranges));
}

template<class R, class OR, class RR> inline Meta::EnableIf<
	Range::IsAsNonInfiniteForwardRange<R>::_ &&
	Range::IsAsOutputRange<OR>::_ &&
	Range::IsAsNonInfiniteForwardRange<RR>::_,
Range::ResultOfTake<Range::AsRangeResult<OR&&>>> MultiReplaceTo(R&& range,
	OR&& dstBuffer, RR&& replacementSubranges)
{
	auto dstRangeCopy = Range::Forward<OR>(dstBuffer);
	return Algo::MultiReplaceToAdvance(Range::Forward<R>(range),
		dstRangeCopy, Range::Forward<RR>(replacementSubranges));
}

template<typename R, typename OR, typename LR, typename RR,
	typename SubstitutionRangeOfTupleOfRanges, typename UnknownSubstitutionRange> Meta::EnableIf<
	Range::IsAsNonInfiniteForwardRange<R>::_ &&
	Range::IsOutputRange<OR>::_ && Meta::IsCopyConstructible<OR>::_ &&
	Range::IsNonInfiniteForwardRange<LR>::_ &&
	Range::IsNonInfiniteForwardRange<RR>::_ &&
	Range::IsNonInfiniteForwardRange<SubstitutionRangeOfTupleOfRanges>::_ &&
	Range::IsNonInfiniteForwardRange<UnknownSubstitutionRange>::_,
Range::ResultOfTake<Range::AsRangeResult<R>>> MultiSubstituteTo(R&& range, OR& dstBuffer,
	const LR& entryStart, const RR& entryEnd,
	const SubstitutionRangeOfTupleOfRanges& substitutions,
	const UnknownSubstitutionRange& unknownSubstitution)
{
	auto src = Range::Forward<R>(range);
	size_t index = 0;
	auto resultBufferStart = dstBuffer;
	while(CopyToAdvance(Range::TakeUntilAdvance(src, entryStart, &index), dstBuffer), !src.Empty())
	{
		Range::PopFirstExactly(src, Range::Count(entryStart));
		int counter = 1;
		auto entryStr = Algo::TakeRecursiveBlockAdvance(src, counter, &index, entryStart, entryEnd, null, null, null);
		if(counter>0)
		{
			INTRA_ASSERT(src.Empty());
			Algo::CopyToAdvance(entryStr, dstBuffer);
			index += Range::Count(entryStr);
			break;
		}
		Range::PopLastExactly(entryStr, Range::Count(entryEnd));
		auto substituionsCopy = substitutions;
		Algo::FindAdvance(substituionsCopy, Meta::TupleElementEquals<0>(entryStr));
		if(substituionsCopy.Empty())
		{
			Algo::CopyToAdvance(unknownSubstitution, dstBuffer);
			index += Range::Count(unknownSubstitution);
		}
		else
		{
			Algo::CopyToAdvance(substituionsCopy.First(), dstBuffer);
			index += Range::Count(substituionsCopy.First());
		}
	}
	return Range::Take(resultBufferStart, index);
}

}}
