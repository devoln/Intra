#pragma once

#include "Core/FundamentalTypes.h"
#include "Platform/CppWarnings.h"
#include "Platform/CppFeatures.h"

namespace Intra { namespace Math {

constexpr const double PI = 3.14159265358979323846;
constexpr const double E = 2.71828182845904523536;

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#ifdef _MSC_VER
#pragma warning(disable: 4056)
const float Infinity = 1e300*1e300;
#else
constexpr const float Infinity = __builtin_huge_valf();
#endif

//Объявление новых типов заранее
template<typename T> struct Plane;
template<typename T> struct Triangle;
template<typename T> struct Matrix3;
template<typename T> struct Matrix4;
template<typename T> struct Vector2;
template<typename T> struct Vector3;
template<typename T> struct Vector4;
template<typename T> struct Quaternion;

struct NaNType
{
	NaNType() {}

	bool operator==(float rhs) const;
	bool operator==(double rhs) const;
	bool operator==(real rhs) const;

	bool operator!=(float rhs) const {return !operator==(rhs);}
	bool operator!=(double rhs) const {return !operator==(rhs);}
	bool operator!=(real rhs) const {return !operator==(double(rhs));}

	operator float() const {return float(Infinity/Infinity);}
	operator double() const {return double(Infinity/Infinity);}
	operator real() const {return real(Infinity/Infinity);}

	operator long64() const {return 0;}
	operator int() const {return 0;}
	operator short() const {return 0;}
	operator sbyte() const {return 0;}

	operator ulong64() const {return 0;}
	operator uint() const {return 0;}
	operator ushort() const {return 0;}
	operator byte() const {return 0;}
} const NaN;

inline bool operator==(float l, NaNType) {return NaN==l;}
inline bool operator!=(float l, NaNType) {return NaN!=l;}
inline bool operator==(double l, NaNType) {return NaN==l;}
inline bool operator!=(double l, NaNType) {return NaN!=l;}
inline bool operator==(real l, NaNType) {return NaN==l;}
inline bool operator!=(real l, NaNType) {return NaN!=l;}


#ifndef NOMINMAX
#define NOMINMAX 1
#endif

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

template<typename T> T Sqr(T n) {return n*n;}
template<typename T> T Abs(T v) {return v>=0? v: -v;}

float Exp(float v);
double Exp(double v);

template<typename T> constexpr forceinline T Min(const T& t1, const T& t2) {return t1<t2? t1: t2;}
template<typename T> constexpr forceinline T Max(const T& t1, const T& t2) {return t1<t2? t2: t1;}

template<typename T> constexpr int ISign(T x) {return (x>0)-(x<0);}
template<typename T> constexpr T Sign(T x) {return T(ISign(x));}

template<typename T, typename N, typename X>
auto Clamp(T v, N minv, X maxv) -> decltype(Max(minv, Min(maxv, v))) {return Max(minv, Min(maxv, v));}

inline int IFloor(float x) {return int(x)-int(x<0);}
inline intptr IFloor(double x) {return intptr(x)-intptr(x<0);}

#ifndef INTRA_INLINE_MATH

float Floor(float v);
double Floor(double v);
float Ceil(float v);
double Ceil(double v);
float Round(float v);
double Round(double v);


inline float Fract(float x) {return x-Floor(x);}
inline double Fract(double x) {return x-Floor(x);}

float Sin(float v);
double Sin(double v);
float Cos(float v);
double Cos(double v);
float Tan(float v);
double Tan(double v);

float Sinh(float v);
double Sinh(double v);
float Cosh(float v);
double Cosh(double v);
float Tanh(float v);
double Tanh(double v);

float Asin(float v);
double Asin(double v);
float Acos(float v);
double Acos(double v);
float Atan(float v);
double Atan(double v);

float Atanh(float v);
double Atanh(double v);

float Sqrt(float v);
double Sqrt(double v);

float Log(float v);
double Log(double v);

float Mod(float x, float y);
double Mod(double x, double y);

float Pow(float v, float power);
double Pow(double v, double power);

#else


#if(INTRA_PLATFORM_ARCH==INTRA_PLATFORM_X86)

#if(INTRA_COMPILER_INLINE_ASM_SYNTAX == INTRA_COMPILER_INLINE_ASM_SYNTAX_INTEL)

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
		fprem1
		fstp r
		fstp y
	}
	return r;
}


