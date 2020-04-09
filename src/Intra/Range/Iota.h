#pragma once

#include "Intra/Assert.h"
#include "Intra/Range/Concepts.h"
#include "Intra/Range/Take.h"

INTRA_BEGIN
template<typename T, typename S = T> struct IotaInf
{
	static constexpr bool IsAnyInstanceInfinite = true;

	T Begin{0};
	S Step{1};

	explicit IotaInf() = default;
	explicit constexpr IotaInf(T begin, S step = 1): Begin(begin), Step(step) {}

	[[nodiscard]] constexpr T First() const {return Begin;}
	constexpr void PopFirst() {Begin = T(Begin+Step);}
	[[nodiscard]] constexpr bool Empty() const {return Step == 0;}
	[[nodiscard]] constexpr auto operator[](Index index) const {return Begin + Step*index_t(index);}

	[[nodiscard]] constexpr index_t PopFirstCount(ClampedSize elementsToPop) const
	{
		Begin = T(operator[](size_t(elementsToPop)));
		return index_t(elementsToPop);
	}
};
IotaInf() -> IotaInf<int>;
template <typename T> IotaInf(T) -> IotaInf<T, T>;
template <typename T, typename S> IotaInf(T, S) -> IotaInf<decltype(T() + S()), S>;

template<typename T, typename S = int, typename TCommon = decltype(Val<T>()+Val<S>())> [[nodiscard]] constexpr
RTake<IotaInf<TCommon, S>> Iota(T begin, T end, S step = 1)
{return Take(IotaInf(begin, step), index_t((end - begin + step - 1) / step));}

template<typename T = int> [[nodiscard]] constexpr
RTake<IotaInf<T, T>> Iota(T end)
{return Take(IotaInf(0, 1), size_t(end));}


#if INTRA_CONSTEXPR_TEST
static_assert(CRandomAccessRange<IotaInf<int, int>>);
static_assert(CRandomAccessRange<decltype(Iota(1, 2, 3))>);
static_assert(!CBidirectionalRange<IotaInf<int, int>>);
static_assert(!CFiniteRange<IotaInf<int, int>>);
#endif
INTRA_END
