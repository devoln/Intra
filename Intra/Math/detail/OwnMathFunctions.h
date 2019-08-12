#pragma once

// This file is included inside Math/Math.h in namespace Intra::Math

// Implementation of math functions without using CRT library.
// These implementations are incomplete: they may be slow or have limited range and precision.
// May be useful to create demoscenes or other application where C runtime is not available.
// For complete implementation use #define INTRA_USE_CRT_MATH

#if(INTRA_PLATFORM_ARCH == INTRA_PLATFORM_X86)

#if(INTRA_COMPILER_INLINE_ASM_SYNTAX == INTRA_COMPILER_INLINE_ASM_SYNTAX_INTEL)

#ifdef _MSC_VER
#pragma warning(disable: 4725) //instruction may be inaccurate on some Pentiums
#endif

//TODO: use SSE instructions, intrinsics or approximate

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
		fstp a //Ќенужное значение 1.0 надо куда-то выкинуть из стека
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
INTRA_NODISCARD constexpr inline long double Floor(long double x) {return static_cast<long double>(IFloor(double(x)));}
INTRA_NODISCARD constexpr inline long double Ceil(long double x) {return Floor(x+0.999999);}
INTRA_NODISCARD constexpr inline long double Round(long double x) {return Floor(x+0.5);}

INTRA_NODISCARD constexpr inline double Floor(double x) {return double(IFloor(x));}
INTRA_NODISCARD constexpr inline double Ceil(double x) {return Floor(x+0.999999);}
INTRA_NODISCARD constexpr inline double Round(double x) {return Floor(x+0.5);}

INTRA_NODISCARD constexpr inline float Floor(float x) {return float(IFloor(x));}
INTRA_NODISCARD constexpr inline float Ceil(float x) {return Floor(x+0.999999f);}
INTRA_NODISCARD constexpr inline float Round(float x) {return Floor(x+0.5f);}

//INTRA_NODISCARD forceinline float Floor(float x) {return float(Floor(double(x)));}
//INTRA_NODISCARD forceinline float Ceil(float x) {return float(Ceil(double(x)));}
//INTRA_NODISCARD forceinline float Round(float v) {return float(Round(double(v)));}
INTRA_NODISCARD inline float Sin(float x) {return float(Sin(double(x)));}
INTRA_NODISCARD inline float Cos(float x) {return float(Cos(double(x)));}
INTRA_NODISCARD inline float Tan(float x) {return float(Tan(double(x)));}
INTRA_NODISCARD inline float Atan(float x) {return float(Atan(double(x)));}
INTRA_NODISCARD inline float Sqrt(float x) {return float(Sqrt(double(x)));}
INTRA_NODISCARD inline float Exp(float x) {return float(Exp(double(x)));}
INTRA_NODISCARD inline float Mod(float x, float y) {return float(Mod(double(x), double(y)));}


INTRA_NODISCARD inline double Acos(double x) {return Atan(Sqrt(1.0 - x*x)/x);}
INTRA_NODISCARD inline float Acos(float x) {return Atan(Sqrt(1.0f - x*x)/x);}
INTRA_NODISCARD inline double Asin(double x) {return Atan(x/Sqrt(1.0 - x*x));}
INTRA_NODISCARD inline float Asin(float x) {return Atan(x/Sqrt(1.0f - x*x));}

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

//! Logarithm for unsigned numbers < 65535
INTRA_NODISCARD INTRA_CONSTEXPR2 inline float Log(float x) {return float(FixedPointLog(uint(x * 65536)))/65536.0f;}
INTRA_NODISCARD INTRA_CONSTEXPR2 inline double Log(double x) {return double(FixedPointLog(uint(x * 65536)))/65536.0;}
INTRA_NODISCARD INTRA_CONSTEXPR2 inline long double Log(long double x) {return static_cast<long double>(FixedPointLog(uint(x * 65536)))/65536.0;}

INTRA_NODISCARD inline float Pow(float v, float power) {return power == 0.0f? 1.0f: Exp(Log(v)*power);}
INTRA_NODISCARD inline double Pow(double v, double power) {return power == 0.0? 1.0: Exp(Log(v)*power);}
