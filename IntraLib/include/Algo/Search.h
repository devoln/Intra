#pragma once

#include "Platform/CppWarnings.h"
#include "Meta/Type.h"
#include "Range/Concepts.h"
#include "Range/Operations.h"
#include "Range/Construction/Take.h"
#include "Range/Construction/Null.h"

#include "Algo/Comparison.h"
#include "Algo/Mutation.h"

#include "Search/Trim.h"
#include "Search/Single.h"
#include "Search/Subrange.h"
#include "Search/Binary.h"

namespace Intra {

namespace Range {template<typename T> struct ArrayRange;}

namespace Algo {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename R,
	typename OpeningBracketRange,
	typename ClosingBracketRange,
	typename StopTokenRange,
	typename CommentBlockRangePairRange,
	typename RecursiveCommentBlockRangePairRange> Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsFiniteForwardRange<OpeningBracketRange>::_ &&
	Range::IsFiniteForwardRange<ClosingBracketRange>::_ &&
	Range::IsFiniteForwardRange<StopTokenRange>::_ &&
	Range::IsFiniteForwardRange<CommentBlockRangePairRange>::_ &&
	Range::IsFiniteForwardRange<RecursiveCommentBlockRangePairRange>::_,
Range::ResultOfTake<R>> ReadRecursiveBlockAdvance(R& range, int& counter, size_t* ioIndex,
	OpeningBracketRange openingBracket, ClosingBracketRange closingBracket, StopTokenRange stopToken,
	CommentBlockRangePairRange commentBlocks, RecursiveCommentBlockRangePairRange recursiveCommentBlocks)
{
	typedef Range::ValueTypeOf<R> T;
	R start = range;
	size_t index = 0;
	const size_t openingBracketLen = Range::Count(openingBracket);
	const size_t closingBracketLen = Range::Count(closingBracket);
	const size_t stopTokenLen = Range::Count(stopToken);
	while(!range.Empty() && counter!=0)
	{
		if(openingBracketLen!=0 && StartsWith(range, openingBracket))
		{
			counter++;
			range.PopFirstExactly(openingBracketLen);
			index += openingBracketLen;
			continue;
		}

		if(closingBracketLen!=0 && StartsWith(range, closingBracket))
		{
			counter--;
			range.PopFirstExactly(closingBracketLen);
			index += closingBracketLen;
			continue;
		}

		if(stopTokenLen!=0 && StartsWith(range, stopToken))
		{
			range.PopFirstExactly(stopTokenLen);
			index += stopTokenLen;
			break;
		}

		bool commentFound = false;
		for(auto& commentBlock: commentBlocks)
		{
			commentFound = StartsWith(range, Meta::Get<0>(commentBlock));
			if(!commentFound) continue;

			const size_t commentBlockOpeningLen = Range::Count(Meta::Get<0>(commentBlock));
			const size_t commentBlockClosingLen = Range::Count(Meta::Get<1>(commentBlock));
			range.PopFirstN(commentBlockOpeningLen);
			index += commentBlockOpeningLen;
			FindAdvance(range, Meta::Get<1>(commentBlock), &index);
			range.PopFirstN(commentBlockClosingLen);
			index += commentBlockClosingLen;
			break;
		}
		if(commentFound) continue;

		for(auto& commentBlock: recursiveCommentBlocks)
		{
			commentFound = StartsWith(range, Meta::Get<0>(commentBlock));
			if(!commentFound) continue;

			int commentCounter = 1;
			ReadRecursiveBlockAdvance(range, commentCounter, &index,
				Meta::Get<0>(commentBlock), Meta::Get<1>(commentBlock), Range::NullRange<T>(),
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




template<typename R, typename ResultRange, typename EntryStartRange, typename EntryEndRange,
	typename SubstitutionRangeOfTupleOfRanges, typename UnknownSubstitutionRange> Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsOutputRange<ResultRange>::_ &&
	Range::IsForwardRange<ResultRange>::_ &&
	Range::IsFiniteForwardRange<EntryStartRange>::_ &&
	Range::IsFiniteForwardRange<EntryEndRange>::_ &&
	Range::IsFiniteForwardRange<SubstitutionRangeOfTupleOfRanges>::_ &&
	Range::IsFiniteForwardRange<UnknownSubstitutionRange>::_,
Range::ResultOfTake<R>> StringSubstitute(const R& range, ResultRange& dstBuffer,
	EntryStartRange entryStart, EntryEndRange entryEnd,
	SubstitutionRangeOfTupleOfRanges substitutions, UnknownSubstitutionRange unknownSubstitution)
{
	R src = range;
	size_t index = 0;
	ResultRange resultBufferStart = dstBuffer;
	while(CopyToAdvance(ReadUntilAdvance(src, entryStart, &index), dstBuffer), !src.Empty())
	{
		Range::PopFirstExactly(src, Range::Count(entryStart));
		int counter = 1;
		auto entryStr = ReadRecursiveBlockAdvance(src, counter, &index, entryStart, entryEnd, null, null, null);
		if(counter>0)
		{
			INTRA_ASSERT(src.Empty());
			CopyToAdvance(entryStr, dstBuffer);
			index += Range::Count(entryStr);
			break;
		}
		PopLastExactly(entryStr, Range::Count(entryEnd));
		auto substituionsCopy = substitutions;
		FindAdvance(substituionsCopy, Meta::TupleElementEquals<0>(entryStr));
		if(substituionsCopy.Empty())
		{
			CopyToAdvance(unknownSubstitution, dstBuffer);
			index += Range::Count(unknownSubstitution);
		}
		else
		{
			CopyToAdvance(substituionsCopy.First(), dstBuffer);
			index += Range::Count(substituionsCopy.First());
		}
	}
	return Range::Take(resultBufferStart, index);
}



//! Удаляет элементы из from, пока не получится to или from не станет пустым.
//! В результате либо from.Empty(), либо from==to.
//! Диапазон должен определять операцию сравнения на равенство.
//! \returns Количество удалённых элементов.
template<typename R> Meta::EnableIf<
	Range::IsForwardRange<R>::_ && !Meta::IsConst<R>::_ && !Range::HasData<R>::_,
size_t> DistanceToAdvance(R&& from, const R& to)
{
	size_t result = 0;
	while(!from.Empty() && !(from==to))
	{
		from.PopFirst();
		result++;
	}
	return result;
}

template<typename R> Meta::EnableIf<
	Range::IsForwardRange<R>::_ && !Meta::IsConst<R>::_ && Range::HasData<R>::_,
size_t> DistanceToAdvance(R&& from, const R& to)
{
	size_t result = size_t(to.Data()-from.Data());
	from = to;
	return result;
}

//! Сколько элементов нужно удалить из from, пока не получится to или from не станет пустым.
//! Диапазон должен определять операцию сравнения на равенство.
//! \returns Количество удалённых элементов.
template<typename R> Meta::EnableIf<
	Range::IsForwardRange<R>::_,
size_t> DistanceTo(const R& from, const R& to)
{return DistanceToAdvance(R(from), to);}

INTRA_WARNING_POP

}}
