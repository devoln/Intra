#pragma once

#include "Core/Type.h"
#include "Core/Range/Concepts.h"
#include "Core/CContainer.h"
#include "Core/Range/Comparison.h"

INTRA_BEGIN
template<typename L, typename R> forceinline Requires<
	(CSequentialContainer<L> &&
		CAsConsumableRange<R>) ||
	(CSequentialContainer<R> &&
		CAsConsumableRange<L>),
bool> operator==(L&& lhs, R&& rhs)
{
	return Equals(RangeOf(lhs), RangeOf(rhs));
}

template<typename L, typename R> forceinline Requires<
	(CSequentialContainer<L> &&
		CAsConsumableRange<R>) ||
	(CSequentialContainer<R> &&
		CAsConsumableRange<L>),
bool> operator!=(L&& lhs, R&& rhs)
{return !operator==(Forward<L>(lhs), Forward<R>(rhs));}

template<typename L, typename T> forceinline Requires<
	CSequentialContainer<L>,
bool> operator==(L&& lhs, InitializerList<T> rhs)
{
	return Equals(RangeOf(lhs), SpanOf(rhs));
}

template<typename L, typename T> forceinline Requires<
	CSequentialContainer<L>,
bool> operator!=(L&& lhs, InitializerList<T> rhs)
{return !operator==(Forward<L>(lhs), rhs);}
INTRA_END
