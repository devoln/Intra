#pragma once

#include "Range/Concepts.h"
#include "Algo/Search/Single.h"
#include "Algo/Search/Subrange.h"
#include "Range/Iteration/Zip.h"
#include "Range/Construction/TakeUntilAny.h"

namespace Intra { namespace Algo {

template<class R, class ResultRange, class ReplacementRange> Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsOutputRange<ResultRange>::_ &&
	Range::IsFiniteForwardRange<ReplacementRange>::_,
Range::ResultOfTake<ResultRange>> MultiReplaceToAdvance(const R& range,
	ResultRange& dstBuffer, const ReplacementRange& replacementSubranges)
{
	size_t index = 0;
	R src = range;
	ResultRange resultStart = dstBuffer;
	size_t substrIndex = 0;
	while(CopyToAdvance(
		Range::TakeUntilAdvanceAny(src, Range::Unzip<0>(replacementSubranges), &index, &substrIndex),
			dstBuffer), !src.Empty())
	{
		auto&& replacement = Range::AtIndex(replacementSubranges, substrIndex);
		CopyToAdvance(Meta::Get<1>(replacement), dstBuffer);
		src.PopFirstExactly(Range::Count(Meta::Get<0>(replacement)));
		index += Range::Count(Meta::Get<1>(replacement));
	}
	return Range::Take(resultStart, index);
}

template<typename R, typename ResultRange, typename ReplacementRange> forceinline Meta::EnableIf<
	Range::IsOutputRange<ResultRange>::_ &&
	Range::IsFiniteForwardRange<ReplacementRange>::_,
Range::ResultOfTake<ResultRange>> MultiReplaceTo(const R& range,
	const ResultRange& dstBuffer, const ReplacementRange& replacementSubranges)
{
	ResultRange dstRangeCopy = dstBuffer;
	return MultiReplaceToAdvance(range, dstRangeCopy, replacementSubranges);
}

}}
