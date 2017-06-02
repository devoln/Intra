#pragma once

#include "Cpp/Warnings.h"
#include "Cpp/Features.h"
#include "Meta/Type.h"
#include "Concepts/Range.h"
#include "Concepts/RangeOf.h"
#include "Concepts/Container.h"
#include "Range/Comparison/Equals.h"

namespace Intra { namespace Container {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename L, typename R> forceinline Meta::EnableIf<
	(Concepts::IsSequentialContainer<L>::_ &&
		Concepts::IsAsConsumableRange<R>::_) ||
	(Concepts::IsSequentialContainer<R>::_ &&
		Concepts::IsAsConsumableRange<L>::_),
bool> operator==(L&& lhs, R&& rhs)
{
	using Concepts::RangeOf;
	return Range::Equals(RangeOf(lhs), RangeOf(rhs));
}

template<typename L, typename R> forceinline Meta::EnableIf<
	(Concepts::IsSequentialContainer<L>::_ &&
		Concepts::IsAsConsumableRange<R>::_) ||
	(Concepts::IsSequentialContainer<R>::_ &&
		Concepts::IsAsConsumableRange<L>::_),
bool> operator!=(L&& lhs, R&& rhs)
{return !operator==(Cpp::Forward<L>(lhs), Cpp::Forward<R>(rhs));}

template<typename L, typename T> forceinline Meta::EnableIf<
	Concepts::IsSequentialContainer<L>::_,
bool> operator==(L&& lhs, InitializerList<T> rhs)
{
	using Concepts::RangeOf;
	return Range::Equals(RangeOf(lhs), SpanOf(rhs));
}

template<typename L, typename T> forceinline Meta::EnableIf<
	Concepts::IsSequentialContainer<L>::_,
bool> operator!=(L&& lhs, InitializerList<T> rhs)
{return !operator==(Cpp::Forward<L>(lhs), rhs);}

INTRA_WARNING_POP

}}
