#pragma once

#include <Intra/Core.h>

namespace Intra { INTRA_BEGIN
constexpr struct
{
	double PI = 3.14159265358979323846;
	double E = 2.71828182845904523536;
	double SqrtPI = 1.7724538509055160273;
	double SqrtE = 1.6487212707001281;
	double Sqrt2 = 1.4142135623730950488;
	double Log2E = 1.44269504088896340736;
	double Log10E = 0.434294481903251827651;
	double LN2 = 0.693147180559945309417;
	double LN10 = 2.30258509299404568402;
} Constants;

constexpr auto Sqr = [](const auto& a) {return a * a;};
constexpr auto IsEven = []<CBasicIntegral T>(T&& a) {return (a & 1) == 0;};
constexpr auto IsOdd = FNot(IsEven);
constexpr auto Sign = [](const auto& a) {return TRemoveConstRef<decltype(a)>(ISign(a));};

namespace z_D {
template<CNumber To, CNumber From> constexpr To NumericCastTo_(const From& x) {return static_cast<To>(x);}
}

/// NumericCastTo is useful in generic code working with numbers and SIMD types
template<typename To> constexpr auto NumericCastTo = [](const auto& x) {return z_D::NumericCastTo_<To>(x);}



constexpr auto Clamp = [](const auto& v, const auto& minv, const auto& maxv) noexcept {return Max(minv, Min(maxv, v));};

#if defined(__clang__) || defined(__GNUC__)
#define INTRAZ_D_WRAP_DECL(builtinNamePart)

#define INTRAZ_D_WRAP_BUILTIN_PART(builtinNamePart) \
	if constexpr(CSame<T, float>) return __builtin_##builtinNamePart##f(x); \
	else if constexpr(CSame<T, double>) return __builtin_##builtinNamePart(x); \
	else if constexpr(CSame<T, long double>) return __builtin_##builtinNamePart##l(x);
#else

#ifndef __i386__
#define INTRAZ_D_WRAP_DECL(builtinNamePart) \
	namespace z_D { extern "C" { \
	INTRA_CRTIMP float INTRA_CRTDECL builtinNamePart##f(float); \
	INTRA_CRTIMP double INTRA_CRTDECL builtinNamePart(double); \
	}}
#define INTRAZ_D_WRAP_DECL2(builtinNamePart) \
	namespace z_D { extern "C" { \
	float INTRA_CRTDECL builtinNamePart##f(float, float); \
	double INTRA_CRTDECL builtinNamePart(double, double); \
	}}
#else
#define INTRAZ_D_WRAP_DECL(builtinNamePart) namespace z_D { extern "C" { \
	INTRA_CRTIMP double INTRA_CRTDECL builtinNamePart(double); \
	INTRA_CRTIMP inline float builtinNamePart##f(float x) {return float(builtinNamePart(x));} \
	}}
#define INTRAZ_D_WRAP_DECL2(builtinNamePart) namespace z_D { extern "C" { \
	double INTRA_CRTDECL builtinNamePart(double, double); \
	inline float builtinNamePart##f(float x, float y) {return float(builtinNamePart(x, y));} \
	}}
#endif

#define INTRAZ_D_WRAP_BUILTIN_PART(builtinNamePart) \
	if constexpr(CSame<T, float>) return builtinNamePart##f(x); \
	else if constexpr(CUnqualedBasicFloatingPoint<T>) return builtinNamePart(double(x));

/*namespace z_D { extern "C" {
	float INTRA_CRTDECL powf(float, float);
	double INTRA_CRTDECL pow(double, double);
}}*/

INTRAZ_D_WRAP_DECL2(pow);
INTRAZ_D_WRAP_DECL2(fmod);
#endif

