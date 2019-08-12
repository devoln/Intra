#pragma once

#include "Core/Type.h"

// This module defines SIMD vector types that can be used on different platforms and compilers.
// Its design is based on GCC vector extensions. When compiled with GCC or Clang it is defined as a thin free function wrapper around them.
// When used with MSVC it defines classes with overloaded operators implemented via intrinsics.

// There is no emulation of float4/float8/int4/int8 operations on MSVC.
// For example if code is compiled without /arch:AVX the types float8 and int8 will not be defined:
// float4 and int4 require /arch:SSE2 on 32-bit x86
// float8 requires /arch:AVX
// int8 requires /arch:AVX2

// Warning for MSVC:
// There is a performance problem that appears when:
// 1) float4 or int4 is used in translation units with /arch flag below /arch:AVX, and
// 2) a translation unit with /arch:AVX and above uses (1), and
// 3) flags /GL (whole program optimization) and /LTCG (link time code generation) are used to compile program.
// The problem is that forceinline doesn't work, and the function defined in (1)
// becomes an order of magnitude (~10 times) slower, than the direct implementation written in intrinsics
// You can see this problem by enabling /w34714
// To avoid this problem, it is necessary to isolate AVX code in its own translation units compiled with /arch:AVX or above,
// It must not call code compiled with an /arch flag below /arch:AVX.
// In an opposite direction, calling AVX code from SSE code appears to be safe.

#if(defined(_M_AMD64) || defined(_M_X64) || defined(__amd64)) && !defined(__x86_64__)
#define __x86_64__ 1
#endif

#define INTRA_SIMD_LEVEL_NONE 0
#define INTRA_SIMD_LEVEL_SSE 1
#define INTRA_SIMD_LEVEL_SSE2 2
#define INTRA_SIMD_LEVEL_SSE3 3
#define INTRA_SIMD_LEVEL_SSSE3 4
#define INTRA_SIMD_LEVEL_SSE4_1 5
#define INTRA_SIMD_LEVEL_SSE4_2 6
#define INTRA_SIMD_LEVEL_AVX 7
#define INTRA_SIMD_LEVEL_AVX2 8


#if INTRA_PLATFORM_ARCH == INTRA_PLATFORM_X86 || INTRA_PLATFORM_ARCH == INTRA_PLATFORM_X86_64
#if !defined(INTRA_SIMD_LEVEL)
#ifdef __AVX2__
#define INTRA_SIMD_LEVEL INTRA_SIMD_LEVEL_AVX2
#elif defined(__AVX__)
#define INTRA_SIMD_LEVEL INTRA_SIMD_LEVEL_AVX
#elif defined(__SSE4_2__)
#define INTRA_SIMD_LEVEL INTRA_SIMD_LEVEL_SSE4_2
#elif defined(__SSE4_1__)
#define INTRA_SIMD_LEVEL INTRA_SIMD_LEVEL_SSE4_1
#elif defined(__SSSE3__)
#define INTRA_SIMD_LEVEL INTRA_SIMD_LEVEL_SSSE3
#elif defined(__SSE3__)
#define INTRA_SIMD_LEVEL INTRA_SIMD_LEVEL_SSE3
#elif defined(__SSE2__) || defined(__x86_64__)
#define INTRA_SIMD_LEVEL INTRA_SIMD_LEVEL_SSE2
#elif defined(__SSE__)
#define INTRA_SIMD_LEVEL INTRA_SIMD_LEVEL_SSE
#elif defined(_M_IX86_FP)

#if(_M_IX86_FP >= 2)
#define INTRA_SIMD_LEVEL INTRA_SIMD_LEVEL_SSE2
#elif(_M_IX86_FP == 1)
#define INTRA_SIMD_LEVEL INTRA_SIMD_LEVEL_SSE
#else
#define INTRA_SIMD_LEVEL INTRA_SIMD_LEVEL_NONE
#endif

#endif
#endif
#endif

#ifndef INTRA_SIMD_LEVEL
#define INTRA_SIMD_LEVEL INTRA_SIMD_LEVEL_NONE
#endif

