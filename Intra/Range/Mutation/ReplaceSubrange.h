#pragma once

#include "Concepts/Range.h"
#include "Range/Operations.h"
#include "Range/Search/Single.h"
#include "Range/Search/Subrange.h"
#include "Range/Compositors/Unzip.h"
#include "Range/Decorators/TakeUntil.h"
#include "Range/Decorators/TakeUntilAny.h"
#include "Range/Search/RecursiveBlock.h"
#include "Range/Mutation/Copy.h"
#include "Meta/GetField.h"

namespace Intra { namespace Range {

template<class R, class OR, class RR> Meta::EnableIf<
	Concepts::IsNonInfiniteForwardRange<R>::_ &&
	Concepts::IsOutputRange<OR>::_ &&
	Concepts::IsNonInfiniteForwardRange<RR>::_,
TakeResult<OR>> MultiReplaceAdvanceToAdvance(R& src, OR& dstBuffer, const RR& replacementSubranges)
{
	size_t index = 0;
	auto resultStart = Cpp::Forward<OR>(dstBuffer);
	size_t substrIndex = 0;
	while(WriteTo(
		TakeUntilAdvanceAny(src, Unzip<0>(replacementSubranges), &index, &substrIndex),
			dstBuffer), !src.Empty())
	{
		auto&& replacement = AtIndex(replacementSubranges, substrIndex);
		WriteTo(Meta::Get<1>(replacement), dstBuffer);
		PopFirstExactly(src, Count(Meta::Get<0>(replacement)));
		index += Count(Meta::Get<1>(replacement));
	}
	return Take(resultStart, index);
}

template<class R, class OR, class RR> Meta::EnableIf<
	Concepts::IsAsNonInfiniteForwardRange<R>::_ &&
	Concepts::IsOutputRange<OR>::_ &&
	Concepts::IsAsNonInfiniteForwardRange<RR>::_,
TakeResult<OR>> MultiReplaceToAdvance(R&& range,
	OR& dstBuffer, RR&& replacementSubranges)
{
	auto rangeCopy = Range::Forward<R>(range);
	return MultiReplaceAdvanceToAdvance(rangeCopy,
		dstBuffer, Range::Forward<RR>(replacementSubranges));
}

template<class R, class OR, class RR> inline Meta::EnableIf<
	Concepts::IsAsNonInfiniteForwardRange<R>::_ &&
	Concepts::IsAsOutputRange<OR>::_ &&
	Concepts::IsAsNonInfiniteForwardRange<RR>::_,
TakeResult<Concepts::RangeOfType<OR&&>>> MultiReplaceTo(R&& range,
	OR&& dstBuffer, RR&& replacementSubranges)
{
	auto dstRangeCopy = Range::Forward<OR>(dstBuffer);
	return MultiReplaceToAdvance(Range::Forward<R>(range),
		dstRangeCopy, Range::Forward<RR>(replacementSubranges));
}

template<typename R, typename OR, typename LR, typename RR,
	typename SubstitutionRangeOfTupleOfRanges, typename UnknownSubstitutionRange>
Meta::EnableIf<
	Concepts::IsAsNonInfiniteForwardRange<R>::_ &&
	Concepts::IsOutputRange<OR>::_ &&
	Meta::IsCopyConstructible<OR>::_ &&
	Concepts::IsNonInfiniteForwardRange<LR>::_ &&
	Concepts::IsNonInfiniteForwardRange<RR>::_ &&
	Concepts::IsNonInfiniteForwardRange<SubstitutionRangeOfTupleOfRanges>::_ &&
	Concepts::IsNonInfiniteForwardRange<UnknownSubstitutionRange>::_,
TakeResult<Concepts::RangeOfType<R>>> MultiSubstituteTo(R&& range, OR& dstBuffer,
	const LR& entryStart, const RR& entryEnd,
	const SubstitutionRangeOfTupleOfRanges& substitutions,
	const UnknownSubstitutionRange& unknownSubstitution)
{
	auto src = Range::Forward<R>(range);
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
		FindAdvance(substituionsCopy, Meta::TupleElementEquals<0>(entryStr));
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

}}
