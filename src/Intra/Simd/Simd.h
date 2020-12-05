#pragma once

#include "Intra/Preprocessor.h"
#include "Intra/Math.h"

// This module defines SIMD vector types that can be used on different platforms and compilers.
// Its design is based on GCC vector extensions. When compiled with GCC or Clang it is defined as a thin free function wrapper around them.
// When used with MSVC it defines a class with overloaded operators mimicking GCC vector extensions implemented via intrinsics.

// Warning for MSVC:
// There is a performance problem that appears when:
// 1) float4 or int4 is used in translation units with /arch flag below /arch:AVX, and
// 2) a translation unit with /arch:AVX and above uses (1), and
// 3) flags /GL (whole program optimization) and /LTCG (link time code generation) are used to compile program.
// The problem is that inline doesn't work, and the function defined in (1)
// becomes an order of magnitude (~10 times) slower, than the direct implementation written in intrinsics
// You can see this problem by enabling /w34714
// To avoid this problem, it is necessary to isolate AVX code in its own translation units compiled with /arch:AVX or above,
// It must not call code compiled with an /arch flag below /arch:AVX.
// In the opposite direction, calling AVX code from SSE code appears to be safe.

#ifndef INTRA_CONFIG_USE_VECTOR_EXTENSIONS
#if defined(__GNUC__) || defined(__clang__)
#define INTRA_CONFIG_USE_VECTOR_EXTENSIONS 1
#else
#define INTRA_CONFIG_USE_VECTOR_EXTENSIONS 0
#endif
#endif

#ifndef __SSE__
#if defined(_M_IX86_FP) && _M_IX86_FP >= 2 || defined(__amd64__)
#define __SSE2__ 1
#define __SSE__ 1
#elif defined(_M_IX86_FP) && _M_IX86_FP == 1
#define __SSE__ 1
#endif
#ifdef __AVX__
#define __SSE3__ 1
#define __SSSE3__ 1
#define __SSE4_1__ 1
#define __SSE4_2__ 1
#endif
#endif


INTRA_BEGIN
namespace Config {
constexpr bool TargetHasSSE =
#ifdef __SSE__
true;
#else
false;
#endif

constexpr bool TargetHasSSE2 =
#ifdef __SSE2__
true;
#else
false;
#endif

constexpr bool TargetHasSSE3 =
#ifdef __SSE3__
true;
#else
false;
#endif

constexpr bool TargetHasSSSE3 =
#ifdef __SSSE3__
true;
#else
false;
#endif

constexpr bool TargetHasSSE41 =
#ifdef __SSE4_1__
true;
#else
false;
#endif

constexpr bool TargetHasSSE42 =
#ifdef __SSE4_2__
true;
#else
false;
#endif

constexpr bool TargetHasAVX =
#ifdef __AVX__
true;
#else
false;
#endif

constexpr bool TargetHasAVX2 =
#ifdef __AVX2__
true;
#else
false;
#endif

constexpr bool TargetHasAVX512F =
#ifdef __AVX512F__
true;
#else
false;
#endif

constexpr bool TargetHasAVX512CD =
#ifdef __AVX512CD__
true;
#else
false;
#endif

constexpr bool TargetHasAVX512ER =
#ifdef __AVX512ER__
true;
#else
false;
#endif

constexpr bool TargetHasAVX512PF =
#ifdef __AVX512PF__
true;
#else
false;
#endif

constexpr bool TargetHasAVX512BW =
#ifdef __AVX512BW__
true;
#else
false;
#endif

constexpr bool TargetHasAVX512DQ =
#ifdef __AVX512DQ__
true;
#else
false;
#endif

constexpr bool TargetHasAVX512VL =
#ifdef __AVX512VL__
true;
#else
false;
#endif

constexpr bool TargetHasNEON =
#ifdef __ARM_NEON
true;
#else
false;
#endif

template<typename T> constexpr size_t TargetMaxSimdLength = (TargetHasAVX512F? 64:
	(CIntegral<T>? TargetHasAVX2: TargetHasAVX)? 32:
	(TargetHasNEON || (CIntegral<T>? TargetHasSSE: TargetHasSSE2))? 16: sizeof(T))/sizeof(T);
}
INTRA_END

#if !defined(__FMA__) && defined(__AVX2__) && !defined(__GNUC__) && !defined(__clang__)
#define __FMA__ 1
#endif

#ifndef INTRA_GNU_EXTENSION_SUPPORT
#include "detail/SimdVectorMSVC.h"
#endif

INTRA_BEGIN
namespace z_D {
#if INTRA_CONFIG_USE_VECTOR_EXTENSIONS && defined(__SSE__)
using __m128 = float INTRA_MAY_ALIAS __attribute__((__vector_size__(16)));
using __m128i = int64 INTRA_MAY_ALIAS __attribute__((__vector_size__(16)));
using __m128d = double INTRA_MAY_ALIAS __attribute__((__vector_size__(16)));

using __m256 = float INTRA_MAY_ALIAS __attribute__((__vector_size__(32)));
using __m256i = int64 INTRA_MAY_ALIAS __attribute__((__vector_size__(32)));
using __m256d = double INTRA_MAY_ALIAS __attribute__((__vector_size__(32)));

using __m512 = float INTRA_MAY_ALIAS __attribute__((__vector_size__(64)));
using __m512i = int64 INTRA_MAY_ALIAS __attribute__((__vector_size__(64)));
using __m512d = double INTRA_MAY_ALIAS __attribute__((__vector_size__(64)));


using m128u = float INTRA_MAY_ALIAS __attribute__((__vector_size__(16), __aligned__(1)));
using m256u = float INTRA_MAY_ALIAS __attribute__((__vector_size__(32), __aligned__(1)));
using m512u = float INTRA_MAY_ALIAS __attribute__((__vector_size__(64), __aligned__(1)));
#endif

#if INTRA_CONFIG_USE_VECTOR_EXTENSIONS && !defined(__INTEL_COMPILER)
#else
#ifdef __SSE__
#define INTRAZ_D_USE_SSE_INTRINSICS
extern "C" {
	__m128 _mm_round_ps(__m128 x, int roundMode);
	__m128 _mm_round_pd(__m128 x, int roundMode);
	__m256 __cdecl _mm256_round_ps(__m256 x, int roundMode);
	__m256 __cdecl _mm256_round_pd(__m256 x, int roundMode);
	__m512 __cdecl _mm512_floor_ps(__m256 x);
	__m512 __cdecl _mm512_floor_pd(__m256 x);
	__m512 __cdecl _mm512_ceil_ps(__m256 x);
	__m512 __cdecl _mm512_ceil_pd(__m256 x);

	__m128i _mm_shuffle_epi8(__m128i, __m128i);
	__m128 _mm_shuffle_ps(__m128 a, __m128 b, unsigned mask);
	__m128 _mm_shuffle_pd(__m128 a, __m128 b, unsigned mask);
	__m128i _mm_shuffle_epi32(__m128i _A, int _Imm);

	__m128 _mm_movehdup_ps(__m128);

	__m128 _mm_cvtepi32_ps(__m128i);
	__m128d _mm_cvtepi32_pd(__m128i);
	__m128i _mm_cvttps_epi32(__m128);
	__m256 __cdecl _mm256_cvtepi32_ps(__m256i);
	__m256d __cdecl _mm256_cvtepi32_pd(__m256i);
	__m256i __cdecl _mm256_cvttps_epi32(__m256);
	__m128i __cdecl _mm256_cvttpd_epi32(__m256d);
	__m256i   __cdecl _mm256_cvttpd_epi64(__m256d);
	__m512i __cdecl _mm512_cvttps_epi64(__m256);
	__m512 __cdecl _mm512_cvtepi32_ps(__m512i);
	__m512d __cdecl _mm512_cvtepi32_pd(__m512i);
	__m512i __cdecl _mm512_cvttps_epi32(__m512);
	__m256i __cdecl _mm512_cvttpd_epi32(__m512d);
	__m512i __cdecl _mm512_cvttpd_epi64(__m512d);

	__m128 _mm_movehl_ps(__m128, __m128);
	__m128i _mm_blendv_epi8(__m128i, __m128i, __m128i mask);
	__m256i __cdecl _mm256_blendv_epi8(__m256i, __m256i, __m256i);
	__m512i __cdecl _mm512_mask_blend_epi8(uint64, __m512i, __m512i);
	uint64 __cdecl _mm512_movepi8_mask(__m512i);
	__m128  __cdecl _mm256_extractf128_ps(__m256, int);
	__m128d __cdecl _mm256_extractf128_pd(__m256d, int);
	__m128i __cdecl _mm256_extractf128_si256(__m256i, int);

	__m128 _mm_unpacklo_ps(__m128 a, __m128 b);
	__m128d _mm_unpacklo_pd(__m128d a, __m128d b);
	__m128i _mm_unpacklo_epi8(__m128i a, __m128i b);
	__m128i _mm_unpacklo_epi16(__m128i a, __m128i b);
	__m128i _mm_unpacklo_epi32(__m128i a, __m128i b);
	__m128i _mm_unpacklo_epi64(__m128i a, __m128i b);

	__m128 _mm_unpackhi_ps(__m128 a, __m128 b);
	__m128d _mm_unpackhi_pd(__m128d a, __m128d b);
	__m128i _mm_unpackhi_epi8(__m128i a, __m128i b);
	__m128i _mm_unpackhi_epi16(__m128i a, __m128i b);
	__m128i _mm_unpackhi_epi32(__m128i a, __m128i b);
	__m128i _mm_unpackhi_epi64(__m128i a, __m128i b);

	__m256 __cdecl _mm256_unpacklo_ps(__m256 a, __m256 b);
	__m256d __cdecl _mm256_unpacklo_pd(__m256d a, __m256d b);
	__m256i __cdecl _mm256_unpacklo_epi8(__m256i a, __m256i b);
	__m256i __cdecl _mm256_unpacklo_epi16(__m256i a, __m256i b);
	__m256i __cdecl _mm256_unpacklo_epi32(__m256i a, __m256i b);
	__m256i __cdecl _mm256_unpacklo_epi64(__m256i a, __m256i b);

	__m256 __cdecl _mm256_unpackhi_ps(__m256 a, __m256 b);
	__m256d __cdecl _mm256_unpackhi_pd(__m256d a, __m256d b);
	__m256i __cdecl _mm256_unpackhi_epi8(__m256i a, __m256i b);
	__m256i __cdecl _mm256_unpackhi_epi16(__m256i a, __m256i b);
	__m256i __cdecl _mm256_unpackhi_epi32(__m256i a, __m256i b);
	__m256i __cdecl _mm256_unpackhi_epi64(__m256i a, __m256i b);

	__m256  __cdecl _mm256_permute2f128_ps(__m256, __m256, int);
	extern __m256d __cdecl _mm256_permute2f128_pd(__m256d, __m256d, int);
	extern __m256i __cdecl _mm256_permute2f128_si256(__m256i, __m256i, int);

	__m256  __cdecl _mm256_insertf128_ps(__m256, __m128, int);
	__m256d __cdecl _mm256_insertf128_pd(__m256d, __m128d, int);
	__m256i __cdecl _mm256_insertf128_si256(__m256i, __m128i, int);
}
#endif
#endif

}

