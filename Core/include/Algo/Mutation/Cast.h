#pragma once

#include "Range/Concepts.h"
#include "Range/AsRange.h"
#include "Range/Generators/ArrayRange.h"
#include "Copy.h"
#include "Platform/CppWarnings.h"

namespace Intra { namespace Algo {

using namespace Range::Concepts;

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename R, typename OR> Meta::EnableIf<
	IsNonInfiniteInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	IsOutputRange<OR>::_ &&
	!ValueTypeEquals<R, OR>::_
> CastAdvanceToAdvance(R& src, OR& dst)
{
	while(!src.Empty())
	{
		dst.Put(ValueTypeOf<OR>(src.First()));
		src.PopFirst();
	}
}

//Оптимизированная перегрузка
void CastAdvanceToAdvance(ArrayRange<const float>& src, ArrayRange<short>& dst);
forceinline void CastAdvanceToAdvance(ArrayRange<const float>&& src, ArrayRange<short>& dst)
{CastAdvanceToAdvance(src, dst);}
forceinline void CastAdvanceToAdvance(ArrayRange<const float>& src, ArrayRange<short>&& dst)
{CastAdvanceToAdvance(src, dst);}
forceinline void CastAdvanceToAdvance(ArrayRange<const float>&& src, ArrayRange<short>&& dst)
{CastAdvanceToAdvance(src, dst);}

template<typename R, typename OR> Meta::EnableIf<
	IsNonInfiniteInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	IsOutputRange<OR>::_ &&
	ValueTypeEquals<R, OR>::_
> CastAdvanceToAdvance(R& src, OR& dst)
{Algo::CopyAdvanceToAdvance(src, dst);}

template<typename R, typename OR> Meta::EnableIf<
	IsNonInfiniteInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	IsAsOutputRange<OR>::_
> CastAdvanceTo(R& src, OR&& dst)
{
	auto dstRange = Range::Forward<OR>(dst);
	Algo::CastAdvanceToAdvance(src, dstRange);
}

template<typename R, typename OR> Meta::EnableIf<
	IsAsNonInfiniteInputRange<R>::_ &&
	IsOutputRange<OR>::_
> CastToAdvance(R&& src, OR& dst)
{
	auto srcCopy = Range::Forward<R>(src);
	CastAdvanceToAdvance(srcCopy, dst);
}

template<typename R, typename OR> Meta::EnableIf<
	IsAsNonInfiniteInputRange<R>::_ &&
	IsAsOutputRange<OR>::_
> CastTo(R&& src, OR&& dst)
{
	auto srcCopy = Range::Forward<R>(src);
	auto dstCopy = Range::Forward<OR>(dst);
	CastAdvanceToAdvance(srcCopy, dstCopy);
}



//! Нормировать массив делением каждого элемента на NumericLimits<From>::Max
template<typename To, typename From> Meta::EnableIf<
	Meta::IsFloatType<To>::_ && Meta::IsIntegralType<From>::_
> CastToNormalized(ArrayRange<To> dst, ArrayRange<const From> src)
{
	INTRA_ASSERT(dst.Length()==src.Length());
	while(!dst.Empty())
	{
		dst.First() = To(src.First()) / To(Meta::NumericLimits<From>::Max());
		dst.PopFirst();
		src.PopFirst();
	}
}

//! Заполнить массив dst элементами из src, умноженными на NumericLimits<To>::Max.
//! Предполагается, что -1.0 <= src[i] <= 1.0. Иначе произойдёт переполнение.
template<typename To, typename From> Meta::EnableIf<
	Meta::IsFloatType<From>::_ && Meta::IsIntegralType<To>::_
> CastFromNormalized(ArrayRange<To> dst, ArrayRange<const From> src)
{
	INTRA_ASSERT(dst.Length()==src.Length());
	while(!dst.Empty())
	{
		dst.First() = To(src.First() * Meta::NumericLimits<From>::Max());
		dst.PopFirst();
		src.PopFirst();
	}
}

INTRA_WARNING_POP

}}
