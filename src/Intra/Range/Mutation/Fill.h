#pragma once

#include "Intra/Range/Concepts.h"
#include "Intra/Misc/RawMemory.h"
#include "Intra/Range/Cycle.h"

INTRA_BEGIN
INTRA_IGNORE_WARNING_LOSING_CONVERSION
INTRA_IGNORE_WARNING_SIGN_CONVERSION
template<typename T, typename R> constexpr Requires<
	CAssignableRange<R> &&
	!CInfiniteInputRange<R> &&
	!CConst<R> &&
	CConvertibleTo<T, TValueTypeOf<R>>
> FillAdvance(R& range, const T& value)
{
	while(!range.Empty())
	{
		range.First() = value;
		range.PopFirst();
	}
}

template<typename R, typename PR,
	typename AsPR = TRangeOfRef<PR>
> constexpr Requires<
	CAssignableRange<R> &&
	!CInfiniteRange<R> &&
	!CConst<R> &&
	(CForwardRange<AsPR> ||
		CInfiniteRange<AsPR>)
> FillPatternAdvance(R& range, PR&& pattern)
{
	auto patternCopy = Cycle(ForwardAsRange<PR>(pattern));
	while(!range.Empty())
	{
		range.First() = patternCopy.First();
		range.PopFirst();
		patternCopy.PopFirst();
	}
}

template<typename T, typename R,
	typename AsR = TRangeOfRef<R>
> constexpr Requires<
	CConsumableRange<AsR> &&
	CAssignableRange<AsR> &&
	CConvertibleTo<T, TValueTypeOf<AsR>>
> Fill(R&& range, const T& value)
{
	auto dst = ForwardAsRange<R>(range);
	FillAdvance(dst, value);
}


template<typename R, typename AsR> constexpr Requires<
	CConsumableRange<AsR> &&
	CAssignableRange<AsR> &&
	!(CArrayClass<R> && CTriviallyCopyable<TArrayElement<R>>)
> FillZeros(R&& range)
{Fill(ForwardAsRange<R>(range), TValueTypeOf<AsR>(0));}

template<typename R> constexpr Requires<
	CArrayClass<R> &&
	CTriviallyCopyable<TArrayElement<R>>
> FillZeros(R&& dst)
{Misc::BitwiseZero(DataOf(dst), LengthOf(dst));}

template<typename R, typename PR> constexpr Requires<
	CAsConsumableRange<R> &&
	CAsAccessibleRange<PR>
> FillPattern(R&& range, PR&& pattern)
{
	auto dst = ForwardAsRange<R>(range);
	FillPatternAdvance(dst, ForwardAsRange<PR>(pattern));
}
INTRA_END
