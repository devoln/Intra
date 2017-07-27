#pragma once

#include "Cpp/Features.h"
#include "Cpp/Warnings.h"
#include "Concepts/Range.h"
#include "Utils/Span.h"
#include "Concepts/RangeOf.h"

namespace Intra { namespace Range {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename R, typename F> Meta::EnableIf<
	Concepts::IsAssignableRange<R>::_ &&
	!Meta::IsConst<R>::_ &&
	!Concepts::IsInfiniteRange<R>::_ &&
	Meta::IsCallable<F, Concepts::ValueTypeOf<R>&>::_
> TransformAdvance(R& range, F f)
{
	while(!range.Empty())
	{
		auto& v = range.First();
		v = f(v);
		range.PopFirst();
	}
}

template<typename R1, typename R2, typename F> Meta::EnableIf<
	Concepts::IsAssignableRange<R1>::_ &&
	!Meta::IsConst<R1>::_ &&
	!Concepts::IsInfiniteRange<R1>::_ &&
	Concepts::IsInputRange<R2>::_ &&
	!Meta::IsConst<R2>::_ &&
	Meta::IsCallable<F, Concepts::ReturnValueTypeOf<R1>, Concepts::ReturnValueTypeOf<R2>>::_
> Transform2Advance(R1& dstOp1, R2& op2, F f)
{
	while(!dstOp1.Empty())
	{
		auto&& v1 = dstOp1.First();
		v1 = f(v1, op2.First());
		dstOp1.PopFirst();
		op2.PopFirst();
	}
}

template<typename R1, typename R2, typename F> Meta::EnableIf<
	Concepts::IsAsAssignableRange<R1>::_ &&
	!Concepts::IsAsInfiniteRange<R1>::_ &&
	Concepts::IsAsInputRange<R2>::_ &&
	!Concepts::IsAsInfiniteRange<R2>::_ &&
	Meta::IsCallable<F, Concepts::ReturnValueTypeOfAs<R1>, Concepts::ReturnValueTypeOfAs<R2>>::_
> Transform2(R1&& dstOp1, R2&& op2, F f)
{
	auto dstOp1Copy = Range::Forward<R1>(dstOp1);
	auto op2Copy = Range::Forward<R2>(op2);
	Transform2Advance(dstOp1Copy, op2Copy, f);
}

template<typename R, typename OR, typename F> Meta::EnableIf<
	Concepts::IsNonInfiniteInputRange<R>::_ &&
	!Meta::IsConst<R>::_ &&
	Concepts::IsOutputRange<OR>::_ &&
	Meta::IsCallable<F, Concepts::ReturnValueTypeOf<R>>::_
> TransformAdvanceToAdvance(R& range, OR& output, F f)
{
	while(!range.Empty())
	{
		output.Put(f(range.First()));
		range.PopFirst();
	}
}

template<typename R, typename OR, typename F> Meta::EnableIf<
	Concepts::IsNonInfiniteInputRange<R>::_ &&
	!Meta::IsConst<R>::_ &&
	Concepts::IsAsOutputRange<OR>::_ &&
	Meta::IsCallable<F, Concepts::ValueTypeOf<R>&>::_
> TransformAdvanceTo(R& range, OR&& output, F f)
{
	auto outputCopy = Range::Forward<OR>(output);
	TransformAdvanceToAdvance(range, outputCopy, f);
}

template<typename R, typename F> forceinline Meta::EnableIf<
	Concepts::IsAsAssignableRange<R>::_ &&
	Concepts::IsAsNonInfiniteForwardRange<R>::_ &&
	Meta::IsCallable<F, Concepts::ValueTypeOfAs<R>&>::_
> Transform(R&& range, F f)
{
	auto rangeCopy = Range::Forward<R>(range);
	TransformAdvance(rangeCopy, f);
}

template<typename R, typename OR, typename F> forceinline Meta::EnableIf<
	Concepts::IsAsNonInfiniteForwardRange<R>::_ &&
	Concepts::IsOutputRange<OR>::_ &&
	Meta::IsCallable<F, Concepts::ValueTypeOfAs<R>&>::_
> TransformToAdvance(R&& range, OR& output, F f)
{
	auto rangeCopy = Range::Forward<R>(range);
	TransformAdvanceToAdvance(rangeCopy, output, f);
}

template<typename R, typename OR, typename F> forceinline Meta::EnableIf<
	Concepts::IsAsNonInfiniteForwardRange<R>::_ &&
	Concepts::IsAsOutputRange<OR>::_ &&
	Meta::IsCallable<F, Concepts::ValueTypeOfAs<R>&>::_
> TransformTo(R&& range, OR&& output, F f)
{
	auto outputCopy = Range::Forward<OR>(output);
	TransformToAdvance(Range::Forward<R>(range), outputCopy, f);
}



template<typename R1, typename R2, typename OR, typename F> Meta::EnableIf<
	Concepts::IsNonInfiniteInputRange<R1>::_ &&
	!Meta::IsConst<R1>::_ &&
	Concepts::IsNonInfiniteInputRange<R2>::_ &&
	!Meta::IsConst<R2>::_ &&
	Concepts::IsOutputRange<OR>::_ &&
	Meta::IsCallable<F, Concepts::ReturnValueTypeOf<R1>, Concepts::ReturnValueTypeOf<R2>>::_
> Transform2AdvanceToAdvance(R1& range1, R2& range2, OR& output, F f)
{
	while(!range1.Empty() && !range2.Empty())
	{
		output.Put(f(range1.First(), range2.First()));
		range1.PopFirst();
		range2.PopFirst();
	}
}

template<typename R1, typename R2, typename OR, typename F> Meta::EnableIf<
	Concepts::IsAsNonInfiniteInputRange<R1>::_ &&
	Concepts::IsAsNonInfiniteInputRange<R2>::_ &&
	Concepts::IsOutputRange<OR>::_ &&
	Meta::IsCallable<F, Concepts::ReturnValueTypeOfAs<R1>, Concepts::ReturnValueTypeOfAs<R2>>::_
> Transform2ToAdvance(R1&& range1, R2&& range2, OR& output, F f)
{
	auto range1Copy = Range::Forward<R1>(range1);
	auto range2Copy = Range::Forward<R2>(range2);
	return Transform2AdvanceToAdvance(range1Copy, range2Copy, output, f);
}

template<typename R1, typename R2, typename OR, typename F> Meta::EnableIf<
	Concepts::IsAsNonInfiniteInputRange<R1>::_ &&
	Concepts::IsAsNonInfiniteInputRange<R2>::_ &&
	Concepts::IsAsOutputRange<OR>::_ &&
	Meta::IsCallable<F, Concepts::ReturnValueTypeOfAs<R1>, Concepts::ReturnValueTypeOfAs<R2>>::_
> Transform2To(R1&& range1, R2&& range2, OR&& output, F f)
{
	auto outputCopy = Range::Forward<OR>(output);
	return Transform2ToAdvance(Range::Forward<R1>(range1), Range::Forward<R2>(range2), outputCopy, f);
}


//Оптимизированные частные случаи:

//! Складывать соотвествующие элементы диапазонов до тех пор, пока один из диапазонов не окажется пустым.
//! @return Количество обработанных элементов, то есть наименьшее количество элементов двух аргументов.
size_t AddAdvance(Span<float>& dstOp1, CSpan<float>& op2);

inline size_t AddAdvance(Span<float>& dstOp1, Span<float>& op2)
{
	CSpan<float> op2Const = op2;
	return op2.PopFirstN(AddAdvance(dstOp1, op2Const));
}

size_t MultiplyAdvance(Span<float>& dstOp1, CSpan<float>& op2);
size_t MultiplyAdvance(Span<float>& dstOp1, float multiplyer);
size_t MultiplyAdvance(Span<float>& dst, CSpan<float>& op1, float multiplyer);
size_t MulAddAdvance(Span<float>& dstOp1, float mul, float add);
size_t AddMultipliedAdvance(Span<float>& dstOp1, CSpan<float>& op2, float op2Multiplyer);

//! Складывать соответствующие элементы диапазонов.
//! Если один из диапазонов короче другого, будет обработано столько элементов, какова минимальная длина.
//! @return Количество обработанных элементов, то есть наименьшее количество элементов двух аргументов.
forceinline size_t Add(Span<float> dstOp1, CSpan<float> op2) {return AddAdvance(dstOp1, op2);}

forceinline size_t Add(Span<float> dstOp1, Span<float> op2) {return AddAdvance(dstOp1, op2);}

forceinline size_t Multiply(Span<float> dstOp1, CSpan<float> op2) {return MultiplyAdvance(dstOp1, op2);}
forceinline size_t Multiply(Span<float> dstOp1, float multiplier) {return MultiplyAdvance(dstOp1, multiplier);}
forceinline size_t Multiply(Span<float> dst, CSpan<float> op1, float multiplier) {return MultiplyAdvance(dst, op1, multiplier);}
forceinline size_t MulAdd(Span<float> dstOp1, float mul, float add) {return MulAddAdvance(dstOp1, mul, add);}
forceinline size_t AddMultiplied(Span<float> dst, CSpan<float> op1, float multiplier) {return AddMultipliedAdvance(dst, op1, multiplier);}

template<typename R> Meta::EnableIf<
	Concepts::IsInputRange<R>::_ &&
	!Meta::IsConst<R>::_
> AddAdvance(Span<float>& dstOp1, R&& op2)
{
	while(!dstOp1.Empty())
	{
		*dstOp1.Begin++ += op2.First();
		op2.PopFirst();
	}
}

template<typename R> Meta::EnableIf<
	Concepts::IsInputRange<R>::_
> Add(Span<float> dstOp1, const R& op2)
{return AddAdvance(dstOp1, R(op2));}

void LinearMultiply(Span<float> dst, float& u, float du);
void LinearMultiply(Span<float> dst, CSpan<float> src, float& u, float du);
void LinearMultiplyAdd(Span<float> dst, CSpan<float> src, float& u, float du);

INTRA_WARNING_POP

}}