/*inline double Exp(double x)
{
	double result;
	__asm
	{
		fldl2e
		fmulp
		fld st
		frndint
		fsub st(1), st
		fxch
		fchs
		f2xm1
		fld1
		faddp
		fxch
		fld1
		fscale
		fstp st(1)
		fmulp :  "=t"(result) : "0"(x)
	}
	return result;
}*/

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



template<typename T> T Log(T v)
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
}

inline float Pow(float v, float power) {return power==0.0f? 1.0f: Exp(Log(v)*power);}
inline double Pow(double v, double power) {return power==0.0? 1.0f: Exp(Log(v)*power);}

#endif

template<typename T> T Pow(T x, int y)
{
	uint n = uint(y>0? y: -y);
	for(T z = T(1); ; x*=x)
	{
		if((n & 1) != 0) z*=x;
		if((n >>= 1) == 0) return (y<0? T(1)/z: z);
	}
}



inline float FastSqrt(float x)
{
	union {float f; uint i;};
	f = x;
	i = (0xbe6f0000-i) >> 1;
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
	while(a!=b)
	{
		if(a>b) a-=b;
		else b-=a;
	}
	return a;
}

template<typename T> struct Vector3;
template<typename T> struct Vector4;

//inline float Fract(float v) {return v-Floor(v);}
//inline double Fract(double v) {return v-Floor(v);}



//Линейная интерполяция скаляров и векторов
template<typename T, typename U> T LinearMix(T x, T y, U factor) {return T(x*(U(1)-factor)+y*factor);}

template<typename T> T Step(T edge, T value) {return T(value>=edge);}

template<typename T> T SmoothStep(T edge0, T edge1, T value)
{
	const T t = Clamp((value-edge0)/(edge1-edge0), T(0), T(1));
	return t*t*(T(3)-t*2);
}

namespace GLSL {

template<typename T> constexpr forceinline T min(const T& t1, const T& t2) {return Min(t1, t2);}
template<typename T> constexpr forceinline T max(const T& t1, const T& t2) {return Max(t1, t2);}

template<typename T> constexpr forceinline T sign(T x) {return Sign(x);}

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

template<typename T, typename N, typename X>
forceinline auto clamp(T v, N minv, X maxv) -> decltype(Clamp(v, minv, maxv)) {return Clamp(v, minv, maxv);}

template<typename T, typename U> forceinline T mix(T x, T y, U factor) {return LinearMix(x, y, factor);}

template<typename T> T step(T edge, T value) {return Step(edge, value);}
template<typename T> T smoothstep(T edge0, T edge1, T value) {return SmoothStep(edge0, edge1, value);}

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

inline uint CeilToNextPow2(uint v)
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
inline byte Log2i(uint x)
{
	if(x==0) return byte(255);
	uint n = 31;
	if(x<=0x0000ffff) n -= 16, x <<= 16;
	if(x<=0x00ffffff) n -= 8, x <<= 8;
	if(x<=0x0fffffff) n -= 4, x <<= 4;
	if(x<=0x3fffffff) n -= 2, x <<= 2;
	if(x<=0x7fffffffu) n--;
	return byte(n);
}

forceinline bool IsPow2(size_t x) {return x!=0 && ((x&(x-1))==0);}

struct Half
{
	Half() = default;

	Half(ushort s): data(s) {}
	Half(float f): data(fromFloat(f)) {}
	Half(double d): data(fromFloat(float(d))) {}

	operator float() const {return toFloat(data);}
	operator double() const {return toFloat(data);}

	Half& operator=(float rhs) {data = fromFloat(rhs); return *this;}
	Half& operator=(double rhs) {data = fromFloat(float(rhs)); return *this;}
	Half& operator=(const Half& rhs) = default;

	ushort data;

private:
	static ushort fromFloat(float f)
	{
		union {float f32; uint i32;};
		f32 = f;
		return ushort( (((i32 & 0x7fffffffu) >> 13u) - (0x38000000u >> 13u)) | ((i32 & 0x80000000u) >> 16u) );
	}

	static float toFloat(ushort h)
	{
		union {float f32; int i32;};
		i32 = ((h & 0x8000) << 16) | ( ((h & 0x7fff) << 13) + 0x38000000 );
		return f32;
	}
};

INTRA_WARNING_POP

}}
