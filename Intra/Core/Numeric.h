#pragma once

#include "Core/Type.h"

INTRA_BEGIN
constexpr forceinline float LMaxOf(float) noexcept {
#ifdef __FLT_MAX__ //clang, gcc, icc, ...
	return __FLT_MAX__;
#else
	return 3.402823466e+38f;
#endif
}

constexpr forceinline float LMinNormPositiveOf(float) noexcept {
#ifdef __FLT_MIN__ //clang, gcc, icc, ...
	return __FLT_MIN__;
#else
	return 1.175494351e-38f;
#endif
}

constexpr forceinline float LMinDenormPositiveOf(float) noexcept {
#ifdef __FLT_DENORM_MIN__ //clang, gcc, icc, ...
	return __FLT_DENORM_MIN__;
#else
	return 1.401298464e-45f;
#endif
}

constexpr forceinline float LEpsOf(float) noexcept {
#ifdef __FLT_EPSILON__ //clang, gcc, icc, ...
	return __FLT_EPSILON__;
#else
	return 1.192092896e-07f;
#endif
}

constexpr forceinline double LMaxOf(double) noexcept {
#ifdef __DBL_MAX__ //clang, gcc, icc, ...
	return __DBL_MAX__;
#else
	return 1.7976931348623158e+308;
#endif
}

constexpr forceinline double LMinNormPositiveOf(double) noexcept {
#ifdef __DBL_MIN__ //clang, gcc, icc, ...
	return __DBL_MIN__;
#else
	return 2.2250738585072014e-308;
#endif
}

constexpr forceinline double LMinDenormPositiveOf(double) noexcept {
#ifdef __DBL_DENORM_MIN__ //clang, gcc, icc, ...
	return __DBL_DENORM_MIN__;
#else
	return 4.9406564584124654e-324;
#endif
}

constexpr forceinline double LEpsOf(double) noexcept {
#ifdef __DBL_EPSILON__ //clang, gcc, icc, ...
	return __DBL_EPSILON__;
#else
	return 2.2204460492503131e-016;
#endif
}

constexpr forceinline long double LMaxOf(long double) noexcept {
#ifdef __LDBL_MAX__ //clang, gcc, icc, ...
	return __LDBL_MAX__;
#else
	//TODO: assume long double == double on other compilers
	return 1.7976931348623158e+308;
#endif
}

constexpr forceinline long double LMinNormPositiveOf(long double) noexcept {
#ifdef __LDBL_MIN__ //clang, gcc, icc, ...
	return __LDBL_MIN__;
#else
	//TODO: assume long double == double on other compilers
	return 2.2250738585072014e-308;
#endif
}

constexpr forceinline long double LMinDenormPositiveOf(long double) noexcept {
#ifdef __LDBL_DENORM_MIN__ //clang, gcc, icc, ...
	return __LDBL_DENORM_MIN__;
#else
	//TODO: assume long double == double on other compilers
	return 2.2250738585072014e-308;
#endif
}

constexpr forceinline long double LEpsOf(long double) noexcept {
#ifdef __LDBL_EPSILON__ //clang, gcc, icc, ...
	return __LDBL_EPSILON__;
#else
	//TODO: assume long double == double on other compilers
	return 2.2204460492503131e-016;
#endif
}

template<typename T> constexpr forceinline Requires<
	CPlainUnsignedIntegral<T>,
T> LMaxOf(T) noexcept {return T(~T(0));}

template<typename T> constexpr forceinline Requires<
	CPlainSignedIntegral<T>,
T> LMaxOf(T) noexcept {return T((1ull << (sizeof(T)*8-1)) - 1);}

template<typename T> constexpr forceinline Requires<
	CPlainUnsignedIntegral<T>,
T> LMinOf(T) noexcept {return 0;}

template<typename T> constexpr forceinline Requires<
	CPlainSignedIntegral<T>,
T> LMinOf(T) noexcept {return T(1ull << (sizeof(T)*8-1));}