#ifdef INTRA_GNU_EXTENSION_SUPPORT
template<typename T, size_t N> using SimdVector = Requires<
	(CIntegral<T> || CFloatingPoint<T>) &&
	!CSame<T, long double> &&
	!CChar<T>,
	T INTRA_MAY_ALIAS __attribute__((__vector_size__(sizeof(T)*N)))>;
#endif

namespace z_D {
template<typename T> TScalarOf_;
template<typename T, size_t N> TScalarOf_<SimdVector<T, N>> {using _ = T;};
template<typename T, size_t N> constexpr index_t StaticLength_<SimdVector<T, N>, 0> = N;
template<typename T, size_t N> struct TToIntegral_<SimdVector<T, N>, false> {using _ = SimdVector<TToIntegral<T>, N>;};

template<typename To, typename From> requires CSimdVector<To> && (CSimdVector<From> || CNumber<From>)
To NumericCastTo_(const From& x)
{
	if constexpr(CSimdVector<From>) return SimdCastTo<To>(x);
	else return SimdVectorFilled<TScalarOf<To>, StaticLength<To>)>(x);
}

#if defined(__clang__) && (__clang_major__ < 10 || defined(__APPLE__) && __clang_major__ < 12) || defined(__INTEL_COMPILER)
// Implement Min and Max functors for SIMD types. GCC and clang 10+ don't need this bacause they can use default generic implementation with ternary operator
template<class T, size_t N>
INTRA_FORCEINLINE auto Min_(SimdVector<T, N> a, SimdVector<T, N> b)
{
	using EquivIntVector = SimdVector<TToIntegral<T>, N>;
	return T((EquivIntVector(a < b) & EquivIntVector(a)) | (~EquivIntVector(a < b) & EquivIntVector(b)));
}

template<class T, size_t N>
INTRA_FORCEINLINE auto Max_(SimdVector<T, N> a, SimdVector<T, N> b)
{
	using EquivIntVector = SimdVector<TToIntegral<T>, N>;
	return T((EquivIntVector(a > b) & EquivIntVector(a)) | (~EquivIntVector(a > b) & EquivIntVector(b)));
}
#endif

template<typename T> constexpr bool CSimdVector_ = false;
template<typename T, size_t N> constexpr bool CSimdVector_<SimdVector<T, N>> = true;
}
template<typename T> concept CSimdVector = z_D::CSimdVector_<T>;

INTRA_OPTIMIZE_FUNCTION(template<typename T, size_t N>) INTRA_FORCEINLINE SimdVector<T, N> SimdVectorFilled(T x) noexcept
{
	if constexpr(N == 2) return SimdVector<T, N>{x, x};
	else if constexpr(N == 4) return SimdVector<T, N>{x, x, x, x};
	else if constexpr(N == 8) return SimdVector<T, N>{INTRA_MACRO_SIMPLE_REPEAT(8, x, (,))};
	else if constexpr(N == 16) return SimdVector<T, N>{INTRA_MACRO_SIMPLE_REPEAT(16, x, (,))};
	else if constexpr(N == 32) return SimdVector<T, N>{INTRA_MACRO_SIMPLE_REPEAT(32, x, (,))};
	else if constexpr(N == 64) return SimdVector<T, N>{INTRA_MACRO_SIMPLE_REPEAT(64, x, (,))};
}
INTRA_OPTIMIZE_FUNCTION_END

template<size_t N, typename T> INTRA_FORCEINLINE SimdVector<T, N> SimdLoadAligned(const T* ptr) noexcept
{
	// OK since SimdVector<T, N> allows aliasing
	return *reinterpret_cast<const SimdVector<T, N>*>(ptr);
}
template<typename T, size_t N> INTRA_FORCEINLINE void SimdStoreAligned(T* dst, SimdVector<T, N> v) noexcept
{
	// OK since SimdVector<T, N> allows aliasing
	*reinterpret_cast<SimdVector<T, N>*>(dst) = v;
}
template<size_t N, typename T> INTRA_FORCEINLINE SimdVector<T, N> SimdLoad(const T* ptr) noexcept
{
	using UnalignedV = TPackAt<sizeof(SimdVector<T, N>) / 16 - 1, z_D::m128u, z_D::m256u, void, z_D::m512u>;
	// OK since SimdVector<T, N> and UnalignedV allow aliasing
	const auto u = *reinterpret_cast<const UnalignedV*>(x);
	return reinterpret_cast<const V&>(u);
}
template<typename T, size_t N> INTRA_FORCEINLINE void SimdStore(T* dst, SimdVector<T, N> v) noexcept
{
	using UnalignedV = TPackAt<sizeof(SimdVector<T, N>) / 16 - 1, z_D::m128u, z_D::m256u, void, z_D::m512u>;
	// OK since SimdVector<T, N> and UnalignedV allow aliasing
	*reinterpret_cast<UnalignedV*>(dst) = reinterpret_cast<const UnalignedV&>(v);
}

