#pragma once

#include "Intra/Range/Concepts.h"
#include "Intra/Range/Comparison.h"
#include "Intra/Range/Search/Subrange.h"
#include "Intra/Range/Take.h"
#include "Intra/Range/Null.h"
#include "Intra/Range/Operations.h"
#include "Intra/Container/Tuple.h"

namespace Intra { INTRA_BEGIN
template<typename R, typename OB, typename CB, typename ST, typename CBP, typename RCBP> constexpr Requires<
	CNonInfiniteForwardRange<R> &&
	CNonInfiniteForwardList<OB> &&
	CNonInfiniteForwardList<CB> &&
	CNonInfiniteForwardList<ST> &&
	CFiniteForwardList<CBP> &&
	CFiniteForwardList<RCBP>,
TTakeResult<R>> TakeRecursiveBlockAdvance(R& range, int& counter, size_t* ioIndex,
	const OB& openingBracket, const CB& closingBracket, const ST& stopToken,
	const CBP& commentBlocks, const RCBP& recursiveCommentBlocks)
{
	typedef TRangeValue<R> T;

	auto start = range;
	size_t index = 0;
	const size_t openingBracketLen = Count(openingBracket);
	const size_t closingBracketLen = Count(closingBracket);
	const size_t stopTokenLen = Count(stopToken);
	while(!range.Empty() && counter != 0)
	{
		if(openingBracketLen!=0 && StartsWith(range, openingBracket))
		{
			counter++;
			PopFirstExactly(range, openingBracketLen);
			index += openingBracketLen;
			continue;
		}

		if(closingBracketLen!=0 && StartsWith(range, closingBracket))
		{
			counter--;
			PopFirstExactly(range, closingBracketLen);
			index += closingBracketLen;
			continue;
		}

		if(stopTokenLen!=0 && StartsWith(range, stopToken))
		{
			PopFirstExactly(range, stopTokenLen);
			index += stopTokenLen;
			break;
		}

		bool commentFound = false;
		for(auto cblocks = RangeOf(commentBlocks); !cblocks.Empty(); cblocks.PopFirst())
		{
			auto commentBlockBegin = get<0>(cblocks.First());
			auto commentBlockEnd = get<1>(cblocks.First());
			commentFound = StartsWith(range, commentBlockBegin);
			if(!commentFound) continue;

			const size_t commentBlockOpeningLen = Count(commentBlockBegin);
			const size_t commentBlockClosingLen = Count(commentBlockEnd);
			PopFirstCount(range, commentBlockOpeningLen);
			index += commentBlockOpeningLen;
			FindAdvance(range, commentBlockEnd, OptRef(index));
			PopFirstCount(range, commentBlockClosingLen);
			index += commentBlockClosingLen;
			break;
		}
		if(commentFound) continue;

		for(auto reccblocks = RangeOf(recursiveCommentBlocks); !reccblocks.Empty(); reccblocks.PopFirst())
		{
			auto commentBlockBegin = get<0>(reccblocks.First());
			auto commentBlockEnd = get<1>(reccblocks.First());
			commentFound = StartsWith(range, commentBlockBegin);
			if(!commentFound) continue;

			int commentCounter = 1;
			TakeRecursiveBlockAdvance(range, commentCounter, &index,
				commentBlockBegin, commentBlockEnd, NullRange<T>(),
				NullRange<Tuple<NullRange<T>, NullRange<T>>>(),
				NullRange<Tuple<NullRange<T>, NullRange<T>>>());
			break;
		}
		if(commentFound) continue;

		range.PopFirst();
	}
	if(ioIndex != nullptr) *ioIndex += index;
	return Take(start, index);
}

template<typename R, typename OB, typename CB, typename ST> constexpr Requires<
	CNonInfiniteForwardRange<R> &&
	CNonInfiniteForwardList<OB> &&
	CNonInfiniteForwardList<CB> &&
	CNonInfiniteForwardList<ST>,
TTakeResult<R>> TakeRecursiveBlockAdvance(R& range, int& counter, size_t* ioIndex,
	const OB& openingBracket, const CB& closingBracket, const ST& stopToken)
{
	typedef TRangeValue<R> T;
	return TakeRecursiveBlockAdvance(range, counter, ioIndex,
		openingBracket, closingBracket, stopToken,
		NullRange<Tuple<NullRange<T>, NullRange<T>>>(),
		NullRange<Tuple<NullRange<T>, NullRange<T>>>());
}
} INTRA_END