constexpr forceinline float LMinOf(float x) noexcept {return -LMaxOf(x);}
constexpr forceinline double LMinOf(double x) noexcept {return -LMaxOf(x);}
constexpr forceinline long double LMinOf(long double x) noexcept {return -LMaxOf(x);}

template<typename T> constexpr forceinline Requires<
	CPlainIntegral<T>,
T> LMantissaLenOf(T) noexcept {return sizeof(T)*8-size_t(CPlainSignedIntegral<T>);}

constexpr forceinline int LMantissaLenOf(float) noexcept {return 23;}
constexpr forceinline int LMantissaLenOf(double) noexcept {return 52;}
constexpr forceinline int LMantissaLenOf(long double) noexcept {return 52;}

template<typename T> constexpr forceinline Requires<
	CPlainIntegral<T>,
T> LExponentLenOf(T) noexcept {return 0;}

constexpr forceinline float LExponentLenOf(float) noexcept {return 8;}
constexpr forceinline double LExponentLenOf(double) noexcept {return 11;}
constexpr forceinline long double LExponentLenOf(long double) noexcept {return 11;}

template<typename T> constexpr forceinline Requires<
	CPlainIntegral<T>,
T> LExponentBiasOf(T) noexcept {return 0;}

constexpr forceinline float LExponentBiasOf(float) noexcept {return 127;}
constexpr forceinline double LExponentBiasOf(double) noexcept {return 1023;}
constexpr forceinline long double LExponentBiasOf(long double) noexcept {return 1023;}

template<typename T> constexpr forceinline Requires<
	CPlainIntegral<T>,
T> LMinPositiveNormOf(T) noexcept {return 1;}

//! @returns the difference between 1.0 and the next representable value of the given type 
template<typename T> constexpr forceinline Requires<
	CPlainIntegral<T>,
T> LEpsOf(T) noexcept {return 1;}

enum class NumericType: byte {Integral, FloatingPoint, FixedPoint, Normalized};

template<typename T> constexpr forceinline Requires<
	CPlainIntegral<T>,
NumericType> LTypeOf(T) noexcept {return NumericType::Integral;}
template<typename T> constexpr forceinline Requires<
	CPlainFloatingPoint<T>,
NumericType> LTypeOf(T) noexcept {return NumericType::FloatingPoint;}



namespace D {
template<size_t Size, bool Signed=false> struct TIntMin_;
template<> struct TIntMin_<1, false> {typedef byte _;};
template<> struct TIntMin_<1, true> {typedef sbyte _;};
template<> struct TIntMin_<2, false> {typedef ushort _;};
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
template<size_t SIZE> using TIntMin = typename D::TIntMin_<SIZE>::_;

namespace D {
	template<typename T> struct TLargerInt_ {};
#define DECLARE_LARGER_INT_TYPE(T, LARGER) template<> struct TLargerInt_<T> {typedef LARGER _;}
	DECLARE_LARGER_INT_TYPE(sbyte, short);
	DECLARE_LARGER_INT_TYPE(byte, ushort);
	DECLARE_LARGER_INT_TYPE(short, int32);
	DECLARE_LARGER_INT_TYPE(ushort, uint32);
	DECLARE_LARGER_INT_TYPE(int32, int64);
	DECLARE_LARGER_INT_TYPE(uint32, uint64);
#undef DECLARE_LARGER_INT_TYPE
}
template<typename T> using TLargerInt = typename D::TLargerInt_<T>::_;

namespace D
{
template<typename T> struct TMakeUnsignedT {typedef T _;};
#define DECLARE_UNSIGNED_TYPE(T, UNSIGNED) template<> struct TMakeUnsignedT<T> {typedef UNSIGNED _;}
DECLARE_UNSIGNED_TYPE(sbyte, byte);
DECLARE_UNSIGNED_TYPE(short, ushort);
DECLARE_UNSIGNED_TYPE(int, uint);
DECLARE_UNSIGNED_TYPE(long, unsigned long);
DECLARE_UNSIGNED_TYPE(int64, uint64);
#undef DECLARE_UNSIGNED_TYPE
}
template<typename T> using TMakeUnsigned = typename D::TMakeUnsignedT<T>::_;

INTRA_END
