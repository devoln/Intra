#pragma once

#include "Concepts/Range.h"
#include "Concepts/RangeOf.h"
#include "Utils/Span.h"
#include "Copy.h"
#include "Cpp/Warnings.h"

namespace Intra { namespace Range {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename R, typename OR> Meta::EnableIf<
	Concepts::IsNonInfiniteInputRange<R>::_ &&
	!Meta::IsConst<R>::_ &&
	Concepts::IsOutputRange<OR>::_ &&
	!Concepts::ValueTypeEquals<R, OR>::_
> CastAdvanceToAdvance(R& src, OR& dst)
{
	while(!src.Empty())
	{
		dst.Put(Concepts::ValueTypeOf<OR>(src.First()));
		src.PopFirst();
	}
}

//Оптимизированная перегрузка
void CastAdvanceToAdvance(CSpan<float>& src, Span<short>& dst);
forceinline void CastAdvanceToAdvance(CSpan<float>&& src, Span<short>& dst)
{CastAdvanceToAdvance(src, dst);}
forceinline void CastAdvanceToAdvance(CSpan<float>& src, Span<short>&& dst)
{CastAdvanceToAdvance(src, dst);}
forceinline void CastAdvanceToAdvance(CSpan<float>&& src, Span<short>&& dst)
{CastAdvanceToAdvance(src, dst);}

template<typename R, typename OR> Meta::EnableIf<
	Concepts::IsNonInfiniteInputRange<R>::_ &&
	!Meta::IsConst<R>::_ &&
	Concepts::IsOutputRange<OR>::_ &&
	Concepts::ValueTypeEquals<R, OR>::_
> CastAdvanceToAdvance(R& src, OR& dst)
{ReadWrite(src, dst);}

template<typename R, typename OR> Meta::EnableIf<
	Concepts::IsNonInfiniteInputRange<R>::_ &&
	!Meta::IsConst<R>::_ &&
	Concepts::IsAsOutputRange<OR>::_
> CastAdvanceTo(R& src, OR&& dst)
{
	auto dstRange = Range::Forward<OR>(dst);
	CastAdvanceToAdvance(src, dstRange);
}

template<typename R, typename OR> Meta::EnableIf<
	Concepts::IsAsNonInfiniteInputRange<R>::_ &&
	Concepts::IsOutputRange<OR>::_
> CastToAdvance(R&& src, OR& dst)
{
	auto srcCopy = Range::Forward<R>(src);
	CastAdvanceToAdvance(srcCopy, dst);
}

template<typename R, typename OR> Meta::EnableIf<
	Concepts::IsAsNonInfiniteInputRange<R>::_ &&
	Concepts::IsAsOutputRange<OR>::_
> CastTo(R&& src, OR&& dst)
{
	auto srcCopy = Range::Forward<R>(src);
	auto dstCopy = Range::Forward<OR>(dst);
	CastAdvanceToAdvance(srcCopy, dstCopy);
}



//! Нормировать массив делением каждого элемента на NumericLimits<From>::Max
template<typename To, typename From> Meta::EnableIf<
	Meta::IsFloatType<To>::_ &&
	Meta::IsIntegralType<From>::_
> CastToNormalized(Span<To> dst, CSpan<From> src)
{
	INTRA_DEBUG_ASSERT(dst.Length() == src.Length());
	while(dst.Begin < dst.End)
		*dst.Begin++ = To(*src.Begin++) / To(Meta::NumericLimits<From>::Max());
}

//! Заполнить массив dst элементами из src, умноженными на NumericLimits<To>::Max.
//! Предполагается, что -1.0 <= src[i] <= 1.0. Иначе произойдёт переполнение.
template<typename To, typename From> Meta::EnableIf<
	Meta::IsFloatType<From>::_ &&
	Meta::IsSignedIntegralType<To>::_
> CastFromNormalized(Span<To> dst, CSpan<From> src)
{
	INTRA_DEBUG_ASSERT(dst.Length() == src.Length());
	while(dst.Begin < dst.End)
		*dst.Begin++ = To(*src.Begin++ * Meta::NumericLimits<From>::Max());
}

INTRA_WARNING_POP

}}
