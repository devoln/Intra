#pragma once

#include "Intra/Type.h"
#include "Intra/Container/Concepts.h"
#include "Intra/Range/Concepts.h"
#include "Intra/Range/Inserter.h"
#include "Intra/Range/Mutation/Copy.h"
#include "Intra/Range/Operations.h"

INTRA_BEGIN
template<typename L, typename R> requires
	(CSequentialContainer<L> && CConsumableList<R>) ||
	(CSequentialContainer<R> && CConsumableList<L>)
[[nodiscard]] constexpr bool operator==(L&& lhs, R&& rhs)
{
	return INTRA_FWD(lhs)|Equals(INTRA_FWD(rhs));
}

template<typename L, typename R> requires
	(CSequentialContainer<L> && CConsumableList<R>) ||
	(CSequentialContainer<R> && CConsumableList<L>)
[[nodiscard]] constexpr bool operator!=(L&& lhs, R&& rhs)
{
	return !operator==(INTRA_FWD(lhs), INTRA_FWD(rhs));
}

template<typename SC, typename R> requires
	(CHasSetCountUninitialized<SC> || CHas_resize<SC> || CHas_push_back<SC>) &&
	CConsumableListOf<R, TRangeValue<SC>>
SC& operator+=(SC& lhs, R&& rhs)
{
	if constexpr(!CHasSetCountUninitialized<SC> && !CHas_resize<SC>)
	{
		LastAppender{lhs} << rhs;
	}
	else if constexpr(CSameArrays<SC, R> && CTriviallyCopyable<TArrayElement<SC>>)
	{
		const auto oldLen = LengthOf(lhs);
		SetCountTryNotInit(lhs, oldLen + LengthOf(rhs));
		BitwiseCopy(Unsafe, DataOf(lhs) + oldLen, DataOf(rhs), LengthOf(rhs));
	}
	else
	{
		auto r = ForwardAsRange<R>(rhs);
		const auto oldLen = LengthOf(lhs);
		SetCountTryNotInit(lhs, oldLen + Count(r));
		r|CopyTo(lhs|Drop(oldLen));
	}
	return lhs;
}
INTRA_END
