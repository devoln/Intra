#pragma once

#include "Platform/CppWarnings.h"
#include "Platform/Intrinsics.h"
#include "Range/Concepts.h"
#include "Range/Generators/ArrayRange.h"
#include "Range/AsRange.h"
#include "Algo/Op.h"

namespace Intra { namespace Algo {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

using namespace Range::Concepts;

template<typename R1, typename R2> forceinline Meta::EnableIf<
	(IsFiniteRange<R1>::_ && IsInfiniteRange<R2>::_) ||
		(IsInfiniteRange<R1>::_ && IsFiniteRange<R2>::_),
bool> Equals(R1&& r1, R2&& r2) {(void)r1; (void)r2; return false;}

namespace D {

template<typename R1, typename R2, typename P> bool EqualsAdvance(R1&& r1, R2&& r2, P pred)
{
	while(!r1.Empty() && !r2.Empty())
	{
		if(pred(r1.First(), r2.First()))
		{
			r1.PopFirst();
			r2.PopFirst();
			continue;
		}
		return false;
	}
	return r1.Empty() && r2.Empty();
}

}

template<typename R1, typename R2, typename P> Meta::EnableIf<
	!(HasLength<R1>::_ && HasLength<R2>::_) &&
	IsNonInfiniteForwardRange<R1>::_ && IsNonInfiniteInputRange<R2>::_ &&
	IsElementPredicate<P, R1, R2>::_,
bool> Equals(R1&& lhs, R2&& rhs, P pred)
{
	auto lhsCopy = Meta::Forward<R1>(lhs);
	auto rhsCopy = Meta::Forward<R2>(rhs);
	return D::EqualsAdvance(lhsCopy, rhsCopy, pred);
}

template<typename R1, typename R2, typename P> Meta::EnableIf<
	IsForwardRangeWithLength<R1>::_ && IsForwardRangeWithLength<R2>::_ &&
	IsElementPredicate<P, R1, R2>::_,
bool> Equals(R1&& lhs, R2&& rhs, P pred)
{
	if(lhs.Length()!=rhs.Length()) return false;
	auto lhsCopy = Meta::Forward<R1>(lhs);
	auto rhsCopy = Meta::Forward<R2>(rhs);
	return D::EqualsAdvance(lhsCopy, rhsCopy, pred);
}

template<typename R1, typename R2> forceinline Meta::EnableIf<
	!IsTrivCopyCompatibleArrayWith<R1, R2>::_ &&
	IsForwardRange<R1>::_ && IsForwardRange<R2>::_,
bool> Equals(R1&& lhs, R2&& rhs)
{
	return Algo::Equals(
		Meta::Forward<R1>(lhs), Meta::Forward<R2>(rhs),
		Op::Equal<Range::ValueTypeOf<R1>, Range::ValueTypeOf<R2>>);
}

template<typename R1, typename R2> inline Meta::EnableIf<
	IsTrivCopyCompatibleArrayWith<R1, R2>::_,
bool> Equals(const R1& r1, const R2& r2)
{
	if(r1.Length()!=r2.Length()) return false;
	return C::memcmp(r1.Data(), r2.Data(), r1.Length()*sizeof(*r1.Data()))==0;
}


template<typename R1, typename R2> forceinline Meta::EnableIf<
	(!IsInputRange<R1>::_ || !IsInputRange<R2>::_) &&
	IsAsForwardRange<R1>::_ && IsAsForwardRange<R2>::_,
bool> Equals(R1&& lhs, R2&& rhs)
{return Equals(Range::Forward<R1>(lhs), Range::Forward<R2>(rhs));}

template<typename R1, typename R2, typename P> forceinline Meta::EnableIf<
	(!IsInputRange<R1>::_ || !IsInputRange<R2>::_) &&
	IsAsForwardRange<R1>::_ && IsAsForwardRange<R2>::_ &&
	IsElementPredicate<P, R1, R2>::_,
bool> Equals(R1&& lhs, R2&& rhs, P pred)
{return Equals(Range::Forward<R1>(lhs), Range::Forward<R2>(rhs), pred);}

INTRA_WARNING_POP

}}
