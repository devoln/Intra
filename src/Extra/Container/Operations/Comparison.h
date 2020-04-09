#pragma once

#include "Intra/Container/Concepts.h"
#include "Intra/Range/Concepts.h"
#include "Intra/Range/Operations.h"
#include "Intra/Type.h"

INTRA_BEGIN
template<typename L, typename R, typename = Requires<
	(CSequentialContainer<L> && CAsConsumableRange<R>) ||
	(CSequentialContainer<R> && CAsConsumableRange<L>)>>
[[nodiscard]] constexpr bool operator==(L&& lhs, R&& rhs)
{
	return Forward<L>(lhs)|Equals(Forward<R>(rhs));
}

template<typename L, typename R, typename = Requires<
	(CSequentialContainer<L> && CAsConsumableRange<R>) ||
	(CSequentialContainer<R> && CAsConsumableRange<L>)>>
[[nodiscard]] constexpr bool operator!=(L&& lhs, R&& rhs)
{
	return !operator==(Forward<L>(lhs), Forward<R>(rhs));
}
INTRA_END
