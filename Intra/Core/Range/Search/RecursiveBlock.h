#pragma once

#include "Core/Range/Concepts.h"
#include "Core/Range/Comparison.h"
#include "Core/Range/Search/Subrange.h"
#include "Core/Range/Take.h"
#include "Core/Range/Null.h"
#include "Core/Range/Operations.h"
#include "Core/Tuple.h"

INTRA_CORE_RANGE_BEGIN
template<typename R, typename OB, typename CB, typename ST, typename CBP, typename RCBP> INTRA_CONSTEXPR2 Requires<
	CNonInfiniteForwardRange<R> &&
	CAsNonInfiniteForwardRange<OB> &&
	CAsNonInfiniteForwardRange<CB> &&
	CAsNonInfiniteForwardRange<ST> &&
	CAsFiniteForwardRange<CBP> &&
	CAsFiniteForwardRange<RCBP>,
TTakeResult<R>> TakeRecursiveBlockAdvance(R& range, int& counter, size_t* ioIndex,
	const OB& openingBracket, const CB& closingBracket, const ST& stopToken,
	const CBP& commentBlocks, const RCBP& recursiveCommentBlocks)
{
	typedef TValueTypeOf<R> T;

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
			auto commentBlockBegin = GetField<0>(cblocks.First());
			auto commentBlockEnd = GetField<1>(cblocks.First());
			commentFound = StartsWith(range, commentBlockBegin);
			if(!commentFound) continue;

			const size_t commentBlockOpeningLen = Count(commentBlockBegin);
			const size_t commentBlockClosingLen = Count(commentBlockEnd);
			PopFirstN(range, commentBlockOpeningLen);
			index += commentBlockOpeningLen;
			FindAdvance(range, commentBlockEnd, &index);
			PopFirstN(range, commentBlockClosingLen);
			index += commentBlockClosingLen;
			break;
		}
		if(commentFound) continue;

		for(auto reccblocks = RangeOf(recursiveCommentBlocks); !reccblocks.Empty(); reccblocks.PopFirst())
		{
			auto commentBlockBegin = GetField<0>(reccblocks.First());
			auto commentBlockEnd = GetField<1>(reccblocks.First());
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
	if(ioIndex != null) *ioIndex += index;
	return Take(start, index);
}

template<typename R, typename OB, typename CB, typename ST> INTRA_CONSTEXPR2 Requires<
	CNonInfiniteForwardRange<R> &&
	CAsNonInfiniteForwardRange<OB> &&
	CAsNonInfiniteForwardRange<CB> &&
	CAsNonInfiniteForwardRange<ST>,
TTakeResult<R>> TakeRecursiveBlockAdvance(R& range, int& counter, size_t* ioIndex,
	const OB& openingBracket, const CB& closingBracket, const ST& stopToken)
{
	typedef TValueTypeOf<R> T;
	return TakeRecursiveBlockAdvance(range, counter, ioIndex,
		openingBracket, closingBracket, stopToken,
		NullRange<Tuple<NullRange<T>, NullRange<T>>>(),
		NullRange<Tuple<NullRange<T>, NullRange<T>>>());
}
INTRA_CORE_RANGE_END
