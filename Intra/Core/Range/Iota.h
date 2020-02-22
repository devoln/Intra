#pragma once

#include "Core/Range/Concepts.h"
#include "Core/Range/Take.h"

INTRA_BEGIN
template<typename T, typename S> struct RIota
{
	enum: bool {RangeIsInfinite = true};

	T Begin{};
	S Step{};

	RIota() = default;
	constexpr forceinline RIota(null_t) {}
	constexpr forceinline RIota(T begin, S step): Begin(begin), Step(step) {}

	INTRA_NODISCARD constexpr forceinline T First() const {return Begin;}
	constexpr forceinline void PopFirst() {Begin = T(Begin+Step);}
	INTRA_NODISCARD constexpr forceinline bool Empty() const {return Step == 0;}
	INTRA_NODISCARD constexpr forceinline auto operator[](size_t index) const {return Begin + Step*index;}

	INTRA_NODISCARD constexpr forceinline bool operator==(const RIota<T, S>& rhs) const
	{return (Begin == rhs.Begin || Step == 0) && Step == rhs.Step;}

	INTRA_NODISCARD constexpr forceinline auto operator()(size_t start, size_t end) const
	{
		INTRA_PRECONDITION(start <= end);
		return RTake<RIota>(RIota(T(operator[](start)), Step), end - start);
	}
};

template<typename T=int, typename S=int, typename TCommon=decltype(Val<T>()+Val<S>())> INTRA_NODISCARD constexpr forceinline
RIota<TCommon, S> IotaInf(T begin = 0, S step = 1)
{return {begin, step};}

template<typename T, typename S=int, typename TCommon=decltype(Val<T>()+Val<S>())> INTRA_NODISCARD constexpr forceinline
RTake<RIota<TCommon, S>> Iota(T begin, T end, S step = 1)
{return Take(IotaInf(begin, step), size_t((end - begin + step - 1) / step));}

template<typename T=int> INTRA_NODISCARD constexpr forceinline
RTake<RIota<T, T>> Iota(T end)
{return Take(IotaInf(0, 1), size_t(end));}


#if INTRA_CONSTEXPR_TEST
static_assert(CInputRange<RIota<int, int>>, "TEST FAILED!");
static_assert(CForwardRange<decltype(Iota(1, 2, 3))>, "TEST FAILED!");
static_assert(!CBidirectionalRange<RIota<int, int>>, "TEST FAILED!");
static_assert(CRandomAccessRange<RIota<int, int>>, "TEST FAILED!");
static_assert(!CFiniteRandomAccessRange<RIota<int, int>>, "TEST FAILED!");
#endif
INTRA_END
