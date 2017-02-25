#pragma once

#include "Platform/CppWarnings.h"
#include "Platform/CppFeatures.h"
#include "Meta/Type.h"
#include "Range/Concepts.h"
#include "Container/Concepts.h"
#include "Algo/Comparison/Equals.h"

namespace Intra { namespace Container {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename L, typename R> forceinline Meta::EnableIf<
	(IsSequentialContainer<L>::_ && Range::IsAsConsumableRange<R>::_) ||
	(IsSequentialContainer<R>::_ && Range::IsAsConsumableRange<L>::_),
bool> operator==(L&& lhs, R&& rhs)
{return Algo::Equals(Range::AsRange(lhs), Range::AsRange(rhs));}

template<typename L, typename R> forceinline Meta::EnableIf<
	(IsSequentialContainer<L>::_ && Range::IsAsConsumableRange<R>::_) ||
	(IsSequentialContainer<R>::_ && Range::IsAsConsumableRange<L>::_),
bool> operator!=(L&& lhs, R&& rhs)
{return !Algo::Equals(Range::AsRange(lhs), Range::AsRange(rhs));}

template<typename L, typename T> forceinline Meta::EnableIf<
	IsSequentialContainer<L>::_,
bool> operator==(L&& lhs, InitializerList<T> rhs)
{return Algo::Equals(Range::AsRange(lhs), Range::AsRange(rhs));}

template<typename L, typename T> forceinline Meta::EnableIf<
	IsSequentialContainer<L>::_,
bool> operator!=(L&& lhs, InitializerList<T> rhs)
{return !Algo::Equals(Range::AsRange(lhs), Range::AsRange(rhs));}

INTRA_WARNING_POP

}}
