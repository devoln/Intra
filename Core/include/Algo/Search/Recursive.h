#pragma once

#include "Platform/CppWarnings.h"
#include "Meta/Type.h"
#include "Range/Concepts.h"
#include "Algo/Comparison/StartsWith.h"
#include "Algo/Search/Subrange.h"
#include "Range/Decorators/Take.h"
#include "Range/Generators/Null.h"
#include "Range/Operations.h"

namespace Intra { namespace Algo {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename R,
	typename OpeningBracketRange,
	typename ClosingBracketRange,
	typename StopTokenRange,
	typename CommentBlockRangePairRange,
	typename RecursiveCommentBlockRangePairRange> Meta::EnableIf<
	Range::IsNonInfiniteForwardRange<R>::_ &&
	Range::IsNonInfiniteForwardRange<OpeningBracketRange>::_ &&
	Range::IsNonInfiniteForwardRange<ClosingBracketRange>::_ &&
	Range::IsNonInfiniteForwardRange<StopTokenRange>::_ &&
	Range::IsFiniteForwardRange<CommentBlockRangePairRange>::_ &&
	Range::IsFiniteForwardRange<RecursiveCommentBlockRangePairRange>::_,
Range::ResultOfTake<R>> TakeRecursiveBlockAdvance(R& range, int& counter, size_t* ioIndex,
	OpeningBracketRange openingBracket, ClosingBracketRange closingBracket, StopTokenRange stopToken,
	CommentBlockRangePairRange commentBlocks, RecursiveCommentBlockRangePairRange recursiveCommentBlocks)
{
	typedef Range::ValueTypeOf<R> T;
	auto start = range;
	size_t index = 0;
	const size_t openingBracketLen = Range::Count(openingBracket);
	const size_t closingBracketLen = Range::Count(closingBracket);
	const size_t stopTokenLen = Range::Count(stopToken);
	while(!range.Empty() && counter!=0)
	{
		if(openingBracketLen!=0 && Algo::StartsWith(range, openingBracket))
		{
			counter++;
			Range::PopFirstExactly(range, openingBracketLen);
			index += openingBracketLen;
			continue;
		}

		if(closingBracketLen!=0 && Algo::StartsWith(range, closingBracket))
		{
			counter--;
			Range::PopFirstExactly(range, closingBracketLen);
			index += closingBracketLen;
			continue;
		}

		if(stopTokenLen!=0 && Algo::StartsWith(range, stopToken))
		{
			Range::PopFirstExactly(range, stopTokenLen);
			index += stopTokenLen;
			break;
		}

		bool commentFound = false;
		for(auto cblocks = commentBlocks; !cblocks.Empty(); cblocks.PopFirst())
		{
			auto commentBlockBegin = Meta::Get<0>(cblocks.First());
			auto commentBlockEnd = Meta::Get<1>(cblocks.First());
			commentFound = Algo::StartsWith(range, commentBlockBegin);
			if(!commentFound) continue;

			const size_t commentBlockOpeningLen = Range::Count(commentBlockBegin);
			const size_t commentBlockClosingLen = Range::Count(commentBlockEnd);
			Range::PopFirstN(range, commentBlockOpeningLen);
			index += commentBlockOpeningLen;
			Algo::FindAdvance(range, commentBlockEnd, &index);
			Range::PopFirstN(range, commentBlockClosingLen);
			index += commentBlockClosingLen;
			break;
		}
		if(commentFound) continue;

		for(auto reccblocks = recursiveCommentBlocks; !reccblocks.Empty(); reccblocks.PopFirst())
		{
			auto commentBlockBegin = Meta::Get<0>(reccblocks.First());
			auto commentBlockEnd = Meta::Get<1>(reccblocks.First());
			commentFound = Algo::StartsWith(range, commentBlockBegin);
			if(!commentFound) continue;

			int commentCounter = 1;
			TakeRecursiveBlockAdvance(range, commentCounter, &index,
				commentBlockBegin, commentBlockEnd, Range::NullRange<T>(),
				Range::NullRange<Meta::Tuple<Range::NullRange<T>, Range::NullRange<T>>>(),
				Range::NullRange<Meta::Tuple<Range::NullRange<T>, Range::NullRange<T>>>());
			break;
		}
		if(commentFound) continue;

		range.PopFirst();
	}
	if(ioIndex!=null) *ioIndex += index;
	return Range::Take(start, index);
}

INTRA_WARNING_POP

}}
