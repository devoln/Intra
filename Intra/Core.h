#pragma once

#include <Cpp/Fundamental.h>
#include <Cpp/Warnings.h>
#include <Cpp/Features.h>
#include <Cpp/Intrinsics.h>
#include <Meta/Type.h>
#include <Utils/Span.h>
#include <Math/Math.h>
#include <Math/SineRange.h>

#ifndef INTRA_BEGIN
#define INTRA_BEGIN
#define INTRA_END
#endif

#ifndef INTRA_FORCEINLINE
#define INTRA_FORCEINLINE forceinline
#endif
#ifndef INTRA_UTIL_INLINE
#define INTRA_UTIL_INLINE INTRA_FORCEINLINE
#endif

namespace Intra {
using int8 = sbyte;
using uint8 = byte;
using int16 = short;
using uint16 = ushort;
using int32 = int;
using uint32 = uint;
using int64 = long64;
using uint64 = ulong64;

using Cpp::Move;
using Cpp::Forward;

using namespace Math;

namespace Math {
constexpr const double Sqrt2 = 1.4142135623730950488;
}
}

namespace Intra {
// Portable SIMD's compiler and scalar-type primitives belong to the main
// Intra core, so consumers do not need a second Core.h include root.
#if defined(__GNUC__) || defined(__clang__)
#ifndef INTRA_GNU_EXTENSION_SUPPORT
#define INTRA_GNU_EXTENSION_SUPPORT
#endif
#ifndef INTRA_MAY_ALIAS
#define INTRA_MAY_ALIAS __attribute__((__may_alias__))
#endif
#ifndef INTRA_FORCEINLINE_LAMBDA
#define INTRA_FORCEINLINE_LAMBDA __attribute__((always_inline))
#endif
#ifndef INTRA_GNU_EXT_CODE
#define INTRA_GNU_EXT_CODE(...) __VA_ARGS__
#endif
#ifndef INTRA_NON_GNU_EXT_CODE
#define INTRA_NON_GNU_EXT_CODE(...)
#endif
#ifndef INTRA_MSVC_EXT_CODE
#define INTRA_MSVC_EXT_CODE(...)
#endif
#ifndef INTRA_TARGET_SSE2
#define INTRA_TARGET_SSE2 __attribute__((__target__("sse2")))
#endif
#ifndef INTRA_TARGET_SSE3
#define INTRA_TARGET_SSE3 __attribute__((__target__("sse3")))
#endif
#ifndef INTRA_TARGET_SSSE3
#define INTRA_TARGET_SSSE3 __attribute__((__target__("ssse3")))
#endif
#ifndef INTRA_TARGET_SSE41
#define INTRA_TARGET_SSE41 __attribute__((__target__("sse4.1")))
#endif
#ifndef INTRA_TARGET_SSE42
#define INTRA_TARGET_SSE42 __attribute__((__target__("sse4.2")))
#endif
#ifndef INTRA_TARGET_AVX
#define INTRA_TARGET_AVX __attribute__((__target__("avx")))
#endif
#ifndef INTRA_TARGET_AVX2
#define INTRA_TARGET_AVX2 __attribute__((__target__("avx2")))
#endif
#ifndef INTRA_OPTIMIZE_FUNCTION
#if defined(__clang__)
#define INTRA_OPTIMIZE_FUNCTION(...) __VA_ARGS__
#define INTRA_OPTIMIZE_FUNCTION_END
#else
#define INTRA_OPTIMIZE_FUNCTION(...) __VA_ARGS__ __attribute__((flatten, optimize("O3")))
#define INTRA_OPTIMIZE_FUNCTION_END
#endif
#endif
#ifndef INTRA_NO_VECTORIZE_FUNC
#if defined(__GNUC__) && !defined(__clang__)
#define INTRA_NO_VECTORIZE_FUNC __attribute__((optimize("no-tree-vectorize")))
#else
#define INTRA_NO_VECTORIZE_FUNC
#endif
#endif
#else
#ifndef INTRA_MAY_ALIAS
#define INTRA_MAY_ALIAS
#endif
#ifndef INTRA_GNU_EXT_CODE
#define INTRA_GNU_EXT_CODE(...)
#endif
#ifndef INTRA_NON_GNU_EXT_CODE
#define INTRA_NON_GNU_EXT_CODE(...) __VA_ARGS__
#endif
#ifndef INTRA_MSVC_EXT_CODE
#define INTRA_MSVC_EXT_CODE(...) __VA_ARGS__
#endif
#ifndef INTRA_TARGET_SSE2
#define INTRA_TARGET_SSE2
#endif
#ifndef INTRA_TARGET_SSE3
#define INTRA_TARGET_SSE3
#endif
#ifndef INTRA_TARGET_SSSE3
#define INTRA_TARGET_SSSE3
#endif
#ifndef INTRA_TARGET_SSE41
#define INTRA_TARGET_SSE41
#endif
#ifndef INTRA_TARGET_SSE42
#define INTRA_TARGET_SSE42
#endif
#ifndef INTRA_TARGET_AVX
#define INTRA_TARGET_AVX
#endif
#ifndef INTRA_TARGET_AVX2
#define INTRA_TARGET_AVX2
#endif
#ifndef INTRA_OPTIMIZE_FUNCTION
#define INTRA_OPTIMIZE_FUNCTION(...) __VA_ARGS__
#define INTRA_OPTIMIZE_FUNCTION_END
#endif
#ifndef INTRA_NO_VECTORIZE_FUNC
#define INTRA_NO_VECTORIZE_FUNC
#endif
#endif

struct TUndefined {};
constexpr TUndefined Undefined{};
#ifndef INTRA_MACRO_SIMPLE_REPEAT
#ifndef INTRA_MACRO_ARGUMENT
#define INTRA_MACRO_ARGUMENT(...) __VA_ARGS__
#endif
#define INTRAZ_D_MACRO_SIMPLE_REPEAT_1(s, f) f
#define INTRAZ_D_MACRO_SIMPLE_REPEAT_2(s, f) \
	INTRAZ_D_MACRO_SIMPLE_REPEAT_1(s, f) INTRA_MACRO_ARGUMENT s INTRAZ_D_MACRO_SIMPLE_REPEAT_1(s, f)
#define INTRAZ_D_MACRO_SIMPLE_REPEAT_4(s, f) \
	INTRAZ_D_MACRO_SIMPLE_REPEAT_2(s, f) INTRA_MACRO_ARGUMENT s INTRAZ_D_MACRO_SIMPLE_REPEAT_2(s, f)
#define INTRAZ_D_MACRO_SIMPLE_REPEAT_8(s, f) \
	INTRAZ_D_MACRO_SIMPLE_REPEAT_4(s, f) INTRA_MACRO_ARGUMENT s INTRAZ_D_MACRO_SIMPLE_REPEAT_4(s, f)
#define INTRAZ_D_MACRO_SIMPLE_REPEAT_16(s, f) \
	INTRAZ_D_MACRO_SIMPLE_REPEAT_8(s, f) INTRA_MACRO_ARGUMENT s INTRAZ_D_MACRO_SIMPLE_REPEAT_8(s, f)
#define INTRAZ_D_MACRO_SIMPLE_REPEAT_32(s, f) \
	INTRAZ_D_MACRO_SIMPLE_REPEAT_16(s, f) INTRA_MACRO_ARGUMENT s INTRAZ_D_MACRO_SIMPLE_REPEAT_16(s, f)
#define INTRAZ_D_MACRO_SIMPLE_REPEAT_64(s, f) \
	INTRAZ_D_MACRO_SIMPLE_REPEAT_32(s, f) INTRA_MACRO_ARGUMENT s INTRAZ_D_MACRO_SIMPLE_REPEAT_32(s, f)
#define INTRA_MACRO_SIMPLE_REPEAT(n, f, separator) INTRAZ_D_MACRO_SIMPLE_REPEAT_##n(separator, f)
#endif
template<typename T, typename... U> concept CSame = (__is_same(T, U) && ...);
template<typename T, typename... U> concept CAnyOf = (CSame<T, U> || ...);
template<typename T> concept CBasicIntegral = __is_integral(T);
template<typename T> concept CBasicFloatingPoint = __is_floating_point(T);
template<typename T> concept CBasicSignedIntegral = CBasicIntegral<T> && (T(-1) < T(0));
template<typename T> concept CBasicUnsignedIntegral = CBasicIntegral<T> && !CBasicSignedIntegral<T>;
template<typename T> concept CBasicSigned = CBasicSignedIntegral<T> || CBasicFloatingPoint<T>;
template<typename T> concept CBasicArithmetic = CBasicIntegral<T> || CBasicFloatingPoint<T>;
template<typename T> concept CChar = CAnyOf<T, char, signed char, unsigned char, char8_t, char16_t, char32_t, wchar_t>;
template<typename T> concept CNumber = CBasicArithmetic<T>;
template<typename T> concept CDefined = !CSame<T, TUndefined>;
template<typename T> concept CFalse = false;
template<typename F, typename... Args> concept CCallable = requires(F&& f, Args&&... args) {
	static_cast<F&&>(f)(static_cast<Args&&>(args)...);
};
template<typename T, typename... U> concept CSameSize = ((sizeof(T) == sizeof(U)) && ...);

namespace z_D {
template<typename T> struct TScalarOf_ {using _ = T;};
template<typename T, bool = CBasicArithmetic<T>> struct TToIntegral_ {};
template<typename T> struct TToIntegralScalar_ {using _ = T;};
template<> struct TToIntegralScalar_<float> {using _ = int32;};
template<> struct TToIntegralScalar_<double> {using _ = int64;};
template<> struct TToIntegralScalar_<long double> {using _ = int64;};
template<typename T> struct TToIntegral_<T, true>: TToIntegralScalar_<T> {};
}
template<size_t... Is> struct TIndexSeq {};
namespace z_D {
template<size_t N, size_t... Is> struct TMakeIndexSeq_: TMakeIndexSeq_<N - 1, N - 1, Is...> {};
template<size_t... Is> struct TMakeIndexSeq_<0, Is...> {using _ = TIndexSeq<Is...>;};
}
template<typename T> using TScalarOf = typename z_D::TScalarOf_<T>::_;
template<CBasicArithmetic T> using TToIntegral = typename z_D::TToIntegral_<T>::_;

template<typename T> struct TToSigned_ {using _ = T;};
template<> struct TToSigned_<uint8> {using _ = int8;};
template<> struct TToSigned_<uint16> {using _ = int16;};
template<> struct TToSigned_<uint32> {using _ = int32;};
template<> struct TToSigned_<uint64> {using _ = int64;};
template<typename T> using TToSigned = typename TToSigned_<T>::_;
template<typename T> using TExplicitType = T;

template<size_t N> using TMakeIndexSeq = typename z_D::TMakeIndexSeq_<N>::_;

template<typename T> constexpr T MaxValueOf = T((~uint64(0)) >> (64 - sizeof(T) * 8 + CBasicSigned<T>));
template<typename T> constexpr T MaxStepInRange = [] {
	if constexpr(CBasicIntegral<T>) return T(1);
	else if constexpr(CSame<T, float>) return T(5.960464477539063e-8f);
	else return T(1.1102230246251565e-16);
}();
template<typename T> constexpr T IntegerRangeMax = [] {
	if constexpr(CBasicIntegral<T>) return MaxValueOf<T>;
	else if constexpr(CSame<T, float>) return T(16777216.0f);
	else return T(9007199254740992.0);
}();
}

template<bool Cond, typename T = void> using Requires = Intra::Meta::EnableIf<Cond, T>;

using namespace Intra;
using namespace Intra::Range;
