#pragma once

#include "Range/Concepts.h"

namespace Intra { namespace Algo {

template<typename R, typename OR> Meta::EnableIf<
	Range::IsFiniteInputRange<R>::_ &&
	Range::IsOutputRange<OR>::_ && !Meta::IsConst<OR>::_
> CopyAdvanceToAdvance(R& src, OR&& dst)
{
	while(!src.Empty())
	{
		dst.Put(src.First());
		src.PopFirst();
	}
}

template<typename R, typename OR> forceinline Meta::EnableIf<
	Range::IsFiniteInputRange<R>::_ &&
	Range::IsOutputRange<OR>::_
> CopyAdvanceTo(R& src, const OR& dst)
{
	OR dst2 = dst;
	src.CopyAdvanceToAdvance(dst2);
}

template<typename R, typename OR, typename P> Meta::EnableIf<
	Range::IsFiniteInputRange<R>::_ &&
	Range::IsOutputRange<OR>::_ && Meta::IsCallable<P, typename R::value_type>::_ && !Meta::IsConst<OR>::_
> CopyAdvanceToAdvance(R& src, OR&& dst, P pred)
{
	while(!src.Empty())
	{
		auto value = src.First();
		if(pred(value)) dst.Put(value);
		src.PopFirst();
	}
}

template<typename R, typename OR, typename P> forceinline Meta::EnableIf<
	Range::IsFiniteInputRange<R>::_ &&
	Range::IsOutputRange<OR>::_ && Meta::IsCallable<P, typename R::value_type>::_
> CopyAdvanceTo(R& src, const OR& dst, P pred)
{
	auto dst2 = dst;
	src.CopyAdvanceToAdvance(dst2, pred);
}

template<typename R> Meta::EnableIf<
	Range::IsFiniteInputRange<R>::_
> FillAdvance(R& range, const typename R::value_type& value)
{
	while(!range.Empty())
	{
		range.First() = value;
		range.PopFirst();
	}
}



template<typename R, typename PatternRange> Meta::EnableIf<
	Range::IsFiniteInputRange<R>::_ && Range::IsRangeElementAssignable<R>::_ &&
	(Range::IsForwardRange<PatternRange>::_ || Range::IsInfiniteRange<PatternRange>::_)
> FillPatternAdvance(R& range, const PatternRange& pattern)
{
	auto patternCopy = pattern.Cycle();
	while(!range.Empty())
	{
		auto& ref = range.First();
		ref = patternCopy.First();
		range.PopFirst();
		patternCopy.PopFirst();
	}
}


template<typename R, typename F> Meta::EnableIf<
	Range::IsFiniteInputRange<R>::_ && Range::IsRangeElementAssignable<R>::_ &&
	Meta::IsCallable<F, typename R::value_type&>::_
> TransformAdvance(R& range, F f)
{
	while(!range.Empty())
	{
		auto& v = range.First();
		v = f(v);
		range.PopFirst();
	}
}

template<typename R, typename ResultRange, typename F> Meta::EnableIf<
	Range::IsFiniteInputRange<R>::_ &&
	Range::IsOutputRange<ResultRange>::_ && Meta::IsCallable<F, typename R::value_type&>::_
> TransformAdvanceToAdvance(R& range, ResultRange& output, F f)
{
	while(!range.Empty())
	{
		output.Put(f(range.First()));
		range.PopFirst();
	}
}

template<typename R, typename ResultRange, typename F> Meta::EnableIf<
	Range::IsFiniteInputRange<R>::_ &&
	Range::IsOutputRange<ResultRange>::_ && Meta::IsCallable<F, typename R::value_type&>::_
> TransformAdvanceTo(R& range, const ResultRange& output, F f)
{
	ResultRange outputCopy = output;
	TransformAdvanceToAdvance(range, outputCopy, f);
}



template<typename R> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_
> Fill(const R& range, const typename R::value_type& value)
{
	auto dst = range;
	FillAdvance(dst, value);
}

template<typename R, typename PatternRange> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ && Range::IsRangeElementAssignable<R>::_ &&
	(Range::IsForwardRange<PatternRange>::_ || Range::IsInfiniteRange<PatternRange>::_)
> FillPattern(const R& range, const PatternRange& pattern)
{
	auto dst = range;
	FillPatternAdvance(dst, pattern);
}

template<typename R, typename F> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ && Range::IsRangeElementAssignable<R>::_ &&
	Meta::IsCallable<F, typename R::value_type&>::_
> Transform(const R& range, F f)
{
	auto r = range;
	TransformAdvance(r, f);
}

template<typename R, typename ResultRange, typename F> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsOutputRange<ResultRange>::_ && Meta::IsCallable<F, typename R::value_type&>::_
> TransformToAdvance(const R& range, ResultRange& output, F f)
{
	auto r = me();
	TransformAdvanceToAdvance(r, output, f);
}

template<typename R, typename ResultRange, typename F> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsOutputRange<ResultRange>::_ && Meta::IsCallable<F, typename R::value_type&>::_
> TransformTo(const R& range, const ResultRange& output, F f)
{
	auto outputCopy = output;
	TransformToAdvance(range, outputCopy, f);
}


template<typename R, typename OR> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	(Range::IsOutputRange<OR>::_ ||
		Range::HasAsRange<Meta::RemoveConstRef<OR>>::_) &&
	!Meta::IsConst<OR>::_
> CopyToAdvance(const R& range, OR&& dst)
{
	auto r = range;
	CopyAdvanceToAdvance(r, core::forward<OR>(dst));
}

template<typename R, typename OR> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	(Range::IsOutputRange<OR>::_ || Range::HasAsRange<OR>::_)
> CopyTo(const R& range, OR&& dst)
{
	auto dst2 = Range::AsRange(core::forward<OR>(dst));
	CopyToAdvance(range, dst2);
}


template<typename R, typename OR, typename P> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsOutputRange<OR>::_ && !Meta::IsConst<OR>::_ &&
	Meta::IsCallable<P, typename R::value_type>::_
> CopyToAdvance(const R& range, OR&& dst, P pred)
{
	auto r = range;
	CopyAdvanceToAdvance(r, core::forward<OR>(dst), pred);
}


template<typename R, typename OR, typename P> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	(Range::IsOutputRange<OR>::_ || Range::HasAsRange<OR>::_) &&
	Meta::IsCallable<P, typename R::value_type>::_
> CopyTo(const R& range, const OR& dst, P pred)
{
	auto dst2 = AsRange(dst);
	CopyToAdvance(range, dst2, pred);
}


}}

