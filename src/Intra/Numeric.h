#pragma once

#include "Intra/Type.h"

INTRA_BEGIN
enum class NumericType: byte {Integral, FloatingPoint, FixedPoint, Normalized};

namespace z_D {
template<typename T, bool = CPlainIntegral<T>> constexpr T lMaxOf;
template<> constexpr float lMaxOf<float, false> =
#ifdef __FLT_MAX__
	__FLT_MAX__;
#else
	3.402823466e+38f;
#endif
template<> constexpr double lMaxOf<double, false> =
#ifdef __DBL_MAX__
	__DBL_MAX__;
#else
	1.7976931348623158e+308;
#endif
template<> constexpr long double lMaxOf<long double> =
#ifdef __LDBL_MAX__
	__LDBL_MAX__;
#else
	lMaxOf<double>;
#endif
template<typename T> constexpr T lMaxOf<T, true> = CPlainUnsignedIntegral<T>? T(~T(0)): T((1ull << (sizeof(T)*8-1)) - 1);


template<typename T, bool = CPlainIntegral<T>> constexpr T lMantissaLenOf;
template<> constexpr float lMantissaLenOf<float, false> = 23;
template<> constexpr double lMantissaLenOf<double, false> = 52;
template<> constexpr long double lMantissaLenOf<long double> = lMantissaLenOf<double>;
template<typename T> constexpr T lMantissaLenOf<T, true> = sizeof(T)*8-(CPlainSignedIntegral<T>? 1: 0);

template<typename T, bool = CPlainIntegral<T>> constexpr T lExponentLenOf;
template<> constexpr float lExponentLenOf<float, false> = 8;
template<> constexpr double lExponentLenOf<double, false> = 11;
template<> constexpr long double lExponentLenOf<long double> = lExponentLenOf<double>;
template<typename T> constexpr T lExponentLenOf<T, true> = 0;

template<typename T, bool = CPlainIntegral<T>> constexpr T lExponentBiasOf;
template<> constexpr float lExponentBiasOf<float, false> = 127;
template<> constexpr double lExponentBiasOf<double, false> = 1023;
template<> constexpr long double lExponentBiasOf<long double> = lExponentBiasOf<double>;
template<typename T> constexpr T lExponentBiasOf<T, true> = 0;

template<typename T, bool = CPlainIntegral<T>> constexpr T lMinNormPositiveOf;
template<> constexpr float lMinNormPositiveOf<float, false> =
#ifdef __FLT_MIN__
	__FLT_MIN__;
#else
	1.175494351e-38f;
#endif
template<> constexpr double lMinNormPositiveOf<double, false> =
#ifdef __DBL_MIN__
	__DBL_MIN__;
#else
	2.2250738585072014e-308;
#endif
template<> constexpr long double lMinNormPositiveOf<long double, false> =
#ifdef __LDBL_MIN__
	__LDBL_MIN__;
#else
	lMinNormPositiveOf<double>;
#endif
template<typename T> constexpr T lMinNormPositiveOf<T, true> = 1;

template<typename T, bool = CPlainIntegral<T>> constexpr T lMinDenormPositiveOf;
template<> constexpr float lMinDenormPositiveOf<float, false> =
#ifdef __FLT_DENORM_MIN__
	__FLT_DENORM_MIN__;
#else
	1.401298464e-45f;
#endif
template<> constexpr double lMinDenormPositiveOf<double, false> =
#ifdef __DBL_DENORM_MIN__
	__DBL_DENORM_MIN__;
#else
	4.9406564584124654e-324;
#endif
template<> constexpr long double lMinDenormPositiveOf<long double, false> =
#ifdef __LDBL_DENORM_MIN__
	__LDBL_DENORM_MIN__;
#else
	lMinDenormPositiveOf<double>;
#endif
template<typename T> constexpr T lMinDenormPositiveOf<T, true> = 1;

template<typename T, bool = CPlainIntegral<T>> constexpr T lEpsOf;
template<> constexpr float lEpsOf<float, false> =
#ifdef __FLT_EPSILON__
	__FLT_EPSILON__;
#else
	1.192092896e-7f;
#endif
template<> constexpr double lEpsOf<double, false> =
#ifdef __DBL_EPSILON__
	__DBL_EPSILON__;
#else
	2.2204460492503131e-16;
#endif
template<> constexpr long double lEpsOf<long double, false> =
#ifdef __LDBL_EPSILON__
	__LDBL_EPSILON__;
#else
	lEpsOf<double>;
#endif
template<typename T> constexpr T lEpsOf<T, true> = 1;

template<typename T, bool = CPlainIntegral<T> || CPlainFloatingPoint<T>> constexpr T lTypeOf;
template<typename T> constexpr T lTypeOf<T, true> = CPlainIntegral<T>? NumericType::Integral: NumericType::FloatingPoint;
}

