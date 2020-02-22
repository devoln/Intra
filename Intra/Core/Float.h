#pragma once

/** Header file for advanced float manipulation:
  1) Infinity
  2) NaN
  3) mantissa and exponent extraction
  4) binary to decimal float conversion
*/

#include "Core/Core.h"
#include "Core/Misc/RawMemory.h"
#include "Core/Numeric.h"

INTRA_BEGIN

#if defined(_MSC_VER) && !defined(__clang__)
#pragma warning(disable: 4056)
static constexpr float Infinity = float(1e300*1e300);
#else
constexpr float Infinity = __builtin_huge_valf();
#endif

struct TNaN
{
#if !defined(_MSC_VER) || defined(__clang__)
	forceinline bool operator==(float rhs) const noexcept {return __builtin_isnan(rhs) != 0;}
	forceinline bool operator==(double rhs) const noexcept {return __builtin_isnan(float(rhs)) != 0;}
	forceinline bool operator==(long double rhs) const noexcept {return __builtin_isnan(float(rhs)) != 0;}
#ifdef __FAST_MATH__
	//#warning NaN checking may not work.
#endif
#else
	forceinline bool operator==(float rhs) const noexcept {return rhs != rhs;}
	forceinline bool operator==(double rhs) const noexcept {return rhs != rhs;}
	forceinline bool operator==(long double rhs) const noexcept {return rhs != rhs;}
#ifdef _M_FP_FAST
	//#warning NaN checking may not work.
#endif
#endif

	forceinline bool operator!=(float rhs) const noexcept {return !operator==(rhs);}
	forceinline bool operator!=(double rhs) const noexcept {return !operator==(rhs);}
	forceinline bool operator!=(long double rhs) const noexcept {return !operator==(double(rhs));}

	friend forceinline bool operator==(float l, TNaN) noexcept {return TNaN{} == l;}
	friend forceinline bool operator!=(float l, TNaN) noexcept {return TNaN{} != l;}
	friend forceinline bool operator==(double l, TNaN) noexcept {return TNaN{} == l;}
	friend forceinline bool operator!=(double l, TNaN) noexcept {return TNaN{} != l;}
	friend forceinline bool operator==(long double l, TNaN) noexcept {return TNaN{} == l;}
	friend forceinline bool operator!=(long double l, TNaN) noexcept {return TNaN{} != l;}

	forceinline operator float() const noexcept {return float(Infinity/Infinity);}
	forceinline operator double() const noexcept {return double(Infinity/Infinity);}
	forceinline operator long double() const noexcept {return static_cast<long double>(Infinity/Infinity);}
};
static constexpr TNaN NaN{};

#ifdef INTRA_CONSTEXPR_BITCAST_SUPPORT
#define INTRA_BITCAST_CONSTEXPR constexpr
#else
#define INTRA_BITCAST_CONSTEXPR
#endif

#if defined(INTRA_CONSTEXPR_BITCAST_SUPPORT) || !defined(INTRA_AGRESSIVE_CONSTEXPR)
//The most performant option, constexpr since C++20
INTRA_BITCAST_CONSTEXPR forceinline uint32 ExtractBiasedExponent(float x) noexcept {return (Misc::BitCast<uint32>(x) >> 23) & 0xFF;}
INTRA_BITCAST_CONSTEXPR forceinline uint64 ExtractBiasedExponent(double x) noexcept {return (Misc::BitCast<uint64>(x) >> 52) & 0x7FF;}
INTRA_BITCAST_CONSTEXPR forceinline uint64 ExtractBiasedExponent(long double x) noexcept {return ExtractBiasedExponent(double(x));}
INTRA_BITCAST_CONSTEXPR forceinline uint32 ExtractMantissa(float x) noexcept {return Misc::BitCast<uint32>(x) & 0x7FFFFF;}
INTRA_BITCAST_CONSTEXPR forceinline uint64 ExtractMantissa(double x) noexcept {return Misc::BitCast<uint64>(x) & 0xFFFFFFFFFFFFF;}
INTRA_BITCAST_CONSTEXPR forceinline uint64 ExtractMantissa(long double x) noexcept {return ExtractMantissa(double(x));}
#elif defined(INTRA_MATH_CONSTEXPR_SUPPORT)
//Slightly slower constexpr option, available in GCC
constexpr forceinline float ExtractBiasedExponent(float x) noexcept {return 127+__builtin_ilogbf(x);}
constexpr forceinline double ExtractBiasedExponent(double x) noexcept {return 1023+__builtin_ilogb(x);}
constexpr forceinline long double ExtractBiasedExponent(long double x) noexcept {return 1023+__builtin_ilogbl(x);}
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
constexpr forceinline uint64 ExtractMantissa(long double x) noexcept {return Mantissa(double(x));}
#else
namespace D {
template<typename Float> constexpr int GetExponentPositive(Float x) noexcept
{
	return x >= 2? GetExponentPositive(x*0.5f)+1:
		x < 1? GetExponentPositive(x*2)-1: 0;
}
template<typename T> INTRA_NODISCARD constexpr T Pow2Int(int y) noexcept
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
constexpr int ExtractBiasedExponent(float x) noexcept {return 127+D::GetExponentPositive(x < 0? -x: x);}
constexpr int ExtractBiasedExponent(double x) noexcept {return 1023+D::GetExponentPositive(x < 0? -x: x);}
constexpr int ExtractBiasedExponent(long double x) noexcept {return ExtractBiasedExponent(double(x < 0? -x: x));}
constexpr uint32 ExtractMantissa(float x) noexcept
{
	return x != Infinity && x != -Infinity?
		// remove hidden 1 and bias the exponent to get integer
		(D::Scalbn(x<0? -x: x, -(ExtractBiasedExponent(x)-127))-1)*(1 << 23): 0;
}
constexpr uint64 ExtractMantissa(double x) noexcept
{
	return x != Infinity && x != -Infinity?
		// remove hidden 1 and bias the exponent to get integer
		D::Scalbn(D::Scalbn(x<0? -x: x, -(ExtractBiasedExponent(x)-1023))-1, 52): 0;
}
constexpr forceinline uint64 ExtractMantissa(long double x) noexcept {return Mantissa(double(x));}
#endif

#undef INTRA_BITCAST_CONSTEXPR


// A floating decimal representing Mantissa * 10^Exponent.
template<typename MantissaT> struct DecimalFloat
{
	MantissaT Mantissa;
	int Exponent;
};

INTRA_END
