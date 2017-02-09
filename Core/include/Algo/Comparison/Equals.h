#pragma once

#include "Platform/CppWarnings.h"
#include "Platform/Intrinsics.h"
#include "Range/Concepts.h"
#include "Range/Generators/ArrayRange.h"
#include "Range/AsRange.h"
#include "Algo/Op.h"

namespace Intra { namespace Algo {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename R1, typename R2> forceinline Meta::EnableIf<
	(Range::IsFiniteRange<R1>::_ && Range::IsInfiniteRange<R2>::_) ||
		(Range::IsInfiniteRange<R1>::_ && Range::IsFiniteRange<R2>::_),
bool> Equals(R1&& r1, R2&& r2) {(void)r1; (void)r2; return false;}

namespace D {

template<typename R1, typename R2, typename P> Meta::EnableIf<
	Range::IsForwardRange<R1>::_ &&
	Range::IsForwardRange<R2>::_ &&
	!Meta::IsConst<R1>::_ && !Meta::IsConst<R2>::_,
bool> EqualsAdvance(R1&& r1, R2&& r2, P pred)
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

template<typename R1, typename R2, typename P> forceinline Meta::EnableIf<
	Range::IsNonInfiniteForwardRange<R1>::_ &&
	Range::IsNonInfiniteForwardRange<R2>::_ &&
	Meta::IsCallable<P, Range::ValueTypeOf<R1>, Range::ValueTypeOf<R2>>::_ &&
	!(Range::HasLength<R1>::_ &&
		Range::HasLength<R2>::_),
bool> Equals(R1&& r1, R2&& r2, P pred)
{return D::EqualsAdvance(R1(Meta::Forward<R1>(r1)), R2(Meta::Forward<R2>(r2)), pred);}

template<typename R1, typename R2, typename P> forceinline Meta::EnableIf<
	Range::IsForwardRange<R1>::_ &&
	Range::IsForwardRange<R2>::_ &&
	Meta::IsCallable<P, Range::ValueTypeOf<R1>, Range::ValueTypeOf<R2>>::_ &&
	Range::HasLength<R1>::_ &&
	Range::HasLength<R2>::_,
bool> Equals(R1&& r1, R2&& r2, P pred)
{
	if(r1.Length()!=r2.Length()) return false;
	return D::EqualsAdvance(R1(Meta::Forward<R1>(r1)), R2(Meta::Forward<R2>(r2)), pred);
}

template<typename R1, typename R2, typename P> forceinline Meta::EnableIf<
	Range::IsForwardRange<R1>::_ &&
	Range::IsForwardRange<R2>::_ &&
	!(Meta::IsAlmostPod<Range::ValueTypeOf<R1>>::_ &&
		Range::ValueTypeEquals<R1, R2>::_ &&
		Range::IsArrayRange<R1>::_ &&
		Range::IsArrayRange<R2>::_),
bool> Equals(R1&& r1, R2&& r2)
{
	return Equals(
		Meta::Forward<R1>(r1), Meta::Forward<R2>(r2),
		Op::Equal<Range::ValueTypeOf<R1>, Range::ValueTypeOf<R2>>);
}

template<typename R1, typename R2> inline Meta::EnableIf<
	Meta::IsAlmostPod<Range::ValueTypeOf<R1>>::_ &&
	Range::ValueTypeEquals<R1, R2>::_ &&
	Range::IsArrayRange<R1>::_ &&
	Range::IsArrayRange<R2>::_,
bool> Equals(const R1& r1, const R2& r2)
{
	if(r1.Length()!=r2.Length()) return false;
	return C::memcmp(r1.Data(), r2.Data(), r1.Length()*sizeof(Range::ValueTypeOf<R1>))==0;
}


template<typename R1, typename R2> forceinline Meta::EnableIf<
	(!Range::IsInputRange<R1>::_ && Range::IsAsForwardRange<R1>::_) ||
	(!Range::IsInputRange<R2>::_ && Range::IsAsForwardRange<R2>::_),
bool> Equals(R1&& r1, R2&& r2)
{return Equals(Range::Forward<R1>(r1), Range::Forward<R2>(r2));}

template<typename R1, typename R2, typename P> forceinline Meta::EnableIf<
	(!Range::IsInputRange<R1>::_ && Range::IsAsForwardRange<R1>::_) ||
	(!Range::IsInputRange<R2>::_ && Range::IsAsForwardRange<R2>::_),
bool> Equals(R1&& r1, R2&& r2, P pred)
{return Equals(Range::Forward<R1>(r1), Range::Forward<R2>(r2), pred);}



template<typename T1, typename T2, size_t N1, size_t N2> forceinline
bool Equals(T1(&arr1)[N1], T2(&arr2)[N2])
{return Equals(ArrayRange<T1>(arr1), ArrayRange<T2>(arr2));}

template<typename T1, typename T2, typename P, size_t N1, size_t N2> forceinline
bool Equals(T1(&arr1)[N1], T2(&arr2)[N2], P pred)
{return Equals(ArrayRange<T1>(arr1), ArrayRange<T2>(arr2), pred);}


template<typename R, typename T, size_t N> forceinline Meta::EnableIf<
	Range::IsAsFiniteForwardRange<R>::_,
bool> Equals(R&& range, T(&arr)[N])
{return Equals(Range::Forward<R>(range), ArrayRange<T>(arr));}

template<typename R, typename T, typename P, size_t N> forceinline Meta::EnableIf<
	Range::IsAsFiniteForwardRange<R>::_,
bool> Equals(R&& range, T(&arr)[N], P pred)
{return Equals(Range::Forward<R>(range), ArrayRange<T>(arr), pred);}


template<typename R, typename T, size_t N> forceinline Meta::EnableIf<
	Range::IsAsFiniteForwardRange<R>::_,
bool> Equals(T(&arr)[N], R&& range)
{return Equals(ArrayRange<T>(arr), Range::Forward<R>(range));}

template<typename R, typename T, typename P, size_t N> forceinline Meta::EnableIf<
	Range::IsAsFiniteForwardRange<R>::_,
bool> Equals(T(&arr)[N], R&& range, P pred)
{return Equals(ArrayRange<T>(arr), Range::Forward<R>(range), pred);}

INTRA_WARNING_POP

}}
