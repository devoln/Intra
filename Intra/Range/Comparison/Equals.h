#pragma once

#include "Cpp/Warnings.h"
#include "Cpp/Intrinsics.h"
#include "Concepts/Range.h"
#include "Utils/Span.h"
#include "Utils/ArrayAlgo.h"
#include "Concepts/RangeOf.h"

#include "Funal/Op.h"

namespace Intra { namespace Range {

using Utils::Equals;

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename R1, typename R2> forceinline Meta::EnableIf<
	(Concepts::IsFiniteRange<R1>::_ && Concepts::IsInfiniteRange<R2>::_) ||
		(Concepts::IsInfiniteRange<R1>::_ && Concepts::IsFiniteRange<R2>::_),
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
	!(Concepts::HasLength<R1>::_ && Concepts::HasLength<R2>::_) &&
	Concepts::IsNonInfiniteForwardRange<R1>::_ && Concepts::IsNonInfiniteInputRange<R2>::_ &&
	Concepts::IsElementPredicate<P, R1, R2>::_,
bool> Equals(R1&& lhs, R2&& rhs, P pred)
{
	auto lhsCopy = Cpp::Forward<R1>(lhs);
	auto rhsCopy = Cpp::Forward<R2>(rhs);
	return D::EqualsAdvance(lhsCopy, rhsCopy, pred);
}

template<typename R1, typename R2, typename P> Meta::EnableIf<
	Concepts::IsForwardRangeWithLength<R1>::_ &&
	Concepts::IsForwardRangeWithLength<R2>::_ &&
	Concepts::IsElementPredicate<P, R1, R2>::_,
bool> Equals(R1&& lhs, R2&& rhs, P pred)
{
	if(lhs.Length()!=rhs.Length()) return false;
	auto lhsCopy = Cpp::Forward<R1>(lhs);
	auto rhsCopy = Cpp::Forward<R2>(rhs);
	return D::EqualsAdvance(lhsCopy, rhsCopy, pred);
}

template<typename R1, typename R2> forceinline Meta::EnableIf<
	!Concepts::IsTrivCopyCompatibleArrayWith<R1, R2>::_ &&
	Concepts::IsForwardRange<R1>::_ &&
	Concepts::IsForwardRange<R2>::_,
bool> Equals(R1&& lhs, R2&& rhs)
{
	return Equals(Cpp::Forward<R1>(lhs), Cpp::Forward<R2>(rhs), Funal::Equal);
}


template<typename R1, typename R2> forceinline Meta::EnableIf<
	(!Concepts::IsInputRange<R1>::_ || !Concepts::IsInputRange<R2>::_) &&
	Concepts::IsAsForwardRange<R1>::_ && Concepts::IsAsForwardRange<R2>::_,
bool> Equals(R1&& lhs, R2&& rhs)
{return Equals(Range::Forward<R1>(lhs), Range::Forward<R2>(rhs));}

template<typename R1, typename R2, typename P> forceinline Meta::EnableIf<
	(!Concepts::IsInputRange<R1>::_ || !Concepts::IsInputRange<R2>::_) &&
	Concepts::IsAsForwardRange<R1>::_ && Concepts::IsAsForwardRange<R2>::_ &&
	Concepts::IsElementPredicate<P, R1, R2>::_,
bool> Equals(R1&& lhs, R2&& rhs, P pred)
{return Equals(Range::Forward<R1>(lhs), Range::Forward<R2>(rhs), pred);}

INTRA_WARNING_POP

}}
