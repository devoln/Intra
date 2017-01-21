#pragma once

#include "Range/ForwardDecls.h"
#include "Range/Iteration/Cycle.h"
#include "Platform/CppWarnings.h"

namespace Intra { namespace Algo {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename T, typename R> Meta::EnableIf<
	Range::IsFiniteInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	Meta::IsConvertible<T, Range::ValueTypeOf<R>>::_
> FillAdvance(R&& range, const T& value)
{
	while(!range.Empty())
	{
		range.First() = value;
		range.PopFirst();
	}
}



template<typename R, typename PatternRange> Meta::EnableIf<
	Range::IsAssignableInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	Range::IsFiniteRange<R>::_ &&
	(Range::IsForwardRange<PatternRange>::_ ||
		Range::IsInfiniteRange<PatternRange>::_)
> FillPatternAdvance(R&& range, const PatternRange& pattern)
{
	auto patternCopy = Range::Cycle(pattern);
	while(!range.Empty())
	{
		auto& ref = range.First();
		ref = patternCopy.First();
		range.PopFirst();
		patternCopy.PopFirst();
	}
}

template<typename T, typename R> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Meta::IsConvertible<T, Range::ValueTypeOf<R>>::_
> Fill(const R& range, const T& value)
{
	auto dst = range;
	FillAdvance(dst, value);
}

template<typename T2, typename T, size_t N> forceinline Meta::EnableIf<
	!Meta::IsConst<T>::_ &&
	Meta::IsConvertible<T2, T>::_
> Fill(T(&arr)[N], const T2& value)
{FillAdvance(ArrayRange<T>(arr), value);}


template<typename R> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	!(Range::IsArrayRange<R>::_ && Meta::IsAlmostPod<Range::ValueTypeOf<R>>::_)
> FillZeros(const R& range)
{Fill(range, Range::ValueTypeOf<R>(0));}

template<typename R> forceinline Meta::EnableIf<
	Range::IsArrayRange<R>::_ && Meta::IsAlmostPod<Range::ValueTypeOf<R>>::_
> FillZeros(const R& range)
{C::memset(range.Data(), 0, range.Length()*sizeof(range.First()));}

template<typename T, size_t N> forceinline Meta::EnableIf<
	!Meta::IsConst<T>::_
> FillZeros(T(&arr)[N])
{FillZeros(ArrayRange<T>(arr));}


template<typename R, typename PatternRange> forceinline Meta::EnableIf<
	Range::IsAssignableInputRange<R>::_ &&
	Range::IsFiniteRange<R>::_ &&
	(Range::IsForwardRange<PatternRange>::_ ||
		Range::IsInfiniteRange<PatternRange>::_)
> FillPattern(const R& range, const PatternRange& pattern)
{FillPatternAdvance(R(range), pattern);}

template<typename R, typename T, size_t N> forceinline Meta::EnableIf<
	Range::IsAssignableInputRange<R>::_ &&
	Range::IsFiniteRange<R>::_
> FillPattern(const R& range, const T(&patternArr)[N])
{FillPattern(range, AsRange(patternArr));}

template<typename T, size_t N, typename PatternRange> forceinline Meta::EnableIf<
	Range::IsForwardRange<PatternRange>::_ ||
	Range::IsInfiniteRange<PatternRange>::_
> FillPattern(T(&arr)[N], const PatternRange& pattern)
{FillPatternAdvance(ArrayRange<T>(arr), pattern);}

template<typename T, size_t N, typename TP, size_t NP> forceinline
void FillPattern(T(&arr)[N], TP(&patternArr)[NP])
{FillPatternAdvance(ArrayRange<T>(arr), AsRange(patternArr));}

INTRA_WARNING_POP

}}

