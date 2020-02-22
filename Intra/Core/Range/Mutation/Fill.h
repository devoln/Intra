#pragma once

#include "Core/Range/Concepts.h"
#include "Core/Misc/RawMemory.h"
#include "Core/Range/Cycle.h"

INTRA_BEGIN
INTRA_WARNING_DISABLE_LOSING_CONVERSION
INTRA_WARNING_DISABLE_SIGN_CONVERSION
template<typename T, typename R> constexpr Requires<
	CAssignableRange<R> &&
	!CInfiniteRange<R> &&
	!CConst<R> &&
	CConvertible<T, TValueTypeOf<R>>
> FillAdvance(R& range, const T& value)
{
	while(!range.Empty())
	{
		range.First() = value;
		range.PopFirst();
	}
}

template<typename R, typename PR,
	typename AsPR = TRangeOfType<PR>
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
	typename AsR = TRangeOfType<R>
> constexpr forceinline Requires<
	CConsumableRange<AsR> &&
	CAssignableRange<AsR> &&
	CConvertible<T, TValueTypeOf<AsR>>
> Fill(R&& range, const T& value)
{
	auto dst = ForwardAsRange<R>(range);
	FillAdvance(dst, value);
}


template<typename R, typename AsR> constexpr forceinline Requires<
	CConsumableRange<AsR> &&
	CAssignableRange<AsR> &&
	!(CArrayClass<R> && CPod<TArrayElement<R>>)
> FillZeros(R&& range)
{Fill(ForwardAsRange<R>(range), TValueTypeOf<AsR>(0));}

template<typename R> constexpr forceinline Requires<
	CArrayClass<R> &&
	CPod<TArrayElement<R>>
> FillZeros(R&& dst)
{Misc::BitwiseZero(DataOf(dst), LengthOf(dst));}

template<typename R, typename PR> constexpr forceinline Requires<
	CAsConsumableRange<R> &&
	CAsAccessibleRange<PR>
> FillPattern(R&& range, PR&& pattern)
{
	auto dst = ForwardAsRange<R>(range);
	FillPatternAdvance(dst, ForwardAsRange<PR>(pattern));
}
INTRA_END
