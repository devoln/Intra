#pragma once

#include "Cpp/Fundamental.h"
#include "Cpp/Warnings.h"
#include "Cpp/Features.h"
#include "Cpp/InfNan.h"
#include "Cpp/PlatformDetect.h"

#ifndef NOMINMAX
#define NOMINMAX
#undef min
#undef max
#endif

#if(!defined(INTRA_USE_CRT_MATH) && !defined(__clang__) && !defined(__GNUC__) && \
	(!defined(_MSC_VER) || INTRA_PLATFORM_ARCH != INTRA_PLATFORM_X86))
#define INTRA_USE_CRT_MATH
#endif

#ifdef INTRA_USE_CRT_MATH
#include <cmath>
#endif

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Math {

constexpr const double PI = 3.14159265358979323846;
constexpr const double E = 2.71828182845904523536;
constexpr const double SqrtPI = 1.77245385090551602729;
constexpr const double SqrtE = 1.6487212707001;

using Cpp::NaN;
using Cpp::Infinity;

template<typename T> constexpr forceinline T Sqr(T n) noexcept {return n*n;}
template<typename T> constexpr forceinline T Abs(T v) noexcept {return v >= 0? v: -v;}

template<typename T> constexpr forceinline T Min(const T& t1, const T& t2) noexcept {return t1 < t2? t1: t2;}
template<typename T> constexpr forceinline T Max(const T& t1, const T& t2) noexcept {return t1 < t2? t2: t1;}

template<typename T> constexpr int ISign(T x) noexcept {return (x > 0) - (x < 0);}
template<typename T> constexpr T Sign(T x) noexcept {return T(ISign(x));}

template<typename T, typename N, typename X>
constexpr auto Clamp(T v, N minv, X maxv) noexcept -> decltype(Max(minv, Min(maxv, v))) {return Max(minv, Min(maxv, v));}

constexpr inline int IFloor(float x) noexcept {return int(x) - int(x < 0);}
constexpr inline intptr IFloor(double x) noexcept {return intptr(x) - intptr(x < 0);}

//! Для фиксированной запятой 16:16.
inline int FixedPointLog(uint x)
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

#ifdef INTRA_USE_CRT_MATH
float Floor(float v);
double Floor(double v);
float Ceil(float v);
double Ceil(double v);
float Round(float v);
double Round(double v);


inline float Fract(float x) {return x - Floor(x);}
inline double Fract(double x) {return x - Floor(x);}

forceinline float Exp(float v) {return ::expf(v);}
forceinline double Exp(double v) {return ::exp(v);}

forceinline float Floor(float v) {return ::floorf(v);}
forceinline double Floor(double v) {return ::floor(v);}

forceinline float Ceil(float v) {return ::ceilf(v);}
forceinline double Ceil(double v) {return ::ceil(v);}

forceinline float Round(float v) {return ::roundf(v);}
forceinline double Round(double v) {return ::round(v);}

forceinline float Sin(float v) {return ::sinf(v);}
forceinline double Sin(double v) {return ::sin(v);}

forceinline float Cos(float v) {return ::cosf(v);}
forceinline double Cos(double v) {return ::cos(v);}

forceinline float Sinh(float v) {return ::sinhf(v);}
forceinline double Sinh(double v) {return ::sinh(v);}

forceinline float Cosh(float v) {return ::coshf(v);}
forceinline double Cosh(double v) {return ::cosh(v);}

forceinline float Tan(float v) {return ::tanf(v);}
forceinline double Tan(double v) {return ::tan(v);}

forceinline float Tanh(float v) {return ::tanhf(v);}
forceinline double Tanh(double v) {return ::tanh(v);}

forceinline float Acos(float v) {return ::acosf(v);}
forceinline double Acos(double v) {return ::acos(v);}

forceinline float Asin(float v) {return ::asinf(v); }
forceinline double Asin(double v) {return ::asin(v);}

forceinline float Atan(float v) {return ::atanf(v);}
forceinline double Atan(double v) {return ::atan(v);}

//forceinline float Atanh(float v) {return ::atanhf(v);}
//forceinline double Atanh(double v) {return ::Atanh(v);}

forceinline float Sqrt(float v) {return ::sqrtf(v);}
forceinline double Sqrt(double v) {return ::sqrt(v);}

forceinline float Log(float v) {return ::logf(v);}
forceinline double Log(double v) {return ::log(v);}

forceinline float Pow(float v, float power) {return ::powf(v, power);}
forceinline double Pow(double v, double power) {return ::pow(v, power);}

forceinline float Mod(float x, float y) {return ::fmodf(x, y);}
forceinline double Mod(double x, double y) {return ::fmod(x, y);}

#elif (defined(__clang__) || defined(__GNUC__)) && !defined(_MSC_VER)

forceinline float Floor(float x) {return __builtin_floorf(x);}
forceinline double Floor(double x) {return __builtin_floor(x);}
forceinline real Floor(real x) {return __builtin_floorl(x);}
forceinline float Ceil(float x) {return __builtin_ceilf(x);}
forceinline double Ceil(double x) {return __builtin_ceil(x);}
forceinline real Ceil(real x) {return __builtin_ceill(x);}
forceinline float Round(float x) {return Floor(x + 0.5f);}
forceinline double Round(double x) {return Floor(x + 0.5);}
forceinline real Round(real x) {return Floor(x + 0.5);}

forceinline float Fract(float x) {return x - Floor(x);}
forceinline double Fract(double x) {return x - Floor(x);}
forceinline real Fract(real x) {return x - Floor(x);}

forceinline float Sin(float radians) {return __builtin_sinf(radians);}
forceinline double Sin(double radians) {return __builtin_sin(radians);}
forceinline real Sin(real radians) {return __builtin_sinl(radians);}
forceinline float Cos(float radians) {return __builtin_cosf(radians);}
forceinline double Cos(double radians) {return __builtin_cos(radians);}
forceinline real Cos(real radians) {return __builtin_cosl(radians);}
forceinline float Tan(float radians) {return __builtin_tanf(radians);}
forceinline double Tan(double radians) {return __builtin_tan(radians);}
forceinline real Tan(real radians) {return __builtin_tanl(radians);}

forceinline float Sinh(float x) {return __builtin_sinhf(x);}
forceinline double Sinh(double x) {return __builtin_sinh(x);}
forceinline real Sinh(real x) {return __builtin_sinhl(x);}
forceinline float Cosh(float x) {return __builtin_coshf(x);}
forceinline double Cosh(double x) {return __builtin_cosh(x);}
forceinline real Cosh(real x) {return __builtin_coshl(x);}
forceinline float Tanh(float x) {return __builtin_tanhf(x);}
forceinline double Tanh(double x) {return __builtin_tanh(x);}
forceinline real Tanh(real x) {return __builtin_tanhl(x);}

forceinline float Asin(float x) {return __builtin_asinf(x);}
forceinline double Asin(double x) {return __builtin_asin(x);}
forceinline real Asin(real x) {return __builtin_asinl(x);}
forceinline float Acos(float x) {return __builtin_acosf(x);}
forceinline double Acos(double x) {return __builtin_acos(x);}
forceinline real Acos(real x) {return __builtin_acosl(x);}

forceinline float Atan(float x) {return __builtin_atanf(x);}
forceinline double Atan(double x) {return __builtin_atan(x);}
forceinline real Atan(real x) {return __builtin_atanl(x);}

forceinline float Atanh(float x) {return __builtin_atanhf(x);}
forceinline double Atanh(double x) {return __builtin_atanh(x);}
forceinline real Atanh(real x) {return __builtin_atanhl(x);}

forceinline float Sqrt(float x) {return __builtin_sqrtf(x);}
forceinline double Sqrt(double x) {return __builtin_sqrt(x);}
forceinline real Sqrt(real x) {return __builtin_sqrtl(x);}

forceinline float Log(float x) {return __builtin_logf(x);}
forceinline double Log(double x) {return __builtin_log(x);}
forceinline real Log(real x) {return __builtin_logl(x);}

forceinline float Mod(float x, float y) {return __builtin_fmodf(x, y);}
forceinline double Mod(double x, double y) {return __builtin_fmod(x, y);}
forceinline real Mod(real x, real y) {return __builtin_fmodl(x, y);}

forceinline float Pow(float x, float power) {return __builtin_powf(x, power);}
forceinline double Pow(double x, double power) {return __builtin_pow(x, power);}
forceinline real Pow(real x, real power) {return __builtin_powl(x, power);}
forceinline float Pow(float x, int power) {return __builtin_powif(x, power);}
forceinline double Pow(double x, int power) {return __builtin_powi(x, power);}
forceinline real Pow(real x, int power) {return __builtin_powil(x, power);}

forceinline float Exp(float x) {return __builtin_expf(x);}
forceinline double Exp(double x) {return __builtin_exp(x);}
forceinline real Exp(real x) {return __builtin_expl(x);}

#else


#if(INTRA_PLATFORM_ARCH == INTRA_PLATFORM_X86)

#if(INTRA_COMPILER_INLINE_ASM_SYNTAX == INTRA_COMPILER_INLINE_ASM_SYNTAX_INTEL)

#ifdef _MSC_VER
#pragma warning(disable: 4725)
#endif

inline double Sin(double a)
{
	double r;
	__asm
	{
		fld a
		fsin
		fstp r
	}
	return r;
}



inline double Cos(double a)
{
	double r;
	__asm
	{
		fld a
		fcos
		fstp r
	}
	return r;
}

inline double Tan(double a)
{
	double r;
	__asm
	{
		fld a
		fptan
		fstp a //Ненужное значение 1.0 надо куда-то выкинуть из стека
		fstp r
	}
	return r;
}

inline double Atan(double x)
{
	double r;
	__asm
	{
		fld x
		fld1
		fpatan
		fstp r
	}
	return r;
}



inline double Sqrt(double x)
{
	double r;
	__asm
	{
		fld x
		fsqrt
		fstp r
	}
	return r;
}

inline double Mod(double x, double y)
{
	double r;
	__asm
	{
		fld y
		fld x
		fprem
		fstp r
		fstp y
	}
	return r;
}


inline double Exp(double x)
{
	__asm
	{
		fld QWORD PTR x
		fldl2e
		fmulp st(1), st(0)
		fld st(0)
		frndint
		fsubr st(1), st(0)
		fxch
		fchs
		f2xm1
		fld1
		faddp st(1), st(0)
		fxch
		fld1
		fscale
		fstp st(1)
		fmulp st(1), st(0)
		fstp QWORD PTR x
	}
	return x;
}

#else

inline double Sin(double val)
{
	double result;
	asm volatile ("fldl %1;"
		"fsin;"
		"fstpl %0;" : "=g" (result) : "g" (val)
		);
	return result;
}

inline double Cos(double val)
{
	double result;
	asm volatile ("fldl %1;"
		"fcos;"
		"fstpl %0;" : "=g" (result) : "g" (val)
		);
	return result;
}

inline double Sqrt(double val)
{
	double result;
	asm volatile ("fldl %1;"
		"fsqrt;"
		"fstpl %0;" : "=g" (result) : "g" (val)
		);
	return result;
}

inline double Mod(double x, double y)
{
	double result;
	asm volatile ("fldl %2;"
		"fldl %1;"
		"fprem1;"
		"fstpl %0;" : "=g" (result) : "g" (x), "g" (y)
		);
	return result;
}

inline double Exp(double x)
{
	double result;
	asm("fldl2e; "
		"fmulp; "
		"fld %%st; "
		"frndint; "
		"fsub %%st,%%st(1); "
		"fxch;"
		"fchs; "
		"f2xm1; "
		"fld1; "
		"faddp; "
		"fxch; "
		"fld1; "
		"fscale; "
		"fstp %%st(1); "
		"fmulp" :  "=t"(result) : "0"(x));
	return result;
}

#endif



#endif
inline double Fract(double x) {return x-(double)IFloor(x);}
inline double Floor(double x) {return (double)IFloor(x);}
inline double Ceil(double x) {return Floor(x+0.999999);}
inline double Round(double x) {return Floor(x+0.5);}

inline float Fract(float x) {return x-float(IFloor(x));}
inline float Floor(float x) {return float(IFloor(x));}
inline float Ceil(float x) {return Floor(x+0.999999f);}
inline float Round(float x) {return Floor(x+0.5f);}

//forceinline float Floor(float x) {return float(Floor(double(x)));}
//forceinline float Ceil(float x) {return float(Ceil(double(x)));}
//forceinline float Round(float v) {return float(Round(double(v)));}
inline float Sin(float x) {return float(Sin(double(x)));}
inline float Cos(float x) {return float(Cos(double(x)));}
inline float Tan(float x) {return float(Tan(double(x)));}
inline float Atan(float x) {return float(Atan(double(x)));}
inline float Sqrt(float x) {return float(Sqrt(double(x)));}
inline float Exp(float x) {return float(Exp(double(x)));}
inline float Mod(float x, float y) {return float(Mod(double(x), double(y)));}


inline double Acos(double x) {return Atan(Sqrt(1.0 - x*x)/x);}
inline float Acos(float x) {return Atan(Sqrt(1.0f - x*x)/x);}
inline double Asin(double x) {return Atan(x/Sqrt(1.0 - x*x));}
inline float Asin(float x) {return Atan(x/Sqrt(1.0f - x*x));}

//forceinline double Tan(double x) {return Sin(x)/Cos(x);}
//forceinline float Tan(float x) {return Sin(x)/Cos(x);}

//inline double Mod(double x, double y) {return x-Floor(x/y)*y;}
//inline float Mod(float x, float y) {return x-Floor(x/y)*y;}



/*template<typename T> T Log(T v)
{
	T P = v;
	T N = 0.0;
	while(P >= T(E))
	{
		P /= T(E);
		N++;
	}
	N += (P / T(E));
	P = v;

	T A;
	do
	{
		A = N;
		T L = (P / (Exp(N - T(1.0))));
		T R = ((N - T(1.0)) * T(E));
		N = ((L + R) / T(E));
	} while(!(Abs(N-A)<T(0.01)));
	return N;
}*/

//! Логарифм для чисел, меньших 65535
inline float Log(float x) {return float(FixedPointLog(uint(x * 65536)))/65536.0f;}
inline double Log(double x) {return double(FixedPointLog(uint(x * 65536)))/65536.0;}
inline real Log(real x) {return real(FixedPointLog(uint(x * 65536)))/65536.0;}

inline float Pow(float v, float power) {return power == 0.0f? 1.0f: Exp(Log(v)*power);}
inline double Pow(double v, double power) {return power == 0.0? 1.0: Exp(Log(v)*power);}

#endif

template<typename T> T PowInt(T x, int y)
{
	uint n = uint(Abs(y));
	for(T z = T(1); ; x *= x)
	{
		if((n & 1) != 0) z *= x;
		if((n >>= 1) == 0) return (y < 0? T(1)/z: z);
	}
}

template<typename T> forceinline T Pow2(T x) {return Exp(x*T(0.6931472));}

template<typename T> T Erf1(T x)
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

template<typename T> T Erfc2(T x)
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

template<typename T> T Erfc3(T x)
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
	result = T((1/SqrtPI - result) / x);
	const T xfloor = Floor(x * 16) / 16;
	T del = (x - xfloor)*(x + xfloor);
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
	union {float f; uint i;};
	f = x;
	i = (0xbe6f0000 - i) >> 1;
	x*=f;
	return x*(1.5f - 0.5f*f*x);
}

