#pragma once

#include "Platform/CppFeatures.h"
#include "Platform/CppWarnings.h"
#include "Range/Concepts.h"
#include "Range/ArrayRange.h"

namespace Intra { namespace Algo {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename R, typename F> Meta::EnableIf<
	Range::IsAssignableInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	Range::IsFiniteRange<R>::_ &&
	Meta::IsCallable<F, Range::ValueTypeOf<R>&>::_
> TransformAdvance(R&& range, F f)
{
	while(!range.Empty())
	{
		auto& v = range.First();
		v = f(v);
		range.PopFirst();
	}
}

template<typename R1, typename R2, typename F> Meta::EnableIf<
	Range::IsAssignableInputRange<R1>::_ && !Meta::IsConst<R1>::_ && Range::IsFiniteRange<R1>::_ &&
	!Meta::IsConst<R2>::_ &&
	Meta::IsCallable<F, Range::ValueTypeOf<R1>, Range::ValueTypeOf<R2>>::_
> Transform2Advance(R1&& dstOp1, R2&& op2, F f)
{
	while(!dstOp1.Empty())
	{
		auto& v1 = dstOp1.First();
		auto v2 = op2.First();
		v1 = f(v1, v2);
		dstOp1.PopFirst();
		op2.PopFirst();
	}
}

template<typename R1, typename R2, typename F> Meta::EnableIf<
	Range::IsAssignableInputRange<R1>::_ && !Meta::IsConst<R1>::_ && Range::IsFiniteRange<R1>::_ &&
	!Meta::IsConst<R2>::_ && Range::IsFiniteRange<R2>::_ &&
	Meta::IsCallable<F, Range::ValueTypeOf<R1>, Range::ValueTypeOf<R2>>::_
> Transform2(const R1& dstOp1, const R2& op2, F f)
{Transform2Advance(R1(dstOp1), R2(op2), f);}

template<typename R, typename ResultRange, typename F> Meta::EnableIf<
	Range::IsFiniteInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	Range::IsOutputRange<ResultRange>::_ &&
	Meta::IsCallable<F, Range::ValueTypeOf<R>&>::_
> TransformAdvanceToAdvance(R&& range, ResultRange&& output, F f)
{
	while(!range.Empty())
	{
		output.Put(f(range.First()));
		range.PopFirst();
	}
}

template<typename R, typename ResultRange, typename F> Meta::EnableIf<
	Range::IsFiniteInputRange<R>::_ &&
	Range::IsOutputRange<ResultRange>::_ &&
	Meta::IsCallable<F, Range::ValueTypeOf<R>&>::_
> TransformAdvanceTo(R& range, const ResultRange& output, F f)
{
	ResultRange outputCopy = output;
	TransformAdvanceToAdvance(range, outputCopy, f);
}

template<typename R, typename F> forceinline Meta::EnableIf<
	Range::IsAssignableInputRange<R>::_ &&
	Range::IsFiniteForwardRange<R>::_ &&
	Meta::IsCallable<F, Range::ValueTypeOf<R>&>::_
> Transform(const R& range, F f)
{TransformAdvance(R(range), f);}

template<typename R, typename ResultRange, typename F> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsOutputRange<ResultRange>::_ &&
	Meta::IsCallable<F, Range::ValueTypeOf<R>&>::_
> TransformToAdvance(const R& range, ResultRange& output, F f)
{TransformAdvanceToAdvance(R(range), output, f);}

template<typename R, typename ResultRange, typename F> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsOutputRange<ResultRange>::_ &&
	Meta::IsCallable<F, Range::ValueTypeOf<R>&>::_
> TransformTo(const R& range, const ResultRange& output, F f)
{TransformToAdvance(range, ResultRange(output), f);}



template<typename R1, typename R2, typename ResultRange, typename F> Meta::EnableIf<
	Range::IsFiniteInputRange<R1>::_ && !Meta::IsConst<R1>::_ &&
	Range::IsFiniteInputRange<R2>::_ && !Meta::IsConst<R2>::_ &&
	Range::IsOutputRange<ResultRange>::_ &&
	Meta::IsCallable<F, Range::ReturnValueTypeOf<R1>, Range::ReturnValueTypeOf<R2>>::_
> Transform2AdvanceToAdvance(R1&& range1, R2&& range2, ResultRange&& output, F f)
{
	while(!range1.Empty() && !range2.Empty())
	{
		output.Put(f(range1.First(), range2.First()));
		range1.PopFirst();
		range2.PopFirst();
	}
}

template<typename R1, typename R2, typename ResultRange, typename F> Meta::EnableIf<
	Range::IsFiniteInputRange<R1>::_ &&
	Range::IsFiniteInputRange<R2>::_ &&
	Range::IsOutputRange<ResultRange>::_ &&
	Meta::IsCallable<F, Range::ReturnValueTypeOf<R1>, Range::ReturnValueTypeOf<R2>>::_
> Transform2ToAdvance(const R1& range1, const R2& range2, ResultRange&& output, F f)
{return Transform2AdvanceToAdvance(R1(range1), R2(range2), Meta::Forward<ResultRange>(output), f);}

template<typename R1, typename R2, typename ResultRange, typename F> Meta::EnableIf<
	Range::IsFiniteInputRange<R1>::_ &&
	Range::IsFiniteInputRange<R2>::_ &&
	Range::IsOutputRange<ResultRange>::_ &&
	Meta::IsCallable<F, Range::ReturnValueTypeOf<R1>, Range::ReturnValueTypeOf<R2>>::_
> Transform2To(const R1& range1, const R2& range2, const ResultRange& output, F f)
{return Transform2ToAdvance(range1, range2, ResultRange(output), f);}


//Оптимизированные частные случаи:

void Add(ArrayRange<float> dstOp1, ArrayRange<const float> op2);
void Multiply(ArrayRange<float> dstOp1, ArrayRange<const float> op2);
void Multiply(ArrayRange<float> dstOp1, float multiplyer);
void Multiply(ArrayRange<float> dst, ArrayRange<const float> op1, float multiplyer);
void MulAdd(ArrayRange<float> dstOp1, float mul, float add);

template<typename R> Meta::EnableIf<
	Range::IsInputRange<R>::_ && !Meta::IsConst<R>::_
> AddAdvance(ArrayRange<float>& dstOp1, R&& op2)
{
	while(!dstOp1.Empty())
	{
		*dstOp1.Begin++ += op2.First();
		op2.PopFirst();
	}
}

template<typename R> Meta::EnableIf<
	Range::IsInputRange<R>::_
> Add(ArrayRange<float> dstOp1, const R& op2)
{return AddAdvance(dstOp1, R(op2));}

INTRA_WARNING_POP

}}
