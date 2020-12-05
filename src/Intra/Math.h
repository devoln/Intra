#pragma once

#include "Intra/Numeric.h"
#include "Intra/Functional.h"

INTRA_BEGIN
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

namespace z_D {
template<typename T> constexpr T PowInt(T x, int y)
{
	unsigned n = unsigned(Abs(y));
	for(T z = 1; ; x *= x)
	{
		if((n & 1) != 0) z *= x;
		if((n >>= 1) == 0) return (y < 0? T(1)/z: z);
	}
}

template<typename To, typename From> requires CNumber<To> && CNumber<From>
constexpr To NumericCastTo_(const From& x) {return static_cast<To>(x);}
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
INTRA_CRTIMP float INTRA_CRTDECL builtinNamePart##f(float, float); \
INTRA_CRTIMP double INTRA_CRTDECL builtinNamePart(double, double); \
}}
#else
#define INTRAZ_D_WRAP_DECL(builtinNamePart) \
namespace z_D { extern "C" { \
INTRA_CRTIMP double INTRA_CRTDECL builtinNamePart(double); \
inline float builtinNamePart##f(float x) {return float(builtinNamePart(x));}
}}
#define INTRAZ_D_WRAP_DECL2(builtinNamePart) \
namespace z_D { extern "C" { \
INTRA_CRTIMP double INTRA_CRTDECL builtinNamePart(double, double); \
inline float builtinNamePart##f(float x, float y) {return float(builtinNamePart(x, y));}
}}
#endif

#define INTRAZ_D_WRAP_BUILTIN_PART(builtinNamePart) \
	if constexpr(CSame<T, float>) return z_D::builtinNamePart##f(x); \
	else if constexpr(CUnqualedFloatingPoint<T>) return z_D::builtinNamePart(double(x));

namespace z_D { extern "C" {
INTRAZ_D_WRAP_DECL2(pow);
INTRAZ_D_WRAP_DECL2(fmod);
}}
#endif

