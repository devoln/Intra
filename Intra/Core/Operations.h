#pragma once

#include "Core/Type.h"

INTRA_BEGIN
template<typename T1, typename T2> constexpr forceinline Requires<
	CAssignable<T1&, const T2&>
> CopyAssign(T1& dst, const T2& src)
{
	dst = src;
}

template<typename T1, typename T2> forceinline Requires<
	!CAssignable<T1&, const T2> &&
	CConstructible<T1, const T2&> &&
	CDestructible<T1>
> CopyAssign(T1& dst, const T2& src)
{
	dst.~T1();
	new(Construct, &dst) T1(src);
}

template<typename T1, typename T2> constexpr forceinline Requires<
	CAssignable<T1&, T2&&>
> MoveAssign(T1& dst, T2&& src) noexcept
{
	dst = Move(src);
}

template<typename T1, typename T2> forceinline Requires<
	!CAssignable<T1&, const T2&> &&
	CConstructible<T1, const T2&> &&
	CDestructible<T1>
> MoveAssign(T1& dst, T2&& src) noexcept
{
	dst.~T1();
	new(Construct, &dst) T1(Move(src));
}

INTRA_DEFINE_CONCEPT_REQUIRES2(CHasOpNotEquals, Val<T1>() != Val<T2>(),, = U1);

//! Automatically define operator!= for all types that don't have it but have operator==
template<typename T1, typename T2> constexpr forceinline Requires<
	CEqualityComparable<T1, T2> &&
	!CHasOpNotEquals<T1, T2> &&
	(!CScalar<T1> || !CScalar<T2>),
bool> operator!=(const T1& lhs, const T2& rhs) noexcept
{return !(lhs == rhs);}
INTRA_END