template<typename T> constexpr T LMaxOf = z_D::lMaxOf<T>;
template<typename T> constexpr T LMinOf = CPlainUnsignedIntegral<T>? 0:
	CPlainSignedIntegral<T>? T(1ull << (sizeof(T)*8-1)): -LMaxOf<T>;

template<typename T> constexpr T LMantissaLenOf = z_D::lMantissaLenOf<T>;
template<typename T> constexpr T LExponentLenOf = z_D::lExponentLenOf<T>;
template<typename T> constexpr T LExponentBiasOf = z_D::lExponentBiasOf<T>;

//! The difference between 1.0 and the next representable value of the given type 
template<typename T> constexpr T LEpsOf = z_D::lEpsOf<T>;

template<typename T> constexpr T LMinNormPositiveOf = z_D::lMinNormPositiveOf<T>;
template<typename T> constexpr T LMinDenormPositiveOf = z_D::lMinDenormPositiveOf<T>;

template<typename R, typename T> class TCommonRange
{
	static_assert(CPlainIntegral<R> && CPlainIntegral<T>);
	static constexpr R calcMin()
	{
		if constexpr(CPlainUnsignedIntegral<R> || CPlainUnsignedIntegral<T>) return 0;
		else if constexpr(sizeof(R) >= sizeof(T)) return R(LMinOf<T>);
		else return LMinOf<R>;
	}
	static constexpr R calcMax()
	{
		constexpr bool isNarrowingCast = sizeof(R) < sizeof(T) ||
			sizeof(R) == sizeof(T) && CPlainUnsignedIntegral<R> != CPlainUnsignedIntegral<T>;
		if constexpr(!isNarrowingCast || sizeof(R) > sizeof(T) || CPlainUnsignedIntegral<R>) return R(LMaxOf<T>);
		else return LMaxOf<R>;
	}
public:
	static constexpr R Min = calcMin();
	static constexpr R Max = calcMax();
};


namespace z_D {
template<size_t Size, bool Signed = false> struct TIntMin_;
template<> struct TIntMin_<1, false> {typedef byte _;};
template<> struct TIntMin_<1, true> {typedef int8 _;};
template<> struct TIntMin_<2, false> {typedef uint16 _;};
template<> struct TIntMin_<2, true> {typedef short _;};

template<> struct TIntMin_<3, false> {typedef uint32 _;};
template<> struct TIntMin_<4, false> {typedef uint32 _;};
template<> struct TIntMin_<3, true> {typedef int32 _;};
template<> struct TIntMin_<4, true> {typedef int32 _;};

template<> struct TIntMin_<5, false> {typedef uint64 _;};
template<> struct TIntMin_<6, false> {typedef uint64 _;};
template<> struct TIntMin_<7, false> {typedef uint64 _;};
template<> struct TIntMin_<8, false> {typedef uint64 _;};
template<> struct TIntMin_<5, true> {typedef int64 _;};
template<> struct TIntMin_<6, true> {typedef int64 _;};
template<> struct TIntMin_<7, true> {typedef int64 _;};
template<> struct TIntMin_<8, true> {typedef int64 _;};
}
template<size_t SIZE, bool Signed = false> using TIntMin = typename z_D::TIntMin_<SIZE, Signed>::_;

template<typename T> using TToUnsigned = TIntMin<CPlainIntegral<T>? sizeof(T): 0xBad, false>;
template<typename T> using TToSigned = TIntMin<CPlainIntegral<T>? sizeof(T): 0xBad, true>;

template<typename T> constexpr Requires<CIntegral<T>, bool> CanMultiplyWithoutOverflow(T a, T b)
{
	using MaxType = TIntMin<sizeof(int64), CSigned<T>>;
	if(MaxType(LMaxOf<T>) < LMaxOf<MaxType>)
	{
		const auto mul = MaxType(a)*MaxType(b);
		return mul == MaxType(T(mul));
	}
	if(a == 0) return true;
	const auto apos = TToUnsigned<T>(FAbs(a));
	const auto bpos = TToUnsigned<T>(FAbs(b));
	const auto mulPos = apos*bpos;
	return mulPos <= TToUnsigned<T>(LMaxOf<T>) && apos == mulPos / apos;
}

INTRA_END