#define INTRAZ_D_WRAP_BUILTIN(functorName, builtinNamePart) \
INTRAZ_D_WRAP_DECL(builtinNamePart) \
namespace z_D {template<typename T> requires CNumber<T> auto functorName##_(T x) { \
	INTRAZ_D_WRAP_BUILTIN_PART(builtinNamePart) \
	else return functorName(TFloatMin<Min(sizeof(T), sizeof(double))>(x)); \
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

#undef INTRAZ_D_WRAP_BUILTIN
#undef INTRAZ_D_WRAP_DECL
#undef INTRAZ_D_WRAP_BUILTIN_PART

constexpr auto Fract = [](const auto& x) {return x - Floor(x);};
constexpr auto Pow2 = [](const auto& x) {return Exp(x*TRemoveConstRef<decltype(x)>(Constants.LN2));};

template<typename T> requires CIntegral<T>
[[nodiscard]] constexpr uint32 GreatestCommonDivisor(T a, T b)
{
	while(a != b)
	{
		if(a > b) a -= b;
		else b -= a;
	}
	return a;
}



namespace z_D {
template<typename T> struct TScalarOf_ {using _ = T;};
}
/// Maps SimdVector<T, N> to T. Leaves other types unchanged.
template<typename T> using TScalarOf = typename TScalarOf_::_;

constexpr auto Abs = [](const auto& x) {
	using T = TRemoveConstRef<decltype(x)>;
	if constexpr(!CSigned<TScalarOf<T>>) return x;
#if defined(__clang__) || defined(__GNUC__) //better code without -ffast-math
	else if constexpr(CFloatingPoint<T>)
	{
		if(!IsConstantEvaluated(x)) return T(__builtin_fabsl(x));
	}
#endif
	else if constexpr(CFloatingPoint<TScalarOf<T>>)
	{
		if(!IsConstantEvaluated(x)) return BitCastTo<T>(
			BitCastTo<TToIntegral<T>>(x) & (SignBitMaskOf<TToIntegral<TScalarOf<T>>> - 1));
	}
	if constexpr(CConvertibleTo<decltype(x < 0), bool>) return x < 0? -x: x;
};

template<int ApproxOrder> constexpr auto Log2Approx = [](const auto& x) requires(CReal<TScalarOf<T>>)
{
	static_assert(2 <= ApproxOrder && ApproxOrder <= 5);

	using T = TRemoveConstRef<<decltype(x)>;
	using S = TScalarOf<T>;
	using SInt = TToIntegral<S>;
	using TInt = TToIntegral<T>;
	constexpr auto exponentMask = (1 << ExponentLenOf<S>) - 1;
	constexpr auto mantissaMask = (SInt(1) << NumMantissaExplicitBitsOf<S>) - 1;

	T m = BitCastTo<T>((BitCastTo<TInt>(x) & mantissaMask) | 1);

	// Minimax polynomial fit of log2(x)/(x - 1), for x in range [1, 2]
	T p;
	if constexpr(ApproxOrder == 2)
	{
		p = S(-1.04913055217340124191) + S(0.204446009836232697516f)*m;
		p = S(2.28330284476918490682) + p*m;
	}
	else if constexpr(ApproxOrder == 3)
	{
		p = S(0.688243882994381274313) + S(-0.107254423828329604454)*m;
		p = S(-1.75647175389045657003) + p*m;
		p = S(2.61761038894603480148) + p*m;
	}
	else if constexpr(ApproxOrder == 4)
	{
		p = S(-0.465725644288844778798) + S(0.0596515482674574969533)*m;
		p = S(1.48116647521213171641) + p*m;
		p = S(-2.52074962577807006663) + p*m;
		p = S(2.8882704548164776201) + p*m;
	}
	else if constexpr(ApproxOrder == 5)
	{
		p = S(3.1821337e-1) + S(-3.4436006e-2)*m;
		p = S(-1.2315303) + p*m;
		p = S(2.5988452) + p*m;
		p = S(-3.324199) + p*m;
		p = S(3.1157899) + p*m;
	}

	T e = NumericCastTo<T>(((BitCastTo<TInt>(x) >> NumMantissaExplicitBitsOf<S>) & exponentMask) - ExponentBiasOf<S>);
	p *= m - 1; // This effectively increases the polynomial degree by one, but ensures that log2(1) == 0
	return p + e;
};

template<int ApproxOrder> constexpr auto LogApprox = [](const auto& x) requires(CReal<TScalarOf<T>>)
{
	return Log2Approx<ApproxOrder>(x) * TScalarOf<T>(Constants.LN2);
};

template<typename T> requires CSame<TScalarOf<T>, float>
INTRA_FORCEINLINE T INTRA_VECTORCALL Pow2(T x) noexcept
{
	const T fractionalPart = Fract(x);

	T factor = float(-8.94283890931273951763e-03) + fractionalPart*float(-1.89646052380707734290e-03);
	factor = float(-5.58662282412822480682e-02) + factor*fractionalPart;
	factor = float(-2.40139721982230797126e-01) + factor*fractionalPart;
	factor = float(3.06845249656632845792e-01) + factor*fractionalPart;
	factor = float(1.06823753710239477000e-07) + factor*fractionalPart;
	x -= factor;

	x *= float(1 << 23);
	x += float((1 << 23) * 127);

	return T(SimdCastTo<TToIntegral<T>>(x)); //TODO: not truncate, but round to int
}

template<typename T> requires CSame<TScalarOf<T>, float>
INTRA_FORCEINLINE T INTRA_VECTORCALL Exp(T x) noexcept
{
	return Pow2(x * float(1/Constants.LN2));
}

constexpr auto Mod = [](const auto& x, const auto& y) noexcept {
	using T = TCommon<decltype(x), decltype(y)>;
	if constexpr(CModDivisible<T>) return x % y;
	else if(!CNumber<T> || IsConstantEvaluated(x, y)) return x - Trunc(x / y) * y;
#if defined(__GNUC__) || defined(__clang__)
	else if constexpr(CSame<T, float>) return __builtin_fmodf(x, y);
	else if constexpr(CSame<T, double>) return __builtin_fmod(x, y);
	else if constexpr(CSame<T, long double>) return __builtin_fmodl(x, y);
#else
	else if constexpr(CSame<T, float>) return z_D::fmodf(x, y);
	else if constexpr(CFloatingPoint<T>) return T(z_D::fmod(double(x), double(y)));
#endif
	else if constexpr(CNumber<T>)
	{
		using T2 = TFloatMin<Min(sizeof(T), sizeof(double))>;
		return Mod(T2(x), T2(y));
	}
};

constexpr auto Pow = [](const auto& x, const auto& power) noexcept {
	using T = TRemoveConstRef<decltype(x)>;
#if defined(__GNUC__) || defined(__clang__)
	if constexpr(CIntegral<decltype(power)>)
	{
		if constexpr(CSame<T, float>) return __builtin_powif(x, int(power));
		else if constexpr(CSame<T, double>) return __builtin_powi(x, int(power));
		else if constexpr(CSame<T, long double>) return __builtin_powil(x, int(power));
	}
	else if constexpr(CSame<T, float>) return __builtin_powf(x, T(power));
	else if constexpr(CSame<T, double>) return __builtin_pow(x, T(power));
	else if constexpr(CSame<T, long double>) return __builtin_powl(x, T(power));
#else
	if constexpr(CIntegral<decltype(power)>) return z_D::PowInt(x, int(power));
	else if constexpr(CSame<T, float>) return z_D::powf(x, T(power));
	else if constexpr(CFloatingPoint<T>) T(return z_D::pow(double(x), double(power)));
#endif
	else return Pow(TFloatMin<Min(sizeof(T), sizeof(double))>(x), T(power));
};

template<typename T> requires CFloatingPoint<TScalarOf<T>>
	[[nodiscard]] INTRA_FORCEINLINE T INTRA_VECTORCALL Mod(T a, T aDiv) {return a - Floor(a / aDiv) * aDiv;}

	template<typename T> requires CSame<TScalarOf<T>, float>
		[[nodiscard]] INTRA_FORCEINLINE T INTRA_VECTORCALL ModSigned(T a, T aDiv) {return ;}


INTRA_END
