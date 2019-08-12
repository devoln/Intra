#pragma once

#include "Core/Core.h"

#if(!defined(INTRA_USE_CRT_MATH_HEADER) && !defined(__clang__) && !defined(__GNUC__) && \
	(!defined(_MSC_VER) || INTRA_PLATFORM_ARCH != INTRA_PLATFORM_X86))
#define INTRA_USE_CRT_MATH_HEADER
#endif

#ifdef INTRA_USE_CRT_MATH_HEADER
#include <cmath>
#endif

INTRA_BEGIN
inline namespace Math {

constexpr struct
{
	double PI = 3.1415926535897932;
	double E = 2.7182818284590452;
	double SqrtPI = 1.772453850905516;
	double SqrtE = 1.6487212707001281;
	double Sqrt2 = 1.414213562373095;
} Constants;

template<typename T> constexpr forceinline T Sqr(T n) noexcept {return n*n;}
template<typename T> constexpr forceinline T Abs(T v) noexcept {return v >= 0? v: -v;}

template<typename T> constexpr forceinline T Min(const T& t1, const T& t2) noexcept {return t1 < t2? t1: t2;}
template<typename T> constexpr forceinline T Max(const T& t1, const T& t2) noexcept {return t1 < t2? t2: t1;}

template<typename T> constexpr forceinline int ISign(T x) noexcept {return (x > 0) - (x < 0);}
template<typename T> constexpr forceinline T Sign(T x) noexcept {return T(ISign(x));}

template<typename T, typename N, typename X>
constexpr forceinline auto Clamp(T v, N minv, X maxv) noexcept {return Max(minv, Min(maxv, v));}

//! Find the nearest integer less than or equal to x.
//! x must be in range [-2^31; 2^31), otherwise return value is undefined.
constexpr forceinline int IFloor(float x) noexcept {return int(x) - int(x < 0 && x != int(x));}
#if INTRA_CONSTEXPR_TEST
static_assert(IFloor(-float(Constants.E)) == -3 && IFloor(float(Constants.E)) == 2, "ERROR");
static_assert(IFloor(-4000001.2f) == -4000002 && IFloor(4000001.2f) == 4000001, "ERROR");
//static_assert(IFloor(-20000000016.0) == -20000000016, "ERROR"); //fails - integer overflow
#endif

//! Find the nearest integer less than or equal to x.
//! x must be in range [-2^63; 2^63), otherwise return value is undefined.
constexpr forceinline int64 IFloor(double x) noexcept {return int64(x) - int64(x < 0 && x != int64(x));}
#if INTRA_CONSTEXPR_TEST
static_assert(IFloor(-Constants.E) == -3 && IFloor(Constants.E) == 2, "ERROR");
static_assert(IFloor(-20000000016.0) == -20000000016, "ERROR");
static_assert(IFloor(-2000000001.6) == -2000000002 && IFloor(2000000001.6) == 2000000001, "ERROR");
#endif

//! Fixed point 16:16 natural logarithm implementation.
INTRA_CONSTEXPR2 inline int FixedPointLog(uint x)
{
	int y = 0xa65af;
	if(x < 0x00008000) x <<= 16, y -= 0xb1721;
	if(x < 0x00800000) x <<= 8, y -= 0x58b91;
	if(x < 0x08000000) x <<= 4, y -= 0x2c5c8;
	if(x < 0x20000000) x <<= 2, y -= 0x162e4;
	if(x < 0x40000000) x <<= 1, y -= 0x0b172;
	uint t = x + (x >> 1); if((t & 0x80000000u) == 0) x = t, y -= 0x067cd;
	t = x + (x >> 2); if((t & 0x80000000u) == 0) x = t, y -= 0x03920;
	t = x + ( x >> 3); if((t & 0x80000000u) == 0) x = t, y -= 0x01e27;
	t = x + (x >> 4); if((t & 0x80000000u) == 0) x = t, y -= 0x00f85;
	t = x + ( x >> 5); if((t & 0x80000000u) == 0) x = t, y -= 0x007e1;
	t = x + (x >> 6); if((t & 0x80000000u) == 0) x = t, y -= 0x003f8;
	t = x + (x >> 7); if((t & 0x80000000u) == 0) x = t, y -= 0x001fe;
	x = uint(0x80000000 - uint(x));
	y -= int(x >> 15);
	return y;
}

#if INTRA_CONSTEXPR_TEST >= 201304
static_assert(FixedPointLog(65536) == 0, "ERROR");
static_assert(FixedPointLog(100000) == 27694, "ERROR");
static_assert(FixedPointLog(uint(Constants.E*65536)) == 65537, "ERROR"); // must be 65536, rounding error?
#endif

#ifdef INTRA_USE_CRT_MATH_HEADER
#include "detail/CRTHeaderMathFunctions.h"
#elif (defined(__clang__) || defined(__GNUC__)) && !defined(_MSC_VER)
#include "detail/MathBuiltins.h"
#else
//Implementation of math functions from scratch
#include "detail/OwnMathFunctions.h"
//TODO: as an alternative for MSVC without standard headers implement math function via manually imported CRT functions
#endif

INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline long double Fract(long double x) {return x - Floor(x);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline double Fract(double x) {return x - Floor(x);}
INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline float Fract(float x) {return x - Floor(x);}

template<typename T> INTRA_NODISCARD INTRA_CONSTEXPR2 T PowInt(T x, int y)
{
	uint n = uint(Abs(y));
	for(T z = T(1); ; x *= x)
	{
		if((n & 1) != 0) z *= x;
		if((n >>= 1) == 0) return (y < 0? T(1)/z: z);
	}
}

template<typename T> INTRA_NODISCARD INTRA_MATH_CONSTEXPR forceinline T Pow2(T x) {return Exp(x*T(0.6931472));}

template<typename T> INTRA_MATH_CONSTEXPR2 T Erf1(T x)
{
	static const double P[] = {3.16112374387056560, 113.864154151050156, 377.485237685302021};
	static const double Q[] = {23.6012909523441209, 244.024637934444173, 1282.61652607737228};
	const T x2 = x*x;
	T xnum = T(0.185777706184603153*x2);
	T xden = x2;
	for(int i = 0; i < 3; i++)
	{
		xnum = T((xnum + P[i]) * x2);
		xden = T((xden + Q[i]) * x2);
	}
	return x * T(xnum + 3209.37758913846947) / T(xden + 2844.23683343917062);
}

template<typename T> INTRA_NODISCARD INTRA_MATH_CONSTEXPR2 T Erfc2(T x)
{
	static const double P[] = {0.564188496988670089, 8.88314979438837594, 66.1191906371416295,
		298.635138197400131, 881.952221241769090, 1712.04761263407058, 2051.07837782607147};
	static const double Q[] = {15.7449261107098347, 117.693950891312499, 537.181101862009858,
		1621.38957456669019, 3290.79923573345963, 4362.61909014324716, 3439.36767414372164};
	T xnum = T(2.15311535474403846e-8 * x);
	T xden = x;
	for(int i = 0; i < 7; i++)
	{
		xnum = T((xnum + P[i])*x);
		xden = T((xden + Q[i])*x);
	}
	const T result = T((xnum + 1230.33935479799725) / (xden + 1230.33935480374942));
	const T xfloor = Floor(x*16) / 16;
	const T del = (x - xfloor) * (x + xfloor);
	return Exp(-Sqr(xfloor) - del) * result;
}

template<typename T> INTRA_NODISCARD INTRA_MATH_CONSTEXPR2 T Erfc3(T x)
{
	static const double P[] = {3.05326634961232344e-1,
		3.60344899949804439e-1, 1.25781726111229246e-1, 1.60837851487422766e-2};
	static const double Q[] = {2.56852019228982242e00,
		1.87295284992346047e00, 5.27905102951428412e-1, 6.05183413124413191e-2};
	const T x2r = 1 / Sqr(x);
	T xnum = T(0.0163153871373020978 * x2r);
	T xden = x2r;
	for(int i = 0; i < 4; i++)
	{
		xnum = T((xnum + P[i]) * x2r);
		xden = T((xden + Q[i]) * x2r);
	}
	T result = T(x2r * (xnum + 6.58749161529837803e-4) / (xden + 2.33520497626869185e-3));
	result = T((1 / Constants.SqrtPI - result) / x);
	const T xfloor = Floor(x * 16) / 16;
	const T del = (x - xfloor)*(x + xfloor);
	return Exp(-Sqr(xfloor) - del) * result;
}

template<typename T> T Erf(T x)
{
	const T xAbs = Abs(x);
	const T approx1Arg = sizeof(T) <= 4? 4: (sizeof(T) <= 8? 6: 10);
	if(xAbs >= approx1Arg) return Sign(x);
	if(xAbs <= 0.46875) return Erf1(x);
	if(xAbs <= 4) return Sign(x)*(1 - Erfc2(xAbs));
	return Sign(x)*(1 - Erfc3(xAbs));
}

template<typename T> forceinline T Erfc(T x) {return 1 - Erf(x);}

inline float FastSqrt(float x)
{
	union {float f; uint32 i;}; //TODO: UB
	f = x;
	i = (0xbe6f0000 - i) >> 1;
	x *= f;
	return x*(1.5f - 0.5f*f*x);
}

// TODO: measure if it is really fast
INTRA_NODISCARD inline float FastByteToNormalizedFloat(byte x)
{
	union {float f; uint32 i;} u; //TODO: UB
	u.f = 32768.0f;
	u.i |= x;
	return (u.f - 32768.0f)*(256.0f/255.0f);
}


INTRA_NODISCARD INTRA_CONSTEXPR2 inline uint GreatestCommonDivisor(uint a, uint b)
{
	while(a != b)
	{
		if(a > b) a -= b;
		else b -= a;
	}
	return a;
}

//inline float Fract(float v) {return v-Floor(v);}
//inline double Fract(double v) {return v-Floor(v);}



//! Linear interpolation
template<typename T, typename U> INTRA_NODISCARD constexpr T LinearMix(T x, T y, U factor)
{return T(x*(U(1) - factor) + y*factor);}

template<typename T> INTRA_NODISCARD constexpr T Step(T edge, T value) {return T(value >= edge);}

namespace D {
template<typename T> constexpr T SmoothStep_helper(T t) {return t*t*(T(3) - t*2);}
}

template<typename T> INTRA_NODISCARD constexpr T SmoothStep(T edge0, T edge1, T value)
{return D::SmoothStep_helper(Clamp((value - edge0) / (edge1 - edge0), T(0), T(1)));}


INTRA_NODISCARD INTRA_CONSTEXPR2 inline uint CeilToNextPow2(uint v)
{
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	return ++v;
}

//! Integer logarithm of base 2, rounded to smaller integer.
INTRA_NODISCARD INTRA_CONSTEXPR2 inline byte Log2i(uint x)
{
	if(x == 0) return byte(255);
	uint n = 31;
	if(x <= 0x0000ffff) n -= 16, x <<= 16;
	if(x <= 0x00ffffff) n -= 8, x <<= 8;
	if(x <= 0x0fffffff) n -= 4, x <<= 4;
	if(x <= 0x3fffffff) n -= 2, x <<= 2;
	if(x <= 0x7fffffffu) n--;
	return byte(n);
}

INTRA_NODISCARD constexpr forceinline bool IsPow2(size_t x) {return x != 0 && ((x & (x - 1)) == 0);}

}

INTRA_END
