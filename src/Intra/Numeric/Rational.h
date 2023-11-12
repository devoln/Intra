#pragma once

#include <Intra/Numeric/Exponential.h>
#include <Intra/Numeric/LongArith.h>

namespace Intra { INTRA_BEGIN

template<CBasicIntegral T> [[nodiscard]] constexpr uint32 GreatestCommonDivisor(T a, T b)
{
	while(a != b)
	{
		if(a > b) a -= b;
		else b -= a;
	}
	return a;
}

INTRA_OPTIMIZE_FUNCTION(template<CBasicUnsignedIntegral auto Divisor> constexpr auto FastDivByConstTrunc = [](auto dividend)) INTRA_FORCEINLINE_LAMBDA
{
	static_assert(Divisor != 0);
	if constexpr(sizeof(dividend) > sizeof(size_t))
	{
		// Optimize divide for some divisors used by Intra using 64 bit multiply + shift trick.
		// Most compilers cannot do this automatically on 32-bit platforms.
		constexpr auto divisorPow2Factor = PowFactor(Divisor, 2);
		constexpr auto oddDiv = Divisor >> divisorPow2Factor;
		if constexpr(oddDiv == 1) return dividend >> divisorPow2Factor;
		if constexpr(oddDiv == 5) return MulHighPart(dividend, 0xCCCCCCCCCCCCCCCD) >> (2 + divisorPow2Factor);
		if constexpr(Divisor >= 50 && oddDiv == 25) return MulHighPart(dividend, 0xA3D70A3D70A3D70B) >> (divisorPow2Factor - 1);
		if constexpr(oddDiv == 625) return MulHighPart(dividend, 0x346DC5D63886594B) >> (7 + divisorPow2Factor);
		if constexpr(oddDiv == 15625) return MulHighPart(dividend, 0x431BDE82D7B634DB) >> (12 + divisorPow2Factor);
		if constexpr(oddDiv == 390625) return MulHighPart(dividend, 0xABCC77118461CEFD) >> (18 + divisorPow2Factor);
		if constexpr(Divisor >= 3906250 && oddDiv == 1953125) return MulHighPart(dividend >> 1, 0x112E0BE826D694B3) >> (17 + divisorPow2Factor - 1);
	}
	// Most compilers replace this with efficient 64-bit multiply trick on 64-bit platforms
	// We enforce this optimization on GCC and MSVC using INTRA_OPTIMIZE_FUNCTION macro
	return dividend / Divisor;
};
INTRA_OPTIMIZE_FUNCTION_END

template<CBasicUnsignedIntegral auto Divisor> constexpr auto FastModByConst = [](auto dividend) INTRA_FORCEINLINE_LAMBDA
{
	if constexpr(sizeof(dividend) > sizeof(size_t) && Divisor < MaxValueOf<size_t>)
		return size_t(dividend) - Divisor * size_t(FastDivByConstTrunc<Divisor>(dividend));
	else return dividend - Divisor * FastDivByConstTrunc<Divisor>(dividend);
};

constexpr auto IDivFast = [](auto&& dividend, auto&& divisor)
{
	if((dividend < 0) ^ (divisor < 0))
		return (dividend - divisor / 2) / divisor;
	return (dividend + divisor / 2) / divisor;
};

template<RoundMode Rounding = RoundMode::Truncate, template<typename> class Overflow = NDebugOverflow>
constexpr auto IDiv = [](auto x, auto divisor)
{
	INTRA_PRECONDITION(divisor != 0);
	if constexpr(Rounding == RoundMode::Truncate ||
		!CBasicSigned<decltype(x)> && Rounding == RoundMode::Floor)
		return INTRA_FWD(x) / divisor;
	else if constexpr(Rounding == RoundMode::Floor)
	{
		if(x < 0) x -= divisor - 1;
		return INTRA_MOVE(x) / divisor;
	}
	else if constexpr(Rounding == RoundMode::Ceil)
	{
		x += divisor - 1;
		return INTRA_MOVE(x) / divisor;
	}
	else
	{
		auto x1 = Overflow(INTRA_MOVE(x));
		if(x1 < 0)
		{
			if(divisor < 0) return (x1 + (-divisor + 1) / 2) / divisor + 1;
			return (x1 + (divisor + 1) / 2) / divisor - 1;
		}
		if(divisor < 0) return (x1 - (-divisor + 1) / 2) / divisor - 1;
		return (x1 - (divisor + 1) / 2) / divisor + 1;
	}
};

template<auto Ratio> auto MulDivConst = []<typename T>(T&& x)
{
	if constexpr(CBasicFloatingPoint<TRemoveReference<T>>)
		return INTRA_FWD(x) * Ratio.Ratio;
	else return INTRA_FWD(x) * Ratio.Num / Ratio.Denom; // TODO: solve overflow
};

template<typename T> struct Ratio
{
	Ratio() = default;
	constexpr Ratio(T num, T denom = 1): Num(num), Denom(denom) {}

	T Num = 1;
	T Denom = 1;
};

} INTRA_END
