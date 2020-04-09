#pragma once

/** Header file for advanced float manipulation:
  1) Infinity
  2) NaN
  3) mantissa and exponent extraction
  4) binary to decimal float conversion
*/

#include "Intra/Misc/RawMemory.h"
#include "Intra/Numeric.h"

INTRA_BEGIN

constexpr float Infinity =
#if defined(_MSC_VER) && _MSC_VER < 1924 && !defined(__clang__)
INTRA_IGNORE_WARNINGS_MSVC(4056)
    float(1e300*1e300);
#else
    __builtin_huge_valf();
#endif

constexpr class
{
    static constexpr bool isNan(float x) noexcept
    {
#if defined(_M_FP_FAST) || defined(__FAST_MATH__)
        if(IsConstantEvaluated(x)) return false;
        return (Misc::BitCast<uint32>(x) << 1) > 0xFF000000u;
#else
        return x != x;
#endif
    }
public:
    template<typename T, typename = Requires<CFloatingPoint<T>>>
	[[nodiscard]] constexpr bool operator==(T x) const noexcept {return isNan(x);}

    template<typename T, typename = Requires<CFloatingPoint<T>>>
	[[nodiscard]] constexpr bool operator!=(T x) const noexcept {return !isNan(x);}

    template<typename T, typename = Requires<CFloatingPoint<T>>>
	friend constexpr bool operator==(T x, TNaN) noexcept {return isNan(x);}

    template<typename T, typename = Requires<CFloatingPoint<T>>>
	friend constexpr bool operator!=(T x, TNaN) noexcept {return !isNan(x);}

    template<typename T, typename = Requires<CFloatingPoint<T>>>
#if defined(_MSC_VER) && _MSC_VER < 1924 && !defined(__clang__)
	operator T() const noexcept {return T(Infinity/Infinity);}
#else
    constexpr operator T() const noexcept {return T(__builtin_nan(""));}
#endif
} NaN;

#ifdef INTRA_CONSTEXPR_BITCAST_SUPPORT
#define INTRA_BITCAST_CONSTEXPR constexpr
#else
#define INTRA_BITCAST_CONSTEXPR
#endif

#if defined(INTRA_CONSTEXPR_BITCAST_SUPPORT) || !defined(INTRA_AGRESSIVE_CONSTEXPR)
//The most performant option, constexpr since C++20
INTRA_BITCAST_CONSTEXPR inline uint32 ExtractBiasedExponent(float x) noexcept {return (Misc::BitCast<uint32>(x) >> 23) & 0xFF;}
INTRA_BITCAST_CONSTEXPR inline uint64 ExtractBiasedExponent(double x) noexcept {return (Misc::BitCast<uint64>(x) >> 52) & 0x7FF;}
INTRA_BITCAST_CONSTEXPR inline uint64 ExtractBiasedExponent(long double x) noexcept {return ExtractBiasedExponent(double(x));}
INTRA_BITCAST_CONSTEXPR inline uint32 ExtractMantissa(float x) noexcept {return Misc::BitCast<uint32>(x) & 0x7FFFFF;}
INTRA_BITCAST_CONSTEXPR inline uint64 ExtractMantissa(double x) noexcept {return Misc::BitCast<uint64>(x) & 0xFFFFFFFFFFFFF;}
INTRA_BITCAST_CONSTEXPR inline uint64 ExtractMantissa(long double x) noexcept {return ExtractMantissa(double(x));}
#elif defined(INTRA_MATH_CONSTEXPR_SUPPORT)
//Slightly slower constexpr option, available in GCC
constexpr float ExtractBiasedExponent(float x) noexcept {return 127+__builtin_ilogbf(x);}
constexpr double ExtractBiasedExponent(double x) noexcept {return 1023+__builtin_ilogb(x);}
constexpr long double ExtractBiasedExponent(long double x) noexcept {return 1023+__builtin_ilogbl(x);}
constexpr uint32 ExtractMantissa(float x) noexcept
{
	return x != Infinity && x != -Infinity?
		// remove hidden 1 and bias the exponent to get integer
		(__builtin_scalbnf(x<0? -x: x, -__builtin_ilogbf(x))-1)*(1 << 23): 0;
}
constexpr uint64 ExtractMantissa(double x) noexcept
{
	return x != Infinity && x != -Infinity?
		// remove hidden 1 and bias the exponent to get integer
		__builtin_scalbn(__builtin_scalbn(x<0? -x: x, -__builtin_ilogb(x))-1, 52): 0;
}
constexpr uint64 ExtractMantissa(long double x) noexcept {return Mantissa(double(x));}
#else
namespace z_D {
template<typename Float> constexpr int GetExponentPositive(Float x) noexcept
{
	return x >= 2? GetExponentPositive(x*0.5f)+1:
		x < 1? GetExponentPositive(x*2)-1: 0;
}
template<typename T> [[nodiscard]] constexpr T Pow2Int(int y) noexcept
{
	enum {BitsPerStep = sizeof(size_t)*8-1};
	T res = 1;
	int p = y < 0? -y: y;
	while(p >= BitsPerStep) res *= size_t(1) << BitsPerStep, p -= BitsPerStep;
	if(p) res *= size_t(1) << p;
	return y >= 0? res: 1/res;
}
template<typename Float> constexpr Float Scalbn(Float value, int exponent) noexcept
{return value*Pow2Int<Float>(exponent);}
}
constexpr int ExtractBiasedExponent(float x) noexcept {return 127+z_D::GetExponentPositive(x < 0? -x: x);}
constexpr int ExtractBiasedExponent(double x) noexcept {return 1023+z_D::GetExponentPositive(x < 0? -x: x);}
constexpr int ExtractBiasedExponent(long double x) noexcept {return ExtractBiasedExponent(double(x < 0? -x: x));}
constexpr uint32 ExtractMantissa(float x) noexcept
{
	return x != Infinity && x != -Infinity?
		// remove hidden 1 and bias the exponent to get integer
		(z_D::Scalbn(x<0? -x: x, -(ExtractBiasedExponent(x)-127))-1)*(1 << 23): 0;
}
constexpr uint64 ExtractMantissa(double x) noexcept
{
	return x != Infinity && x != -Infinity?
		// remove hidden 1 and bias the exponent to get integer
		z_D::Scalbn(z_D::Scalbn(x<0? -x: x, -(ExtractBiasedExponent(x)-1023))-1, 52): 0;
}
constexpr uint64 ExtractMantissa(long double x) noexcept {return Mantissa(double(x));}
#endif

#undef INTRA_BITCAST_CONSTEXPR


// A floating decimal representing Mantissa * 10^Exponent.
template<typename MantissaT> struct DecimalFloat
{
	MantissaT Mantissa;
	int Exponent;
};

INTRA_END