#define INTRAZ_D_WRAP_BUILTIN(functorName, builtinNamePart) \
	INTRAZ_D_WRAP_DECL(builtinNamePart) \
	namespace z_D {template<CNumber T> auto functorName##_(T x) { \
		INTRAZ_D_WRAP_BUILTIN_PART(builtinNamePart) \
		else return functorName(TFloatOfSizeAtLeast<Min(sizeof(T), sizeof(double))>(x)); \
	}} constexpr auto functorName = [](const auto& x) noexcept {return z_D::functorName##_(x);}

INTRAZ_D_WRAP_BUILTIN(Floor, floor);
INTRAZ_D_WRAP_BUILTIN(Ceil, ceil);
INTRAZ_D_WRAP_BUILTIN(Round, round);
INTRAZ_D_WRAP_BUILTIN(Trunc, trunc);
INTRAZ_D_WRAP_BUILTIN(Sin, sin);
INTRAZ_D_WRAP_BUILTIN(Cos, cos);
INTRAZ_D_WRAP_BUILTIN(Tan, tan);
INTRAZ_D_WRAP_BUILTIN(Sinh, sinh);
INTRAZ_D_WRAP_BUILTIN(Cosh, cosh);
INTRAZ_D_WRAP_BUILTIN(Tanh, tanh);
INTRAZ_D_WRAP_BUILTIN(Asin, asin);
INTRAZ_D_WRAP_BUILTIN(Acos, acos);
INTRAZ_D_WRAP_BUILTIN(Atan, atan);
INTRAZ_D_WRAP_BUILTIN(Asinh, asinh);
INTRAZ_D_WRAP_BUILTIN(Acosh, acosh);
INTRAZ_D_WRAP_BUILTIN(Atanh, atanh);
INTRAZ_D_WRAP_BUILTIN(Sqrt, sqrt);
INTRAZ_D_WRAP_BUILTIN(Log, log);
INTRAZ_D_WRAP_BUILTIN(Log2, log2);
INTRAZ_D_WRAP_BUILTIN(Exp, exp);
INTRAZ_D_WRAP_BUILTIN(Exp2, exp2);
INTRAZ_D_WRAP_BUILTIN(Erf, erf);
INTRAZ_D_WRAP_BUILTIN(Erfc, erfc);

#undef INTRAZ_D_WRAP_BUILTIN
#undef INTRAZ_D_WRAP_DECL
#undef INTRAZ_D_WRAP_BUILTIN_PART

constexpr auto Fract = [](const auto& x) {return x - Floor(x);};

namespace z_D {
template<typename T> struct TScalarOf_ {using _ = T;};
}
/// Maps SimdVector<T, N> to T. Leaves other types unchanged.
template<typename T> using TScalarOf = typename z_D::TScalarOf_<T>::_;

constexpr auto Abs = [](const auto& x) {
	using T = TRemoveConstRef<decltype(x)>;
	if constexpr(!CBasicSigned<TScalarOf<T>>) return x;
#if defined(__clang__) || defined(__GNUC__) //better code without -ffast-math
	else if constexpr(CBasicFloatingPoint<T>)
	{
		if(!IsConstantEvaluated(x)) return T(__builtin_fabsl(x));
	}
#endif
	else if constexpr(CBasicFloatingPoint<TScalarOf<T>>)
	{
		if(!IsConstantEvaluated(x)) return BitCastTo<T>(
			BitCastTo<TToIntegral<T>>(x) & (SignBitMaskOf<TToIntegral<TScalarOf<T>>> - 1));
	}
	if constexpr(CConvertibleTo<decltype(x < 0), bool>) return x < 0? -x: x;
};



constexpr auto Mod = [](const auto& x, const auto& y) {
	using T = TCommon<decltype(x), decltype(y)>;
	if constexpr(CHasOpMod<T>) return x % y;
	else if(!CNumber<T> || IsConstantEvaluated()) return x - Trunc(x / y) * y;
#if defined(__GNUC__) || defined(__clang__)
	else if constexpr(CSame<T, float>) return __builtin_fmodf(x, y);
	else if constexpr(CSame<T, double>) return __builtin_fmod(x, y);
	else if constexpr(CSame<T, long double>) return __builtin_fmodl(x, y);
#else
	else if constexpr(CSame<T, float>) return z_D::fmodf(x, y);
	else if constexpr(CBasicFloatingPoint<T>) return T(z_D::fmod(double(x), double(y)));
#endif
	else if constexpr(CNumber<T>)
	{
		using T2 = TFloatOfSizeAtLeast<Min(sizeof(T), sizeof(double))>;
		return Mod(T2(x), T2(y));
	}
};

/// Linear interpolation
template<typename T, typename U>
[[nodiscard]] constexpr T LinearMix(T x, T y, U factor) {return T(x*(U(1) - factor) + y*factor);}

template<typename T> [[nodiscard]] constexpr T SmoothStep(T edge0, T edge1, T value)
{
	const T t = Clamp((value - edge0) / (edge1 - edge0), T(0), T(1));
	return t*t*(T(3) - t*2);
}





} INTRA_END
