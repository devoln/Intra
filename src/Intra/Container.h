#pragma once

#include <Intra/Numeric/Integral.h>
#include <Intra/Range/Inserter.h>

namespace Intra { INTRA_BEGIN
constexpr auto AddLast = [](auto&& v)
{
	return [v = INTRA_FWD(v)]<CList C>(C& container) requires z_D::CHasMethod_push_back<C> {
		container.push_back(v);
	};
};

constexpr auto AddLastRef = [](auto&& v)
{
	return [&]<CList C>(C& container) requires z_D::CHasMethod_push_back<C> {
		container.push_back(INTRA_FWD(v));
	};
};

constexpr auto SetLength = [](Size newLength)
{
	return [newLength]<class C>(C& container) requires z_D::CHasMethod_resize<C> {
		container.resize(size_t(newLength));
	};
};

constexpr auto Reserve = [](Size capacity)
{
	return [capacity]<class C>(C& container) requires z_D::CHasMethod_reserve<C> {
		container.reserve(size_t(capacity));
	};
};

template<typename L, typename R> requires
	(CGrowingList<L> && CConsumableList<R>) ||
	(CGrowingList<R> && CConsumableList<L>)
[[nodiscard]] constexpr bool operator==(L&& lhs, R&& rhs)
{
	return INTRA_FWD(lhs)|Equals(INTRA_FWD(rhs));
}

template<typename L, typename R> requires
	(CGrowingList<L> && CConsumableList<R>) ||
	(CGrowingList<R> && CConsumableList<L>)
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
	else if constexpr(CSameArrays<SC, R> && CTriviallyCopyable<TArrayListValue<SC>>)
	{
		const auto oldLen = LengthOf(lhs);
		SetCountTryNotInit(lhs, oldLen + LengthOf(rhs));
		MemoryCopy(Unsafe, DataOf(lhs) + oldLen, DataOf(rhs), LengthOf(rhs));
	}
	else
	{
		auto r = RangeOf(INTRA_FWD(rhs));
		const auto oldLen = LengthOf(lhs);
		SetCountTryNotInit(lhs, oldLen + Count(r));
		r|CopyTo(lhs|Drop(oldLen));
	}
	return lhs;
}
} INTRA_END