template<int... Is, typename T, size_t N>
INTRA_FORCEINLINE SimdVector<T, N> SimdShuffle(SimdVector<T, N> v1, SimdVector<T, N> v2) noexcept
{
	static_assert(sizeof...(Is) == N);
	static_assert(((-1 <= Is && Is < 2*N) && ...), "Shuffle indices must be in range [-1; 2N)");
#ifdef __clang__
	return __builtin_shufflevector(v1, v2, Is...);
#elif defined(__GNUC__) && !defined(__INTEL_COMPILER)
	return __builtin_shuffle(v, SimdVector<TToIntegral<T>, N>{Max(Is, 0)...});
#elif defined(INTRAZ_D_USE_SSE_INTRINSICS)
	constexpr int Iarr[] = {Is...};
	constexpr int indices[] = {Max(Is, 0)...};

	// Classify the pattern of the indices Is
	constexpr bool isIdentity1 = [&] {
		for(int i = 0; i < N; i++)
		{
			if((Iarr[i] == i || Iarr[i] == -1) continue;
			return false;
		}
		return true;
	}();
	if constexpr(isIdentity1) return v1;
	constexpr bool isIdentity2 = [&] {
		for(int i = 0; i < N; i++)
		{
			if((Iarr[i] == i + N || Iarr[i] == -1) continue;
			return false;
		}
		return true;
	}();
	if constexpr(isIdentity2) return v2;

	if constexpr(Config::TargetMaxSimdLength<T> >= N)
	{
		constexpr auto makeShuffle1BitMask = [&](int indexOffset) {
			return Max(Iarr[0] - indexOffset, 0) |
				(Max(Iarr[1] - indexOffset, 0) << 1);
		};
		constexpr auto makeShuffle2BitMask = [&](int indexOffset) {
			return Max(Iarr[0] - indexOffset, 0) |
				(Max(Iarr[1] - indexOffset, 0) << 2) |
				(Max(Iarr[2] - indexOffset, 0) << 4) |
				(Max(Iarr[3] - indexOffset, 0) << 6);
		};
		constexpr auto make1BitShuffle2BitMask = [&](int indexOffset) {
			return Max(Iarr[indexOffset]*2 - indexOffset, 0) |
				(Max(Iarr[indexOffset]*2 + 1 - indexOffset, 0) << 2) |
				(Max(Iarr[indexOffset+1]*2 - indexOffset, 0) << 4) |
				(Max(Iarr[indexOffset+1]*2+1 - indexOffset, 0) << 6);
		};
		constexpr bool usesOnlyFirstArgument = ((-1 <= Is && Is < N) && ...);
		if constexpr(usesOnlyFirstArgument)
		{
			if constexpr(N == 2)
			{
				if constexpr(CSame<T, double>)
				{
					if constexpr(
						(Iarr[0] == 1 || Iarr[0] == -1) &&
						(Iarr[1] == 1 || Iarr[1] == -1))
						return SimdVector<T, N>(z_D::_mm_movehl_ps(z_D::__m128(v1), z_D::__m128(v1)));
					else
					{
						constexpr int mask = makeShuffle1BitMask(0);
						return SimdVector<T, N>(z_D::_mm_shuffle_pd(v1, v1, mask));
					}
				}
				else if constexpr(sizeof(T) == sizeof(int64))
				{
					if constexpr(
						(Iarr[0] == 1 || Iarr[0] == -1) &&
						(Iarr[1] == 1 || Iarr[1] == -1))
						return SimdVector<T, N>(z_D::_mm_unpackhi_epi64(z_D::__m128i(v1), z_D::__m128i(v1)));
					else
					{
						constexpr int mask = make1BitShuffle2BitMask(0);
						return SimdVector<T, N>(z_D::_mm_shuffle_epi32(v1, mask));
					}
				}
			}
			else if constexpr(N == 4)
			{
				constexpr int mask = makeShuffle2BitMask(0);
				if constexpr(CSame<T, float>)
				{
					if constexpr(Config::TargetHasSSE3 &&
						(Iarr[0] == 1 || Iarr[0] == -1) &&
						(Iarr[1] == 1 || Iarr[1] == -1) &&
						(Iarr[2] == 3 || Iarr[2] == -1) &&
						(Iarr[3] == 3 || Iarr[3] == -1))
						return SimdVector<T, N>(z_D::_mm_movehdup_ps(v1));
					else if constexpr(
						(Iarr[0] == 2 || Iarr[0] == -1) &&
						(Iarr[1] == 3 || Iarr[1] == -1) &&
						(Iarr[2] == 2 || Iarr[2] == -1) &&
						(Iarr[3] == 3 || Iarr[3] == -1))
						return SimdVector<T, N>(z_D::_mm_movehl_ps(v1, v1));
					else return SimdVector<T, N>(z_D::_mm_shuffle_ps(v1, v1, mask));
				}
				else if constexpr(CAnyOf<T, int32, uint32>)
				{
					if constexpr(Iarr[0] < 2 && Iarr[1] < 2 &&
						(Iarr[2] == 2 || Iarr[2] == -1) &&
						(Iarr[3] == 3 || Iarr[3] == -1))
					{
						constexpr int mask = make1BitShuffle2BitMask(0);
						return SimdVector<T, N>(z_D::_mm_shufflelo_epi16(z_D::__m128i(v1), mask));
					}
					else if constexpr((Iarr[0] == 0 || Iarr[0] == -1) &&
						(Iarr[1] == 1 || Iarr[1] == -1) &&
						(2 <= Iarr[2] || Iarr[2] == -1) &&
						(2 <= Iarr[3] || Iarr[3] == -1))
					{
						constexpr int mask = make1BitShuffle2BitMask(N/2);
						return SimdVector<T, N>(z_D::_mm_shufflehi_epi16(z_D::__m128i(v1), mask));
					}
					else return SimdVector<T, N>(z_D::_mm_shuffle_epi32(v1, mask));
				}
			}
			else if constexpr(N == 8)
			{
				if constexpr(CAnyOf<T, int16, uint16>)
				{
					if constexpr(Iarr[0] < 4 && Iarr[1] < 4 && Iarr[2] < 4 && Iarr[3] < 4 &&
						(Iarr[4] == 4 || Iarr[4] == -1) &&
						(Iarr[5] == 5 || Iarr[5] == -1) &&
						(Iarr[6] == 6 || Iarr[6] == -1) &&
						(Iarr[7] == 7 || Iarr[7] == -1))
					{
						constexpr int mask = makeShuffle2BitMask(0);
						return SimdVector<T, N>(z_D::_mm_shufflelo_epi16(z_D::__m128i(v1), mask));
					}
					else if constexpr((Iarr[0] == 0 || Iarr[0] == -1) &&
						(Iarr[1] == 1 || Iarr[1] == -1) &&
						(Iarr[2] == 2 || Iarr[2] == -1) &&
						(Iarr[3] == 3 || Iarr[3] == -1) &&
						(4 <= Iarr[4] || Iarr[4] == -1) &&
						(4 <= Iarr[5] || Iarr[5] == -1) &&
						(4 <= Iarr[6] || Iarr[6] == -1) &&
						(4 <= Iarr[7] || Iarr[7] == -1))
					{
						constexpr int mask = makeShuffle2BitMask(N/2);
						return SimdVector<T, N>(z_D::_mm_shufflehi_epi16(z_D::__m128i(v1), mask));
					}
				}
				else if constexpr(Iarr[0] < 4 && Iarr[1] < 4 && Iarr[2] < 4 && Iarr[3] < 4 &&
					(4 <= Iarr[4] || Iarr[4] == -1) &&
					(4 <= Iarr[5] || Iarr[5] == -1) &&
					(4 <= Iarr[6] || Iarr[6] == -1) &&
					(4 <= Iarr[7] || Iarr[7] == -1))
				{
					constexpr int maskL = makeShuffle2BitMask(0);
					constexpr int maskH = makeShuffle2BitMask(N/2);
					if constexpr(CAnyOf<T, int32, uint32>)
					{
						const auto l = z_D::_mm_shuffle_epi32(z_D::_mm256_extractf128_si256(z_D::__m256i(v), 0), maskL);
						const auto h = z_D::_mm_shuffle_epi32(z_D::_mm256_extractf128_si256(z_D::__m256i(v), 1), maskH);
						return SimdVector<T, N>(z_D::_mm256_insertf128_si256(z_D::_mm256_castsi128_si256(l), h, 1));
					}
					else if constexpr(CSame<T, float>)
					{
						auto l = z_D::_mm256_extractf128_ps(z_D::__m256(v1), 0);
						auto h = z_D::_mm256_extractf128_ps(z_D::__m256(v1), 1);
						l = z_D::_mm_shuffle_ps(l, l, maskL);
						h = z_D::_mm_shuffle_ps(h, h, maskH);
						return SimdVector<T, N>(z_D::_mm256_insertf128_ps(z_D::_mm256_castps128_ps256(l), h, 1));
					}
				}
			}
		}
		else //double argument case
		{
			constexpr bool isInterleaveLowOp = [&] {
				for(int i = 0; i < N/2; i++)
				{
					if((Iarr[2*i] == i || Iarr[2*i] == -1) &&
						(Iarr[2*i+1] == i + N || Iarr[2*i+1] == -1)) continue;
					return false;
				}
				return true;
			}();
			constexpr bool isInterleaveLowRevOp = [&] {
				for(int i = 0; i < N/2; i++)
				{
					if((Iarr[2*i] == i + N || Iarr[2*i] == -1) &&
						(Iarr[2*i+1] == i || Iarr[2*i+1] == -1)) continue;
					return false;
				}
				return true;
			}();
			constexpr bool isInterleaveHighOp = [&] {
				for(int i = 0; i < N/2; i++)
				{
					if((Iarr[2*i] == N/2 + i || Iarr[2*i] == -1) &&
						(Iarr[2*i+1] == N/2 + i + N || Iarr[2*i+1] == -1)) continue;
					return false;
				}
				return true;
			}();
			constexpr bool isInterleaveHighRevOp = [&] {
				for(int i = 0; i < N/2; i++)
				{
					if((Iarr[2*i] == N/2 + i + N || Iarr[2*i] == -1) &&
						(Iarr[2*i+1] == N/2 + i || Iarr[2*i+1] == -1)) continue;
					return false;
				}
				return true;
			}();

			if constexpr(N == 2)
			{
				if constexpr(CSame<T, double>)
				{
					if constexpr((Iarr[0] == 3 || Iarr[0] == -1) && (Iarr[1] == 1 || Iarr[1] == -1))
						return SimdVector<T, N>(z_D::_mm_movehl_ps(z_D::__m128(v1), z_D::__m128(v2)));
					else if constexpr((Iarr[0] == 1 || Iarr[0] == -1) && (Iarr[1] == 3 || Iarr[1] == -1))
						return SimdVector<T, N>(z_D::_mm_movehl_ps(z_D::__m128(v2), z_D::__m128(v1)));
					else if constexpr(Iarr[0] < 2 && (Iarr[1] >= 2 || Iarr[1] == -1))
					{
						constexpr int mask = indices[0] | (Max(indices[1] - N, 0) << 1);
						return SimdVector<T, N>(z_D::_mm_shuffle_pd(z_D::__m128d(v1), z_D::__m128d(v2), mask));
					}
					else if constexpr(Iarr[1] < N && (Iarr[0] >= N || Iarr[0] == -1))
					{
						constexpr int mask = Max(indices[0] - N, 0) | (indices[1] << 1);
						return SimdVector<T, N>(z_D::_mm_shuffle_pd(z_D::__m128d(v2), z_D::__m128d(v1), mask));
					}
					else if constexpr(isInterleaveLowOp)
						return SimdVector<T, N>(z_D::_mm_unpacklo_pd(z_D::__m128d(v1), z_D::__m128d(v2)));
					else if constexpr(isInterleaveLowRevOp)
						return SimdVector<T, N>(z_D::_mm_unpacklo_pd(z_D::__m128d(v2), z_D::__m128d(v1)));
					else if constexpr(isInterleaveHighOp)
						return SimdVector<T, N>(z_D::_mm_unpackhi_pd(z_D::__m128d(v1), z_D::__m128d(v2)));
					else if constexpr(isInterleaveHighRevOp)
						return SimdVector<T, N>(z_D::_mm_unpackhi_pd(z_D::__m128d(v2), z_D::__m128d(v1)));
				}
				else if constexpr(CAnyOf<T, int64, uint64>)
				{
					if constexpr(isInterleaveLowOp)
						return SimdVector<T, N>(z_D::_mm_unpacklo_epi64(z_D::__m128i(v1), z_D::__m128i(v2)));
					else if constexpr(isInterleaveLowRevOp)
						return SimdVector<T, N>(z_D::_mm_unpacklo_epi64(z_D::__m128i(v2), z_D::__m128i(v1)));
					else if constexpr(isInterleaveHighOp)
						return SimdVector<T, N>(z_D::_mm_unpackhi_epi64(z_D::__m128i(v1), z_D::__m128i(v2)));
					else if constexpr(isInterleaveHighRevOp)
						return SimdVector<T, N>(z_D::_mm_unpackhi_epi64(z_D::__m128i(v2), z_D::__m128i(v1)));
				}
			}
			else if constexpr(N == 4)
			{
				if constexpr(CSame<T, float>)
				{
					if constexpr(
						(Iarr[0] == 6 || Iarr[0] == -1) &&
						(Iarr[1] == 7 || Iarr[1] == -1) &&
						(Iarr[2] == 2 || Iarr[2] == -1) &&
						(Iarr[3] == 3 || Iarr[3] == -1))
						return SimdVector<T, N>(z_D::_mm_movehl_ps(v1, v2));
					else if constexpr(
						(Iarr[0] == 2 || Iarr[0] == -1) &&
						(Iarr[1] == 3 || Iarr[1] == -1) &&
						(Iarr[2] == 6 || Iarr[2] == -1) &&
						(Iarr[3] == 7 || Iarr[3] == -1))
						return SimdVector<T, N>(z_D::_mm_movehl_ps(v2, v1));
					else if constexpr(
						Iarr[0] < N && Iarr[1] < N &&
						(Iarr[2] >= N || Iarr[2] == -1) &&
						(Iarr[3] >= N || Iarr[3] == -1))
					{
						constexpr int mask = indices[0] | (indices[1] << 2) | (Max(indices[2] - N, 0) << 4) | (Max(indices[3] - N, 0) << 6);
						return SimdVector<T, N>(z_D::_mm_shuffle_ps(v1, v2, mask));
					}
					else if constexpr(
						Iarr[2] < N && Iarr[3] < N &&
						(Iarr[0] >= N || Iarr[0] == -1) &&
						(Iarr[1] >= N || Iarr[1] == -1))
					{
						constexpr int mask = indices[2] | (indices[3] << 2) | (Max(indices[0] - N, 0) << 4) | (Max(indices[1] - N, 0) << 6);
						return SimdVector<T, N>(z_D::_mm_shuffle_ps(v2, v1, mask));
					}
					else if constexpr(isInterleaveLowOp)
						return SimdVector<T, N>(z_D::_mm_unpacklo_ps(z_D::__m128(v1), z_D::__m128(v2)));
					else if constexpr(isInterleaveLowRevOp)
						return SimdVector<T, N>(z_D::_mm_unpacklo_ps(z_D::__m128(v2), z_D::__m128(v1)));
					else if constexpr(isInterleaveHighOp)
						return SimdVector<T, N>(z_D::_mm_unpackhi_ps(z_D::__m128(v1), z_D::__m128(v2)));
					else if constexpr(isInterleaveHighRevOp)
						return SimdVector<T, N>(z_D::_mm_unpackhi_ps(z_D::__m128(v2), z_D::__m128(v1)));
				}
				else if constexpr(CAnyOf<T, int32, uint32>)
				{
					if constexpr(isInterleaveLowOp)
						return SimdVector<T, N>(z_D::_mm_unpacklo_epi32(z_D::__m128i(v1), z_D::__m128i(v2)));
					else if constexpr(isInterleaveLowRevOp)
						return SimdVector<T, N>(z_D::_mm_unpacklo_epi32(z_D::__m128i(v2), z_D::__m128i(v1)));
					else if constexpr(isInterleaveHighOp)
						return SimdVector<T, N>(z_D::_mm_unpackhi_epi32(z_D::__m128i(v1), z_D::__m128i(v2)));
					else if constexpr(isInterleaveHighRevOp)
						return SimdVector<T, N>(z_D::_mm_unpackhi_epi32(z_D::__m128i(v2), z_D::__m128i(v1)));
				}
				else if constexpr(CSame<T, double>)
				{
					if constexpr(isInterleaveLowOp)
						return SimdVector<T, N>(z_D::_mm256_permute2f128_pd(
							z_D::_mm256_unpacklo_pd(z_D::__m256d(v1), z_D::__m256d(v2)),
							z_D::_mm256_unpackhi_pd(z_D::__m256d(v1), z_D::__m256d(v2)),
							0x20));
					else if constexpr(isInterleaveLowRevOp)
						return SimdVector<T, N>(z_D::_mm256_permute2f128_pd(
							z_D::_mm256_unpacklo_pd(z_D::__m256d(v2), z_D::__m256d(v1)),
							z_D::_mm256_unpackhi_pd(z_D::__m256d(v2), z_D::__m256d(v1)),
							0x20));
					else if constexpr(isInterleaveHighOp)
						return SimdVector<T, N>(z_D::_mm256_permute2f128_pd(
							z_D::_mm256_unpacklo_pd(z_D::__m256d(v1), z_D::__m256d(v2)),
							z_D::_mm256_unpackhi_pd(z_D::__m256d(v1), z_D::__m256d(v2)),
							0x31));
					else if constexpr(isInterleaveHighRevOp)
						return SimdVector<T, N>(z_D::_mm256_permute2f128_pd(
							z_D::_mm256_unpacklo_pd(z_D::__m256d(v2), z_D::__m256d(v1)),
							z_D::_mm256_unpackhi_pd(z_D::__m256d(v2), z_D::__m256d(v1)),
							0x31));
				}
				else if constexpr(CAnyOf<T, int64, uint64>)
				{
					if constexpr(isInterleaveLowOp)
						return SimdVector<T, N>(z_D::_mm256_permute2f128_si256(
							z_D::_mm256_unpacklo_epi64(z_D::__m256i(v1), z_D::__m256i(v2)),
							z_D::_mm256_unpackhi_epi64(z_D::__m256i(v1), z_D::__m256i(v2)),
							0x20));
					else if constexpr(isInterleaveLowRevOp)
						return SimdVector<T, N>(z_D::_mm256_permute2f128_si256(
							z_D::_mm256_unpacklo_epi64(z_D::__m256i(v2), z_D::__m256i(v1)),
							z_D::_mm256_unpackhi_epi64(z_D::__m256i(v2), z_D::__m256i(v1)),
							0x20));
					else if constexpr(isInterleaveHighOp)
						return SimdVector<T, N>(z_D::_mm256_permute2f128_si256(
							z_D::_mm256_unpacklo_epi64(z_D::__m256i(v1), z_D::__m256i(v2)),
							z_D::_mm256_unpackhi_epi64(z_D::__m256i(v1), z_D::__m256i(v2)),
							0x31));
					else if constexpr(isInterleaveHighRevOp)
						return SimdVector<T, N>(z_D::_mm256_permute2f128_si256(
							z_D::_mm256_unpacklo_epi64(z_D::__m256i(v2), z_D::__m256i(v1)),
							z_D::_mm256_unpackhi_epi64(z_D::__m256i(v2), z_D::__m256i(v1)),
							0x31));
				}
			}
			else if constexpr(N == 8)
			{
				if constexpr(CSame<T, float>)
				{
					if constexpr(isInterleaveLowOp)
						return SimdVector<T, N>(z_D::_mm256_permute2f128_ps(
							z_D::_mm256_unpacklo_ps(z_D::__m256(v1), z_D::__m256(v2)),
							z_D::_mm256_unpackhi_ps(z_D::__m256(v1), z_D::__m256(v2)), 0x20));
					else if constexpr(isInterleaveLowRevOp)
						return SimdVector<T, N>(z_D::_mm256_permute2f128_ps(
							z_D::_mm256_unpacklo_ps(z_D::__m256(v2), z_D::__m256(v1)),
							z_D::_mm256_unpackhi_ps(z_D::__m256(v2), z_D::__m256(v1)), 0x20));
					else if constexpr(isInterleaveHighOp)
						return SimdVector<T, N>(z_D::_mm256_permute2f128_ps(
							z_D::_mm256_unpacklo_ps(z_D::__m256(v1), z_D::__m256(v2)),
							z_D::_mm256_unpackhi_ps(z_D::__m256(v1), z_D::__m256(v2)), 0x31));
					else if constexpr(isInterleaveHighRevOp)
						return SimdVector<T, N>(z_D::_mm256_permute2f128_ps(
							z_D::_mm256_unpacklo_ps(z_D::__m256(v2), z_D::__m256(v1)),
							z_D::_mm256_unpackhi_ps(z_D::__m256(v2), z_D::__m256(v1)), 0x31));
				}
				else if constexpr(CAnyOf<T, int32, uint32>)
				{
					if constexpr(isInterleaveLowOp)
						return SimdVector<T, N>(z_D::_mm256_permute2f128_si256(
							z_D::_mm256_unpacklo_epi32(z_D::__m256i(v1), z_D::__m256i(v2)),
							z_D::_mm256_unpackhi_epi32(z_D::__m256i(v1), z_D::__m256i(v2)),
							0x20));
					else if constexpr(isInterleaveLowRevOp)
						return SimdVector<T, N>(z_D::_mm256_permute2f128_si256(
							z_D::_mm256_unpacklo_epi32(z_D::__m256i(v2), z_D::__m256i(v1)),
							z_D::_mm256_unpackhi_epi32(z_D::__m256i(v2), z_D::__m256i(v1)),
							0x20));
					else if constexpr(isInterleaveHighOp)
						return SimdVector<T, N>(z_D::_mm256_permute2f128_si256(
							z_D::_mm256_unpacklo_epi32(z_D::__m256i(v1), z_D::__m256i(v2)),
							z_D::_mm256_unpackhi_epi32(z_D::__m256i(v1), z_D::__m256i(v2)),
							0x31));
					else if constexpr(isInterleaveHighRevOp)
						return SimdVector<T, N>(z_D::_mm256_permute2f128_si256(
							z_D::_mm256_unpacklo_epi32(z_D::__m256i(v2), z_D::__m256i(v1)),
							z_D::_mm256_unpackhi_epi32(z_D::__m256i(v2), z_D::__m256i(v1)),
							0x31));
				}
			}
			else if constexpr(N == 16)
			{
				if constexpr(CAnyOf<T, int16, uint16>)
				{
					if constexpr(isInterleaveLowOp)
						return SimdVector<T, N>(z_D::_mm256_permute2f128_si256(
							z_D::_mm256_unpacklo_epi16(z_D::__m256i(v1), z_D::__m256i(v2)),
							z_D::_mm256_unpackhi_epi16(z_D::__m256i(v1), z_D::__m256i(v2)),
							0x20));
					else if constexpr(isInterleaveLowRevOp)
						return SimdVector<T, N>(z_D::_mm256_permute2f128_si256(
							z_D::_mm256_unpacklo_epi16(z_D::__m256i(v2), z_D::__m256i(v1)),
							z_D::_mm256_unpackhi_epi16(z_D::__m256i(v2), z_D::__m256i(v1)),
							0x20));
					else if constexpr(isInterleaveHighOp)
						return SimdVector<T, N>(z_D::_mm256_permute2f128_si256(
							z_D::_mm256_unpacklo_epi16(z_D::__m256i(v1), z_D::__m256i(v2)),
							z_D::_mm256_unpackhi_epi16(z_D::__m256i(v1), z_D::__m256i(v2)),
							0x31));
					else if constexpr(isInterleaveHighRevOp)
						return SimdVector<T, N>(z_D::_mm256_permute2f128_si256(
							z_D::_mm256_unpacklo_epi16(z_D::__m256i(v2), z_D::__m256i(v1)),
							z_D::_mm256_unpackhi_epi16(z_D::__m256i(v2), z_D::__m256i(v1)),
							0x31));
				}
			}
			else if constexpr(N == 32)
			{
				if constexpr(CAnyOf<T, int8, uint8>)
				{
					if constexpr(isInterleaveLowOp)
						return SimdVector<T, N>(z_D::_mm256_permute2f128_si256(
							z_D::_mm256_unpacklo_epi8(z_D::__m256i(v1), z_D::__m256i(v2)),
							z_D::_mm256_unpackhi_epi8(z_D::__m256i(v1), z_D::__m256i(v2)),
							0x20));
					else if constexpr(isInterleaveLowRevOp)
						return SimdVector<T, N>(z_D::_mm256_permute2f128_si256(
							z_D::_mm256_unpacklo_epi8(z_D::__m256i(v2), z_D::__m256i(v1)),
							z_D::_mm256_unpackhi_epi8(z_D::__m256i(v2), z_D::__m256i(v1)),
							0x20));
					else if constexpr(isInterleaveHighOp)
						return SimdVector<T, N>(z_D::_mm256_permute2f128_si256(
							z_D::_mm256_unpacklo_epi8(z_D::__m256i(v1), z_D::__m256i(v2)),
							z_D::_mm256_unpackhi_epi8(z_D::__m256i(v1), z_D::__m256i(v2)),
							0x31));
					else if constexpr(isInterleaveHighRevOp)
						return SimdVector<T, N>(z_D::_mm256_permute2f128_si256(
							z_D::_mm256_unpacklo_epi8(z_D::__m256i(v2), z_D::__m256i(v1)),
							z_D::_mm256_unpackhi_epi8(z_D::__m256i(v2), z_D::__m256i(v1)),
							0x31));
				}
			}
		}
	}
#endif
	const SimdVector<T, N> vs[2] = {v1, v2};
	return SimdVector<T, N>{vs[Is/N][Max(Is, 0) % N]...};
}

template<int... Is, typename T, size_t N>
INTRA_FORCEINLINE SimdVector<T, N> SimdShuffle(SimdVector<T, N> v) noexcept
{
	static_assert(sizeof...(Is) == N);
	static_assert(((-1 <= Is && Is < N) && ...), "Shuffle indices must be in range [-1; N)");
	return SimdShuffle<Is...>(v, v);
}
	
template<typename T, size_t N>
INTRA_FORCEINLINE SimdVector<T, N> SimdShuffleDynamic(SimdVector<T, N> v, SimdVector<TToIntegral<T>, N> indices) noexcept
{
#if INTRA_CONFIG_USE_VECTOR_EXTENSIONS && !defined(__clang__) && !defined(__INTEL_COMPILER)
	return __builtin_shuffle(v, indices);
#elif defined(INTRAZ_D_USE_SSE_INTRINSICS)
	if constexpr(Config::TargetMaxSimdLength<T> >= N)
	{
		if constexpr(N == 4 && Config::TargetHasSSSE3)
		{
			auto transformedIndices = z_D::_mm_shuffle_epi8(z_D::__m128i(indices << 2),
				z_D::__m128i(SimdVector<int8, 16>{12, 12, 12, 12, 8, 8, 8, 8, 4, 4, 4, 4, 0, 0, 0, 0}));
			transformedIndices = __m128i(SimdVector<int8, 16>(transformedIndices) +
				SimdVector<int8, 16>{3, 2, 1, 0, 3, 2, 1, 0, 3, 2, 1, 0, 3, 2, 1, 0});
			return SimdVector<T, N>(z_D::_mm_shuffle_epi8(z_D::__m128i(v), indices));
		}
	}
#endif
	return [&]<size_t... Is>(TIndexSeq<Is...>) {
		return SimdVector<T, N>{indices[Is]...};
	}(TMakeIndexSeq<N>{});
}

template<size_t ElemShift, typename T, size_t N>
INTRA_FORCEINLINE SimdVector<T, N> SimdRotateLeft(SimdVector<T, N> v) noexcept
{
	return [&]<size_t... Is>(TIndexSeq<Is...>) {
		return SimdShuffle<((ElemShift + Is) % N)...>(v);
	}(TMakeIndexSeq<N>{});
}

template<size_t ElemShift, typename T, size_t N>
INTRA_FORCEINLINE SimdVector<T, N> SimdRotateRight(SimdVector<T, N> v) noexcept {return SimdRotateLeft<N-ElemShift>(v);}

template<typename T, size_t N>
INTRA_FORCEINLINE SimdVector<T, N> SimdSelect(
	SimdVector<T, N> ifFalse, SimdVector<T, N> ifTrue, SimdVector<TToIntegral<T>, N> broadBoolMask) noexcept
{
#if INTRA_CONFIG_USE_VECTOR_EXTENSIONS && !defined(__INTEL_COMPILER) && (!defined(__clang__) || (__clang_major__ >= 10 && !defined(__APPLE__) || __clang_major__ >= 12))
	return broadBoolMask? ifTrue: ifFalse;
#else
	if constexpr(Config::TargetMaxSimdLength<T> >= N)
	{
	#ifdef INTRAZ_D_USE_SSE_INTRINSICS
		if constexpr(sizeof(SimdVector<T, N>) == 16)
		{
			if constexpr(Config::TargetHasSSE41)
				return SimdVector<T, N>(z_D::_mm_blendv_epi8(z_D::__m128i(ifFalse), z_D::__m128i(ifTrue), z_D::__m128i(broadBoolMask)));
		}
		else if constexpr(sizeof(SimdVector<T, N>) == 32)
			return SimdVector<T, N>(z_D::_mm256_blendv_epi8(z_D::__m256i(ifFalse), z_D::__m256i(ifTrue), z_D::__m256i(broadBoolMask)));
		else if constexpr(sizeof(SimdVector<T, N>) == 64)
			return SimdVector<T, N>(z_D::_mm512_mask_blend_epi8(
				z_D::_mm512_movepi8_mask(z_D::__m512i(ifFalse)), z_D::__m512i(ifTrue), z_D::__m512i(broadBoolMask)));
	#elif defined(__ARM_NEON)
		//TODO
	#endif
	}
	using EquivIntVector = decltype(broadBoolMask);
	return SimdVector<T, N>(
		(EquivIntVector(ifTrue) & broadBoolMask) |
		(EquivIntVector(ifFalse) & ~broadBoolMask));
#endif
}

template<typename T, size_t N, class F> requires CCallable<F, SimdVector<T, N>, SimdVector<T, N>> && CCallable<F, T, T>
INTRA_FORCEINLINE T SimdHorReduce(SimdVector<T, N> v, F f) noexcept
{
	if constexpr(Config::TargetMaxSimdLength<T> >= N)
	{
		if constexpr(sizeof(SimdVector<T, N>) == 16)
		{
			if constexpr(CSame<T, float>)
			{
				SimdVector<T, N> shuf = SimdShuffle<1, 1, 3, 3>(v); // movshdup (SSE3) or shufps (SSE)
				SimdVector<T, N> partialResult = f(v, shuf);      // {f(v[0], v[1]), f(v[1], v[1]), f(v[2], v[3]), f(v[3], v[3])}
				shuf = SimdShuffle<6, -1, -1, -1>(shuf, partialResult); // {f(v[2], v[3]), ...}; movhlps, lets the compiler avoid movps by reusing shuf
				return f(partialResult, shuf)[0]; // {f(f(v[0], v[1]), f(v[2], v[3])), ...}[0]
			}
			else if constexpr(CSame<T, double>) return f(v, SimdShuffle<1, -1>(v))[0];
			else if constexpr(CIntegral<T> && N >= 4)
			{
				const auto hi64 = SimdVector<T, N>(SimdShuffle<1, -1>(SimdVector<int64, 2>(v)));
				const auto sum64 = f(hi64, v);
				if constexpr(N >= 8)
				{
					const auto hi32 = SimdVector<T, N>(SimdShuffle<2, 3, -1, -1, -1, -1, -1, -1>(SimdVector<int16, 8>(sum64)));
					const auto sum32 = f(sum64, hi32);
					if constexpr(N == 8) return f(sum32[0], sum32[1]);
					else
					{
						const auto hi16 = SimdVector<T, N>(SimdShuffle<1, -1, -1, -1, -1, -1, -1, -1>(SimdVector<int16, 8>(sum32));
						const auto sum16 = f(sum32, hi16);
						return f(sum16[0], sum16[1]);
					}
				}
				else return f(sum64[0], sum64[1]);
			}
		}
	#ifdef __AVX__
		else if constexpr(sizeof(SimdVector<T, N>) == 32)
		{
			if constexpr(CSame<T, float>)
			{
				const auto vlow = SimdVector<T, N/2>(z_D::_mm256_castps256_ps128(v));
				const auto vhigh = SimdVector<T, N/2>(z_D::_mm256_extractf128_ps(v, 1));
				return SimdHorReduce(f(vlow, vhigh), f);
			}
			else if constexpr(CSame<T, double>)
			{
				const auto vlow = SimdVector<T, N/2>(z_D::_mm256_castpd256_pd128(v));
				const auto vhigh = SimdVector<T, N/2>(z_D::_mm256_extractf128_pd(v, 1));
				return SimdHorReduce(f(vlow, vhigh), f);
			}
			else if constexpr(CIntegral<T>)
			{
				const auto vlow = SimdVector<T, N/2>(z_D::_mm256_castsi256_si128(v));
				const auto vhigh = SimdVector<T, N/2>(z_D::_mm256_extractf128_si256(v, 1));
				return SimdHorReduce(f(vlow, vhigh), f);
			}
		}
		else if constexpr(sizeof(SimdVector<T, N>) == 64)
		{
			if constexpr(CSame<T, float>)
			{
				const auto vlow = SimdVector<T, N/2>(z_D::_mm512_castps512_ps256(v));
				const auto vhigh = SimdVector<T, N/2>(z_D::_mm512_extractf32x8_ps(v, 1));
				return SimdHorReduce(f(vlow, vhigh), f);
			}
			else if constexpr(CSame<T, double>)
			{
				const auto vlow = SimdVector<T, N/2>(z_D::_mm512_castpd512_pd256(v));
				const auto vhigh = SimdVector<T, N/2>(z_D::_mm512_extractf64x4_pd(v, 1));
				return SimdHorReduce(f(vlow, vhigh), f);
			}
			else if constexpr(CIntegral<T>)
			{
				const auto vlow = SimdVector<T, N/2>(z_D::_mm512_castsi512_si256(v));
				const auto vhigh = SimdVector<T, N/2>(z_D::_mm512_extracti32x8_epi32(v, 1));
				return SimdHorReduce(f(vlow, vhigh), f);
			}
		}
	#endif
	}
	T res = v[0];
	for(size_t i = 1; i < N; i++) res = f(res, v[i]);
	return res;
}

template<typename Dst, typename T, size_t N>
INTRA_FORCEINLINE Dst SimdCastTo(SimdVector<T, N> v) noexcept
{
	using DstT = decltype(Dst()[0]);
#if defined(__clang__) || __GNUC__ >= 9 && !defined(__INTEL_COMPILER)
	return __builtin_convertvector(v, Dst);
#elif defined(INTRAZ_D_USE_SSE_INTRINSICS)
	if constexpr(sizeof(SimdVector<T, N>) == 16)
	{
		if constexpr(CSame<T, float>)
		{
			if constexpr(CSame<DstT, int32>) return z_D::_mm_cvttps_epi32(v);
		}
		else if constexpr(CSame<T, double>)
		{
			if constexpr(CSame<DstT, int64>)
			{
				if constexpr(Config::TargetHasAVX512)
					return z_D::_mm_cvttpd_epi64(v);
			}
			//else if constexpr(CSame<DstT, int32>) return z_D::_mm_cvttpd_epi32(v); // TODO: result size is 2*N - move to another function
		}
		else if constexpr(CSame<T, int32>)
		{
			if constexpr(CSame<DstT, float>) return z_D::_mm_cvtepi32_ps(v);
			//else if constexpr(CSame<DstT, double>) return z_D::_mm_cvtepi32_pd(v);  // TODO: result size is N/2 - move to another function
		}
	}
	else if constexpr(sizeof(SimdVector<T, N>) == 32)
	{
		if constexpr(CSame<T, float>)
		{
			if constexpr(CSame<DstT, int32>) return z_D::_mm256_cvttps_epi32(v);
			else if constexpr(CSame<DstT, int64>)
			{
				if constexpr(Config::TargetHasAVX512)
					return z_D::_mm512_cvttps_epi64(v);
			}
		}
		else if constexpr(CSame<T, double>)
		{
			if constexpr(CSame<DstT, int32>) return z_D::_mm256_cvttpd_epi32(v);
			else if constexpr(CSame<DstT, int64>)
			{
				if constexpr(Config::TargetHasAVX512)
					return z_D::_mm256_cvttpd_epi64(v);
			}
		}
	}
	else if constexpr(sizeof(SimdVector<T, N>) == 64)
	{
		if constexpr(CSame<T, float>)
		{
			if constexpr(CSame<DstT, int32>) return z_D::_mm512_cvttps_epi32(v);
		}
		else if constexpr(CSame<T, double>)
		{
			if constexpr(CSame<DstT, int32>) return z_D::_mm512_cvttpd_epi32(v);
			else if constexpr(CSame<DstT, int64>) return z_D::_mm512_cvttpd_epi64(v);
		}
	}
#endif
	return [&]<size_t... Is>(TIndexSeq<Is...>) {
		return Dst{DstT(v[Is])...};
	}(TMakeIndexSeq<N>{});
}

INTRA_OPTIMIZE_FUNCTION(template<typename T, size_t N>)
INTRA_FORCEINLINE SimdVector<T, N> InterleaveLow(SimdVector<T, N> a, SimdVector<T, N> b) noexcept
{
	return [&]<size_t... Is>(TIndexSeq<Is...>) {
		return SimdShuffle<(Is%2*N + Is/2)...>(a, b);
	}(TMakeIndexSeq<N>{});
}
INTRA_OPTIMIZE_FUNCTION_END

INTRA_OPTIMIZE_FUNCTION(template<typename T, size_t N>)
INTRA_FORCEINLINE SimdVector<T, N> InterleaveHigh(SimdVector<T, N> a, SimdVector<T, N> b) noexcept
{
	return [&]<size_t... Is>(TIndexSeq<Is...>) {
		return SimdShuffle<(N/2 + Is%2*N + Is/2)...>(a, b);
	}(TMakeIndexSeq<N>{});
}
INTRA_OPTIMIZE_FUNCTION_END

template<typename T, size_t N> INTRA_FORCEINLINE SimdVector<T, N> SimdAnd(SimdVector<T, N> a, SimdVector<T, N> b)
{
	if constexpr(CIntegral<T>) return a & b;
	else return SimdVector<T, N>(SimdVector<TToIntegral<T>, N>(a) & SimdVector<TToIntegral<T>, N>(b));
}

namespace z_D {
template<typename T, size_t N> requires CSigned<T>
inline SimdVector<T, N> Abs_(SimdVector<T, N> x)
{
	return SimdVector<T, N>(SimdVector<TToIntegral<T>, N>(x) & MaxValueOf<TToIntegral<T>>);
}
template<typename T, size_t N> requires CFloatingPoint<T>
inline SimdVector<T, N> Floor_(SimdVector<T, N> x)
{
	if constexpr(Config::TargetMaxSimdLength<T> >= N)
	{
	#ifdef __SSE4_1__
	#if INTRA_CONFIG_USE_VECTOR_EXTENSIONS
		if constexpr(sizeof(SimdVector<T, N>) == 16 && Config::TargetHasSSE4_1)
		{
			if constexpr(CSame<T, float>) return SimdVector<T, N>(__builtin_ia32_roundps(x, 9));
			else if constexpr(CSame<T, double>) return SimdVector<T, N>(__builtin_ia32_roundpd(x, 9));
		}
		else if constexpr(sizeof(SimdVector<T, N>) == 32)
		{
			if constexpr(CSame<T, float>) return SimdVector<T, N>(__builtin_ia32_roundps256(x, 9));
			else if constexpr(CSame<T, double>) return SimdVector<T, N>(__builtin_ia32_roundpd256(x, 9));
		}
		if constexpr(sizeof(SimdVector<T, N>) == 64)
		{
			if constexpr(CSame<T, float>) return SimdVector<T, N>(__builtin_ia32_rndscaleps_mask(x, 9, x, -1, 4));
			else if constexpr(CSame<T, double>) return SimdVector<T, N>(__builtin_ia32_rndscalepd_mask(x, 9, x, -1, 4));
		}
	#else
		if constexpr(sizeof(SimdVector<T, N>) == 16 && Config::TargetHasSSE4_1)
		{
			if constexpr(CSame<T, float>) return SimdVector<T, N>(z_D::_mm_round_ps(x, 9));
			else if constexpr(CSame<T, double>) return SimdVector<T, N>(z_D::_mm_round_pd(x, 9));
		}
		else if constexpr(sizeof(SimdVector<T, N>) == 32)
		{
			if constexpr(CSame<T, float>) return SimdVector<T, N>(z_D::_mm256_round_ps(x, 9));
			else if constexpr(CSame<T, double>) return SimdVector<T, N>(z_D::_mm256_round_pd(x, 9));
		}
		if constexpr(sizeof(SimdVector<T, N>) == 64)
		{
			if constexpr(CSame<T, float>) return SimdVector<T, N>(z_D::_mm512_floor_ps(x));
			else if constexpr(CSame<T, double>) return SimdVector<T, N>(z_D::_mm512_floor_pd(x));
		}
	#endif
	#endif
	}
	const auto fi = SimdCastTo<SimdVector<T, N>>(SimdCastTo<SimdVector<TToIntegral<T>, N>>(x));
	return fi - SimdAnd(SimdVectorFilled<T, N>(1), fi > x);
}

template<typename T, size_t N> requires CFloatingPoint<T>
inline SimdVector<T, N> Ceil_(SimdVector<T, N> x)
{
	if constexpr(Config::TargetMaxSimdLength<T> >= N)
	{
	#ifdef __SSE4_1__
	#if INTRA_CONFIG_USE_VECTOR_EXTENSIONS
		if constexpr(sizeof(SimdVector<T, N>) == 16 && Config::TargetHasSSE4_1)
		{
			if constexpr(CSame<T, float>) return SimdVector<T, N>(__builtin_ia32_roundps(x, 10));
			else if constexpr(CSame<T, double>) return SimdVector<T, N>(__builtin_ia32_roundpd(x, 10));
		}
		else if constexpr(sizeof(SimdVector<T, N>) == 32)
		{
			if constexpr(CSame<T, float>) return SimdVector<T, N>(__builtin_ia32_roundps256(x, 10));
			else if constexpr(CSame<T, double>) return SimdVector<T, N>(__builtin_ia32_roundpd256(x, 10));
		}
		if constexpr(sizeof(SimdVector<T, N>) == 64)
		{
			if constexpr(CSame<T, float>) return SimdVector<T, N>(__builtin_ia32_rndscaleps_mask(x, 10, x, -1, 4));
			else if constexpr(CSame<T, double>) return SimdVector<T, N>(__builtin_ia32_rndscalepd_mask(x, 10, x, -1, 4));
		}
	#else
		if constexpr(sizeof(SimdVector<T, N>) == 16 && Config::TargetHasSSE4_1)
		{
			if constexpr(CSame<T, float>) return SimdVector<T, N>(z_D::_mm_round_ps(x, 10));
			else if constexpr(CSame<T, double>) return SimdVector<T, N>(z_D::_mm_round_pd(x, 10));
		}
		else if constexpr(sizeof(SimdVector<T, N>) == 32)
		{
			if constexpr(CSame<T, float>) return SimdVector<T, N>(z_D::_mm256_round_ps(x, 10));
			else if constexpr(CSame<T, double>) return SimdVector<T, N>(z_D::_mm256_round_pd(x, 10));
		}
		if constexpr(sizeof(SimdVector<T, N>) == 64)
		{
			if constexpr(CSame<T, float>) return SimdVector<T, N>(z_D::_mm512_ceil_ps(x));
			else if constexpr(CSame<T, double>) return SimdVector<T, N>(z_D::_mm512_ceil_pd(x));
		}
	#endif
	#endif
	}
	const auto fi = SimdCastTo<SimdVector<T, N>>(SimdCastTo<SimdVector<TToIntegral<T>, N>>(x));
	return fi + SimdAnd(SimdVectorFilled<T, N>(1), fi < x);
}

template<typename T, size_t N> requires CFloatingPoint<T>
inline SimdVector<T, N> Round_(SimdVector<T, N> x)
{
#ifdef __SSE4_1__
#if INTRA_CONFIG_USE_VECTOR_EXTENSIONS
	if constexpr(sizeof(SimdVector<T, N>) == 16 && Config::TargetHasSSE41)
	{
		if constexpr(CSame<T, float>) return SimdVector<T, N>(__builtin_ia32_roundps(x, 12));
		else if constexpr(CSame<T, double>) return SimdVector<T, N>(__builtin_ia32_roundpd(x, 12));
	}
	else if constexpr(sizeof(SimdVector<T, N>) == 32 && Config::TargetHasAVX)
	{
		if constexpr(CSame<T, float>) return SimdVector<T, N>(__builtin_ia32_roundps256(x, 12));
		else if constexpr(CSame<T, double>) return SimdVector<T, N>(__builtin_ia32_roundpd256(x, 12));
	}
	if constexpr(sizeof(SimdVector<T, N>) == 64 && Config::TargetHasAVX512F)
	{
		if constexpr(CSame<T, float>) return SimdVector<T, N>(__builtin_ia32_rndscaleps_mask(x, 12, x, -1, 4));
		else if constexpr(CSame<T, double>) return SimdVector<T, N>(__builtin_ia32_rndscalepd_mask(x, 12, x, -1, 4));
	}
#else
	if constexpr(sizeof(SimdVector<T, N>) == 16 && Config::TargetHasSSE41)
	{
		if constexpr(CSame<T, float>) return SimdVector<T, N>(z_D::_mm_round_ps(x, 12));
		else if constexpr(CSame<T, double>) return SimdVector<T, N>(z_D::_mm_round_pd(x, 12));
	}
	else if constexpr(sizeof(SimdVector<T, N>) == 32 && Config::TargetHasAVX)
	{
		if constexpr(CSame<T, float>) return SimdVector<T, N>(z_D::_mm256_round_ps(x, 12));
		else if constexpr(CSame<T, double>) return SimdVector<T, N>(z_D::_mm256_round_pd(x, 12));
	}
#endif
#endif
	const auto fi = SimdCastTo<SimdVector<T, N>>(SimdCastTo<SimdVector<TToIntegral<T>, N>>(x));
	const auto offset = SimdCastTo<SimdVector<T, N>>(SimdCastTo<SimdVector<TToIntegral<T>, N>>(
		(x - fi)*(2 - EpsilonOf<T>)
	));
	return fi + offset;
}

template<typename T, size_t N> requires CFloatingPoint<T>
inline SimdVector<T, N> Trunc_(SimdVector<T, N> x)
{
#ifdef __SSE4_1__
#if INTRA_CONFIG_USE_VECTOR_EXTENSIONS
	if constexpr(sizeof(SimdVector<T, N>) == 16 && Config::TargetHasSSE41)
	{
		if constexpr(CSame<T, float>) return SimdVector<T, N>(__builtin_ia32_roundps(x, 11));
		else if constexpr(CSame<T, double>) return SimdVector<T, N>(__builtin_ia32_roundpd(x, 11));
	}
	else if constexpr(sizeof(SimdVector<T, N>) == 32 && Config::TargetHasAVX)
	{
		if constexpr(CSame<T, float>) return SimdVector<T, N>(__builtin_ia32_roundps256(x, 11));
		else if constexpr(CSame<T, double>) return SimdVector<T, N>(__builtin_ia32_roundpd256(x, 11));
	}
	if constexpr(sizeof(SimdVector<T, N>) == 64 && Config::TargetHasAVX512F)
	{
		if constexpr(CSame<T, float>) return SimdVector<T, N>(__builtin_ia32_rndscaleps_mask(x, 11, x, -1, 4));
		else if constexpr(CSame<T, double>) return SimdVector<T, N>(__builtin_ia32_rndscalepd_mask(x, 11, x, -1, 4));
	}
#else
	if constexpr(sizeof(SimdVector<T, N>) == 16 && Config::TargetHasSSE41)
	{
		if constexpr(CSame<T, float>) return SimdVector<T, N>(z_D::_mm_round_ps(x, 11));
		else if constexpr(CSame<T, double>) return SimdVector<T, N>(z_D::_mm_round_pd(x, 11));
	}
	else if constexpr(sizeof(SimdVector<T, N>) == 32 && Config::TargetHasAVX)
	{
		if constexpr(CSame<T, float>) return SimdVector<T, N>(z_D::_mm256_round_ps(x, 11));
		else if constexpr(CSame<T, double>) return SimdVector<T, N>(z_D::_mm256_round_pd(x, 11));
	}
#endif
#endif
	const auto fi = SimdCastTo<SimdVector<T, N>>(SimdCastTo<SimdVector<TToIntegral<T>, N>>(x));
	return SimdSelect(x, fi, Abs_(x) < T(IntegerRangeMax));
}
}

INTRA_FORCEINLINE void SimdEnd() noexcept
{
#if !INTRA_CONFIG_USE_VECTOR_EXTENSIONS && defined(__AVX__) && !defined(__KNL__)
	z_D::_mm256_zeroupper();
#endif
}

INTRA_END



// runtime instruction support detection
namespace z_D {
#if defined(__i386__) || defined(__amd64__)
#ifdef _MSC_VER
extern "C" {
	void __cpuid(int[4], int);
	uint64 _xgetbv(unsigned index);
}
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

inline uint64 _xgetbv(unsigned int index)
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
#endif
}

inline bool IsAvxSupported()
{
#if defined(__i386__) || defined(__amd64__)
	int cpuinfo[4];
	z_D::__cpuid(cpuinfo, 1);
	bool supported = (cpuinfo[2] & (1 << 28)) != 0;
	const bool osxsaveSupported = (cpuinfo[2] & (1 << 27)) != 0;
	if(osxsaveSupported && supported)
	{
		// _XCR_XFEATURE_ENABLED_MASK = 0
		unsigned long long xcrFeatureMask = z_D::_xgetbv(0);
		supported = (xcrFeatureMask & 0x6) == 0x6;
	}
	return supported;
#else
	return false;
#endif
}
}

INTRA_END