// runtime instruction support detection
#ifdef _MSC_VER
#include <intrin.h>
#elif defined(__GNUC__)
inline void __cpuid(int* cpuinfo, int info)
{
	__asm__ __volatile__(
		"xchg %%ebx, %%edi;"
		"cpuid;"
		"xchg %%ebx, %%edi;"
		:"=a" (cpuinfo[0]), "=D" (cpuinfo[1]), "=c" (cpuinfo[2]), "=d" (cpuinfo[3])
		:"0" (info)
	);
}

inline unsigned long long _xgetbv(unsigned int index)
{
	unsigned eax, edx;
	__asm__ __volatile__(
		"xgetbv;"
		: "=a" (eax), "=d"(edx)
		: "c" (index)
	);
	return (static_cast<unsigned long long>(edx) << 32) | eax;
}
#endif



INTRA_BEGIN
namespace Simd {
template<class T> using TScalarOf = decltype(+Val<T>()[0]);
template<class T> using TIntAnalogOf = decltype(Val<T>() < Val<T>());

inline bool IsAvxSupported()
{
	int cpuinfo[4];
	__cpuid(cpuinfo, 1);
	bool supported = (cpuinfo[2] & (1 << 28)) != 0;
	bool osxsaveSupported = (cpuinfo[2] & (1 << 27)) != 0;
	if(osxsaveSupported && supported)
	{
		// _XCR_XFEATURE_ENABLED_MASK = 0
		unsigned long long xcrFeatureMask = _xgetbv(0);
		supported = (xcrFeatureMask & 0x6) == 0x6;
	}
	return supported;
}

}



#if defined(__GNUC__) || defined(__clang__)
#include "detail/VectorExtImpl.h"
#else
#include "detail/IntelIntrinsicImpl.h"
#endif

inline namespace Funal {
// Specialize Min and Max functors to work with SIMD types
#ifdef INTRA_SIMD_FLOAT4_SUPPORT
template<> struct TMin::Typed<const float4&, const float4&>
{
	INTRA_NODISCARD forceinline float4 INTRA_VECTORCALL operator()(float4 a, float4 b) const {return Simd::Min(a, b);}
};
template<> struct TMax::Typed<const float4&, const float4&>
{
	INTRA_NODISCARD forceinline float4 INTRA_VECTORCALL operator()(float4 a, float4 b) const {return Simd::Max(a, b);}
};
#endif
#ifdef INTRA_SIMD_INT4_SUPPORT
template<> struct TMin::Typed<const int4&, const int4&>
{
	INTRA_NODISCARD forceinline float4 INTRA_VECTORCALL operator()(int4 a, int4 b) const {return Simd::Min(a, b);}
};
template<> struct TMax::Typed<const int4&, const int4&>
{
	INTRA_NODISCARD forceinline float4 INTRA_VECTORCALL operator()(int4 a, int4 b) const {return Simd::Max(a, b);}
};
#endif
#ifdef INTRA_SIMD_FLOAT8_SUPPORT
template<> struct TMin::Typed<const float8&, const float8&>
{
	INTRA_NODISCARD forceinline float8 INTRA_VECTORCALL operator()(float8 a, float8 b) const {return Simd::Min(a, b);}
};
template<> struct TMax::Typed<const float8&, const float8&>
{
	INTRA_NODISCARD forceinline float8 INTRA_VECTORCALL operator()(float8 a, float8 b) const {return Simd::Max(a, b);}
};
#endif
#ifdef INTRA_SIMD_INT8_SUPPORT
template<> struct TMin::Typed<const int8&, const int8&>
{
	INTRA_NODISCARD forceinline int8 INTRA_VECTORCALL operator()(int8 a, int8 b) const {return Simd::Min(a, b);}
};
template<> struct TMax::Typed<const int8&, const int8&>
{
	INTRA_NODISCARD forceinline int8 INTRA_VECTORCALL operator()(int8 a, int8 b) const {return Simd::Max(a, b);}
};
#endif
}