inline float FastByteToNormalizedFloat(byte x)
{
	union {float f; uint i;} u;
	u.f = 32768.0f;
	u.i |= x;
	return (u.f - 32768.0f)*(256.0f/255.0f);
}


inline uint GreatestCommonDivisor(uint a, uint b)
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



//Линейная интерполяция скаляров и векторов
template<typename T, typename U> constexpr T LinearMix(T x, T y, U factor)
{return T(x*(U(1) - factor) + y*factor);}

template<typename T> constexpr T Step(T edge, T value) {return T(value >= edge);}

namespace D {
template<typename T> constexpr T SmoothStep_helper(T t) {return t*t*(T(3) - t*2);}
}

template<typename T> constexpr T SmoothStep(T edge0, T edge1, T value)
{return D::SmoothStep_helper(Clamp((value - edge0) / (edge1 - edge0), T(0), T(1)));}

namespace GLSL {

template<typename T> constexpr forceinline T min(const T& t1, const T& t2) {return Min(t1, t2);}
template<typename T> constexpr forceinline T max(const T& t1, const T& t2) {return Max(t1, t2);}

template<typename T> constexpr forceinline T sign(T x) {return Sign(x);}
template<typename T> constexpr forceinline T abs(T x) {return Abs(x);}

template<typename T> forceinline T floor(T v) {return Floor(v);}
template<typename T> forceinline T ceil(T v) {return Ceil(v);}
template<typename T> forceinline T round(T v) {return Round(v);}

template<typename T> forceinline T fract(T v) {return Fract(v);}

template<typename T> forceinline T sin(T v) {return Sin(v);}
template<typename T> forceinline T cos(T v) {return Cos(v);}
template<typename T> forceinline T tan(T v) {return Tan(v);}

template<typename T> forceinline T sinh(T v) {return Sinh(v);}
template<typename T> forceinline T cosh(T v) {return Cosh(v);}
template<typename T> forceinline T tanh(T v) {return Tanh(v);}

template<typename T> forceinline T asin(T v) {return Asin(v);}
template<typename T> forceinline T acos(T v) {return Acos(v);}
template<typename T> forceinline T atan(T v) {return Atan(v);}

template<typename T> forceinline T sqrt(T v) {return Sqrt(v);}
template<typename T> forceinline T log(T v) {return Log(v);}

template<typename T> forceinline T mod(T x, T y) {return Mod(x, y);}
template<typename T> forceinline T pow(T x, T power) {return Pow(x, power);}

template<typename T, typename N, typename X> constexpr forceinline
auto clamp(T v, N minv, X maxv) -> decltype(Clamp(v, minv, maxv)) {return Clamp(v, minv, maxv);}

template<typename T, typename U> constexpr forceinline T mix(T x, T y, U factor) {return LinearMix(x, y, factor);}

template<typename T> constexpr T step(T edge, T value) {return Step(edge, value);}
template<typename T> constexpr T smoothstep(T edge0, T edge1, T value) {return SmoothStep(edge0, edge1, value);}

}





#if INTRA_DISABLED
//Работает только для стандартных float
//Возвращает число в диапазоне [0.0; 1.0]
inline float frandom(int* seed = &g_frandomSeed)
{
	union {float asFloat; uint asInt;};
	*seed *= 16807;
	asInt = ((((uint)*seed)>>9)|0x3f800000);
	return asFloat-1.0f;
}
#endif

INTRA_EXTENDED_CONSTEXPR inline uint CeilToNextPow2(uint v)
{
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	return ++v;
}

//Логарифм целого числа по основанию 2, округлённый вниз
INTRA_EXTENDED_CONSTEXPR inline byte Log2i(uint x)
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

constexpr forceinline bool IsPow2(size_t x) {return x != 0 && ((x & (x - 1)) == 0);}

}}

INTRA_WARNING_POP