namespace Simd {

#ifdef INTRA_SIMD_FLOAT4_SUPPORT
INTRA_NODISCARD constexpr forceinline index_t LengthOf(Simd::float4) {return 4;}
#endif
#ifdef INTRA_SIMD_INT4_SUPPORT
INTRA_NODISCARD constexpr forceinline index_t LengthOf(Simd::int4) {return 4;}
#endif
#ifdef INTRA_SIMD_FLOAT8_SUPPORT
INTRA_NODISCARD constexpr forceinline index_t LengthOf(Simd::float8) {return 8;}
#endif
#ifdef INTRA_SIMD_INT8_SUPPORT
INTRA_NODISCARD constexpr forceinline index_t LengthOf(Simd::int8) {return 8;}
#endif

template<typename T> T Load(const TScalarOf<T>* src);
template<typename T> T LoadAligned(const TScalarOf<T>* src);
#ifdef INTRA_SIMD_FLOAT4_SUPPORT
template<> INTRA_NODISCARD forceinline float4 Load(const float* src) {return Load4(src);}
template<> INTRA_NODISCARD forceinline float4 LoadAligned(const float* src) {return LoadAligned4(src);}
#endif
#ifdef INTRA_SIMD_INT4_SUPPORT
template<> INTRA_NODISCARD forceinline int4 Load(const int* src) {return Load4(src);}
template<> INTRA_NODISCARD forceinline int4 LoadAligned(const int* src) {return LoadAligned4(src);}
#endif
#ifdef INTRA_SIMD_FLOAT8_SUPPORT
template<> INTRA_NODISCARD forceinline float8 Load(const float* src) {return Load8(src);}
template<> INTRA_NODISCARD forceinline float8 LoadAligned(const float* src) {return LoadAligned8(src);}
#endif
#ifdef INTRA_SIMD_INT8_SUPPORT
template<> INTRA_NODISCARD forceinline int8 Load(const int* src) {return Load8(src);}
template<> INTRA_NODISCARD forceinline int8 LoadAligned(const int* src) {return LoadAligned8(src);}
#endif

template<typename T> forceinline Requires<
	CFloatingPoint<TScalarOf<T>>,
T> INTRA_VECTORCALL Truncate(T x) noexcept
{return CastToFloat(TruncateToInt(x));}

template<typename T> forceinline Requires<
	CFloatingPoint<TScalarOf<T>>,
T> INTRA_VECTORCALL Round(T a)
{
	return CastToFloat(RoundToInt(a));
#if 0
	const float4 vNearest2 = float4(Set<int4>(1073741823));
	const float4 aTrunc = Truncate(a);
	return aTrunc + Truncate((a - aTrunc) * vNearest2);
#endif
}


#ifdef INTRA_SIMD_FLOAT4_SUPPORT

#if INTRA_SIMD_LEVEL >= INTRA_SIMD_LEVEL_SSE4_1
//TODO: intrinsic headers are included only in MSVC
forceinline float4 INTRA_VECTORCALL Floor(float4 x) {return _mm_floor_ps(x);}
forceinline float4 INTRA_VECTORCALL Ceil(float4 x) {return _mm_ceil_ps(x);}
#endif

#ifdef INTRA_SIMD_INT4_SUPPORT

#if INTRA_SIMD_LEVEL >= INTRA_SIMD_LEVEL_SSE2
forceinline int4 INTRA_VECTORCALL RoundToInt(float4 x) noexcept {return int4(_mm_cvtps_epi32(x));}
#else
forceinline int4 INTRA_VECTORCALL RoundToInt(float4 x) noexcept {return TruncateToInt(x + 0.5f) - ((x < 0) & 1);}
#endif

#if INTRA_SIMD_LEVEL < INTRA_SIMD_LEVEL_SSE4_1
forceinline float4 INTRA_VECTORCALL Floor(float4 x)
{
	const float4 fi = Truncate(x);
	const int4 igx = fi > x;
	const float4 j = float4(igx & int4(Set<float4>(1)));
	return fi - j;
}

forceinline float4 INTRA_VECTORCALL Ceil(float4 x)
{
	const float4 fi = Truncate(x);
	const int4 igx = fi < x;
	const float4 j = float4(igx & int4(Set<float4>(1)));
	return fi + j;
}
#endif

#endif

#endif

#ifdef INTRA_SIMD_FLOAT8_SUPPORT

#if INTRA_SIMD_LEVEL >= INTRA_SIMD_LEVEL_AVX

forceinline float8 INTRA_VECTORCALL Floor(float8 x) {return _mm256_floor_ps(x);}
forceinline float8 INTRA_VECTORCALL Ceil(float8 x) {return _mm256_ceil_ps(x);}

#endif

#ifdef INTRA_SIMD_INT8_SUPPORT

#if INTRA_SIMD_LEVEL >= INTRA_SIMD_LEVEL_AVX2
forceinline int8 INTRA_VECTORCALL RoundToInt(float8 x) noexcept {return int8(_mm256_cvtps_epi32(x));}
#else
forceinline int8 INTRA_VECTORCALL RoundToInt(float8 x) noexcept
{return TruncateToInt(x + Set<float8>(0.5f)) - ((x < Set<float8>(0)) & 1);}
#endif

#if INTRA_SIMD_LEVEL < INTRA_SIMD_LEVEL_AVX

forceinline float8 INTRA_VECTORCALL Floor(float8 x)
{
	const float8 fi = Truncate(x);
	const int8 igx = fi > x;
	const float8 j = float8(igx & int8(Set<float8>(1)));
	return fi - j;
}

forceinline float8 INTRA_VECTORCALL Ceil(float8 x)
{
	const float8 fi = Truncate(x);
	const int8 igx = fi < x;
	const float8 j = float8(igx & int8(Set<float8>(1)));
	return fi + j;
}

#endif

#endif

#endif


#if(defined(INTRA_SIMD_LEVEL) && defined(__FMA__))

//a + b*c
forceinline float4 INTRA_VECTORCALL MultiplyAccumulate(float4 a, float4 b, float4 c) {return _mm_fmadd_ps(b, c, a);}
forceinline float8 INTRA_VECTORCALL MultiplyAccumulate(float8 a, float8 b, float8 c) {return _mm256_fmadd_ps(b, c, a);}
forceinline float4 INTRA_VECTORCALL MultiplyAccumulate(float a, float4 b, float4 c) {return MultiplyAccumulate(Set<float4>(a), b, c);}
forceinline float8 INTRA_VECTORCALL MultiplyAccumulate(float a, float8 b, float8 c) {return MultiplyAccumulate(Set<float8>(a), b, c);}
forceinline float4 INTRA_VECTORCALL MultiplyAccumulate(float4 a, float4 b, float c) {return MultiplyAccumulate(a, b, Set<float4>(c));}
forceinline float8 INTRA_VECTORCALL MultiplyAccumulate(float8 a, float8 b, float c) {return MultiplyAccumulate(a, b, Set<float8>(c));}
forceinline float4 INTRA_VECTORCALL MultiplyAccumulate(float a, float4 b, float c) {return MultiplyAccumulate(Set<float4>(a), b, Set<float4>(c));}
forceinline float8 INTRA_VECTORCALL MultiplyAccumulate(float a, float8 b, float c) {return MultiplyAccumulate(Set<float8>(a), b, Set<float8>(c));}

#else

template<typename T1, typename T2, typename T3> forceinline auto INTRA_VECTORCALL MultiplyAccumulate(T1 a, T2 b, T3 c) {return a + b*c;}

#endif

template<typename T> forceinline Requires<
	CFloatingPoint<TScalarOf<T>>,
T> INTRA_VECTORCALL Fract(T x) {return x - Floor(x);}

template<typename T> forceinline Requires<
	CFloatingPoint<TScalarOf<T>>,
T> INTRA_VECTORCALL Mod(T a, T aDiv)
{
	return a - Floor(a / aDiv) * aDiv;
}

template<typename T> forceinline Requires<
	CSame<TScalarOf<T>, float>,
T> INTRA_VECTORCALL ModSigned(T a, T aDiv)
{
	return a - Truncate(a / aDiv) * aDiv;
}

template<typename T> forceinline Requires<
	CSame<TScalarOf<T>, float>,
T> INTRA_VECTORCALL Abs(T v) noexcept
{
	return T(IntAnalogOf<T>(v) & 0x7FFFFFFF);
}

template<typename T> forceinline Requires<
	CSame<TScalarOf<T>, float>,
T> INTRA_VECTORCALL Pow2(T x) noexcept
{
	const T fractional_part = Fract(x);

	T factor = MultiplyAccumulate(float(-8.94283890931273951763e-03), fractional_part, float(-1.89646052380707734290e-03));
	factor = MultiplyAccumulate(float(-5.58662282412822480682e-02), factor, fractional_part);
	factor = MultiplyAccumulate(float(-2.40139721982230797126e-01), factor, fractional_part);
	factor = MultiplyAccumulate(float(3.06845249656632845792e-01), factor, fractional_part);
	factor = MultiplyAccumulate(float(1.06823753710239477000e-07), factor, fractional_part);
	x -= factor;

	x *= Set<T>(float(1 << 23));
	x += Set<T>(float((1 << 23) * 127));

	return T(RoundToInt(x));
}

template<typename T> forceinline Requires<
	CSame<TScalarOf<T>, float>,
T> INTRA_VECTORCALL Exp(T x) noexcept
{
	return Pow2(x * float(1.442695040888963407359924681001892137426645954153));
}

namespace detail {

// Minimax polynomial fit of log2(x)/(x - 1), for x in range [1, 2]
template<typename T, int Order> struct Log2Polynomial;
template<typename T> struct Log2Polynomial<T, 2>
{
	static forceinline T INTRA_VECTORCALL Calc(T m) noexcept
	{
		T p = MultiplyAccumulate(-1.04913055217340124191f, m, 0.204446009836232697516f);
		return MultiplyAccumulate(2.28330284476918490682f, m, p);
	}
};
template<typename T> struct Log2Polynomial<T, 3>
{
	static forceinline T INTRA_VECTORCALL Calc(T m) noexcept
	{
		T p = MultiplyAccumulate(0.688243882994381274313f, m, -0.107254423828329604454f);
		p = MultiplyAccumulate(-1.75647175389045657003f, m, p);
		return MultiplyAccumulate(2.61761038894603480148f, m, p);
	}
};
template<typename T> struct Log2Polynomial<T, 4>
{
	static forceinline T INTRA_VECTORCALL Calc(T m) noexcept
	{
		T p = MultiplyAccumulate(-0.465725644288844778798f, m, 0.0596515482674574969533f);
		p = MultiplyAccumulate(1.48116647521213171641f, m, p);
		p = MultiplyAccumulate(-2.52074962577807006663f, m, p);
		return MultiplyAccumulate(2.8882704548164776201f, m, p);
	}
};
template<typename T> struct Log2Polynomial<T, 5>
{
	static forceinline T INTRA_VECTORCALL Calc(T m) noexcept
	{
		T p = MultiplyAccumulate(3.1821337e-1f, m, -3.4436006e-2f);
		p = MultiplyAccumulate(-1.2315303f, m, p);
		p = MultiplyAccumulate(2.5988452f, m, p);
		p = MultiplyAccumulate(-3.3241990f, m, p);
		return MultiplyAccumulate(3.1157899f, m, p);
	}
};

}

template<int Order, typename T> inline Requires<
	CSame<TScalarOf<T>, float>,
T> INTRA_VECTORCALL Log2Order(T x)
{
	T one = Set<T>(1);
	T e = CastToFloat(UnsignedRightBitShift(IntAnalogOf<T>(x) & 0x7F800000, 23) - 127);
	T m = T((IntAnalogOf<T>(x) & 0x007FFFFF) | IntAnalogOf<T>(one));
	T p = detail::Log2Polynomial<T, Order>::Calc(m);
	p *= m - one; // This effectively increases the polynomial degree by one, but ensures that log2(1) == 0
	return p + e;
}

template<int Order, typename T> inline Requires<
	CSame<TScalarOf<T>, float>,
T> INTRA_VECTORCALL LogOrder(T x)
{
	return Log2Order<Order>(x) / float(1.442695040888963407359924681001892137426645954153);
}

template<typename T> inline Requires<
	CSame<TScalarOf<T>, float>,
T> INTRA_VECTORCALL Log2(T x) {return Log2Order<5>(x);}

template<typename T> inline Requires<
	CSame<TScalarOf<T>, float>,
T> INTRA_VECTORCALL Log(T x) {return LogOrder<5>(x);}

}
INTRA_END
