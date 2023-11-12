#pragma once

#include <Intra/Preprocessor.h>
#include <Intra/Numeric/Math.h>

// This module defines SIMD vector types that can be used on different platforms and compilers.
// Its design is based on GCC vector extensions. When compiled with GCC or Clang it is defined as a thin free function wrapper around them.
// When used with MSVC it defines a class with overloaded operators mimicking GCC vector extensions implemented via intrinsics.
// TODO: support NEON on MSVC and emulate unsupported vector sizes.

// Performance warning for MSVC:
// Don't call functions compiled with /arch:SSE2- from /arch:AVX+ code.
// When LTO is enabled it prevents the wrapper from inlining which makes it extremely slow.

#ifndef INTRA_CONFIG_USE_VECTOR_EXTENSIONS
#if defined(__GNUC__) || defined(__clang__)
#define INTRA_CONFIG_USE_VECTOR_EXTENSIONS 1
#else
#define INTRA_CONFIG_USE_VECTOR_EXTENSIONS 0
#endif
#endif


namespace Intra { INTRA_BEGIN
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
	(CBasicIntegral<T>? TargetHasAVX2: TargetHasAVX)? 32:
	(TargetHasNEON || (CBasicIntegral<T>? TargetHasSSE: TargetHasSSE2))? 16: sizeof(T))/sizeof(T);
}
} INTRA_END

#if !defined(__FMA__) && defined(__AVX2__) && !defined(__GNUC__) && !defined(__clang__)
#define __FMA__ 1
#endif

//// <SimdVector implementation for MSVC>
#ifndef INTRA_GNU_EXTENSION_SUPPORT
namespace Intra { INTRA_BEGIN

namespace z_D {
union __declspec(intrin_type) alignas(16) __m128
{
	float Floats[4];
};
union __declspec(intrin_type) alignas(16) __m128i
{
	int8 Ints[16];
	uint8 Uints[16];
	int16 Ints16[8];
	uint16 Uints16[8];
	int32 Ints32[4];
	uint32 Uints32[4];
	int64 Ints64[2];
	uint64 Uints64[2];
};
struct __declspec(intrin_type) alignas(16) __m128d
{
	double Doubles[2];
};

union __declspec(intrin_type) alignas(32) __m256
{
	float Floats[8];
};
union __declspec(intrin_type) alignas(32) __m256i
{
	int8 Ints[32];
	uint8 Uints[32];
	int16 Ints16[16];
	uint16 Uints16[16];
	int32 Ints32[8];
	uint32 Uints32[8];
	int64 Ints64[4];
	uint64 Uints64[4];
};
struct __declspec(intrin_type) alignas(32) __m256d
{
	double Doubles[4];
};
union __declspec(intrin_type) alignas(64) __m512
{
	float Floats[16];
};
union __declspec(intrin_type) alignas(64) __m512i
{
	int8 Ints[64];
	uint8 Uints[64];
	int16 Ints16[32];
	uint16 Uints16[32];
	int32 Ints32[16];
	uint32 Uints32[16];
	int64 Ints64[8];
	uint64 Uints64[8];
};
struct __declspec(intrin_type) alignas(64) __m512d
{
	double Doubles[8];
};
struct m128u {char c[16];};
struct m256u {char c[32];};
struct m512u {char c[64];};

extern "C" {
	__m128 _mm_set_ps(INTRA_MACRO_SIMPLE_REPEAT(4, float, (,)));
	__m128d _mm_set_pd(INTRA_MACRO_SIMPLE_REPEAT(2, double, (,)));

	__m128i _mm_set_epi8(INTRA_MACRO_SIMPLE_REPEAT(16, char, (,)));
	__m128i _mm_set_epi16(INTRA_MACRO_SIMPLE_REPEAT(8, int16, (,)));
	__m128i _mm_set_epi32(INTRA_MACRO_SIMPLE_REPEAT(4, int32, (,)));
	__m128i _mm_set_epi64x(INTRA_MACRO_SIMPLE_REPEAT(2, int64, (,)));


	__m256 _mm256_set_ps(INTRA_MACRO_SIMPLE_REPEAT(8, float, (,)));
	__m256d _mm256_set_pd(INTRA_MACRO_SIMPLE_REPEAT(4, double, (,)));

	__m256i _mm256_set_epi8(INTRA_MACRO_SIMPLE_REPEAT(32, char, (,)));
	__m256i _mm256_set_epi16(INTRA_MACRO_SIMPLE_REPEAT(16, int16, (,)));
	__m256i _mm256_set_epi32(INTRA_MACRO_SIMPLE_REPEAT(8, int32, (,)));
	__m256i _mm256_set_epi64x(INTRA_MACRO_SIMPLE_REPEAT(4, int64, (,)));


	__m512 _mm512_set_ps(INTRA_MACRO_SIMPLE_REPEAT(16, float, (,)));
	__m512d _mm512_set_pd(INTRA_MACRO_SIMPLE_REPEAT(8, double, (,)));

	__m512i _mm512_set_epi8(INTRA_MACRO_SIMPLE_REPEAT(64, char, (,)));
	__m512i _mm512_set_epi16(INTRA_MACRO_SIMPLE_REPEAT(32, int16, (,)));
	__m512i _mm512_set_epi32(INTRA_MACRO_SIMPLE_REPEAT(16, int32, (,)));
	__m512i _mm512_set_epi64(INTRA_MACRO_SIMPLE_REPEAT(8, int64, (,)));

	__m256 __cdecl _mm256_cmp_ps(__m256 a, __m256 b, int op);
	__m256d __cdecl _mm256_cmp_pd(__m256d a, __m256d b, int op);
	uint16 __cdecl _mm512_cmp_ps_mask(__m512 a, __m512 b, int op);
	uint8 __cdecl _mm512_cmp_pd_mask(__m512d a, __m512d b, int op);

	__m512i __cdecl _mm512_movm_epi8(uint64);
	__m512i __cdecl _mm512_movm_epi16(uint32);
	__m512i __cdecl _mm512_movm_epi32(uint16);
	__m512i __cdecl _mm512_movm_epi64(uint8);


	__m128i _mm_packs_epi16(__m128i a, __m128i b);
	__m128i _mm_unpacklo_epi8(__m128i a, __m128i b);
	__m128i _mm_unpackhi_epi8(__m128i a, __m128i b);
	__m256i __cdecl _mm256_packs_epi16(__m256i a, __m256i b);
	__m256i __cdecl _mm256_unpacklo_epi8(__m256i a, __m256i b);
	__m256i __cdecl _mm256_unpackhi_epi8(__m256i a, __m256i b);
	__m512i __cdecl _mm512_packs_epi16(__m512i a, __m512i b);
	__m512i __cdecl _mm512_unpacklo_epi8(__m512i a, __m512i b);
	__m512i __cdecl _mm512_unpackhi_epi8(__m512i a, __m512i b);
}

// The functions below will be defined to use direct intrinsics without any target instruction set checks or emulation.
// Caller is responsible for knowing what instruction set each specialization requires and
// for doing all these checks in compile-time with unsupported function emulation.
template<typename T, typename V> INTRA_UTIL_INLINE auto simd_add(V a, V b) {return Undefined;}
template<typename T, typename V> INTRA_UTIL_INLINE auto simd_sub(V a, V b) {return Undefined;}

template<typename T, typename V> INTRA_UTIL_INLINE auto simd_mul(V a, V b) {return Undefined;}

template<typename T, typename V> INTRA_UTIL_INLINE auto simd_div(V a, V b) {return Undefined;}

template<typename=void, typename V> INTRA_UTIL_INLINE auto simd_and(V a, V b) {return Undefined;}
template<typename=void, typename V> INTRA_UTIL_INLINE auto simd_or(V a, V b) {return Undefined;}
template<typename=void, typename V> INTRA_UTIL_INLINE auto simd_xor(V a, V b) {return Undefined;}
template<typename=void, typename V> INTRA_UTIL_INLINE auto simd_andnot(V a, V b) {return Undefined;} //NOTE: ~a & b (NOT a & ~b)

template<typename T, typename V> INTRA_UTIL_INLINE auto simd_cmpgt(V a, V b) {return Undefined;}
template<typename T, typename V> INTRA_UTIL_INLINE auto simd_cmpeq(V a, V b) {return Undefined;}
template<typename T, typename V> INTRA_UTIL_INLINE auto simd_cmplt(V a, V b) {return Undefined;}
template<typename T, typename V> INTRA_UTIL_INLINE auto simd_cmple(V a, V b) {return Undefined;}
template<typename T, typename V> INTRA_UTIL_INLINE auto simd_cmpge(V a, V b) {return Undefined;}
template<typename T, typename V> INTRA_UTIL_INLINE auto simd_cmpneq(V a, V b) {return Undefined;}

template<typename T, typename V> INTRA_UTIL_INLINE auto simd_cmpgt_mask(V a, V b) {return Undefined;}
template<typename T, typename V> INTRA_UTIL_INLINE auto simd_cmpeq_mask(V a, V b) {return Undefined;}


template<typename T, typename V> INTRA_UTIL_INLINE auto simd_shl(V a, V b) {return Undefined;}
template<typename T, typename V> INTRA_UTIL_INLINE auto simd_shl(V a, int b) {return Undefined;}
template<typename T, typename V> INTRA_UTIL_INLINE auto simd_shr(V a, V b) {return Undefined;}
template<typename T, typename V> INTRA_UTIL_INLINE auto simd_shr(V a, int b) {return Undefined;}

template<typename T, typename V> INTRA_UTIL_INLINE auto simd_min(V a, V b) {return Undefined;}
template<typename T, typename V> INTRA_UTIL_INLINE auto simd_max(V a, V b) {return Undefined;}

#ifdef __SSE__

#define INTRAZ_D_DECLARE_BINOP_FUNC(opname, mm_opname, V, bits, vecType, T) \
	INTRA_NON_GNU_EXT_CODE(extern "C" T INTRA_CRTDECL _mm##bits##_##mm_opname##_##vecType(V a, V b);) \
	template<> INTRA_UTIL_INLINE auto simd_##opname<T, V>(V a, V b) {return \
		_mm##bits##_##mm_opname##_##vecType(a, b);}

#define INTRAZ_D_DECLARE_BINOP_VS_FUNC(opname, mm_opname, V, bits, vecType, T) \
	INTRA_NON_GNU_EXT_CODE(extern "C" T INTRA_CRTDECL _mm##bits##_##mm_opname##_##vecType(V a, T b);) \
	template<> INTRA_UTIL_INLINE auto simd_##opname<T, V>(V a, T b) {return \
		_mm##bits##_##mm_opname##_##vecType(a, b);}

#define INTRAZ_D_DECLARE_BINOP_MASK_FUNC(opname, mm_opname, V, bits, vecType, T, numMaskBits) \
	extern "C" uint##numMaskBits __cdecl _mm##bits##_##mm_opname##_##vecType_mask(V a, V b); \
	template<> INTRA_UTIL_INLINE auto simd_##opname##_mask<T, V>(V a, V b) {return _mm##bits##_##mm_opname##_##vecType##_mask(a, b);}

// simd_add/simd_sub for all vector types //
////////////////////////////////////////////
#define INTRAZ_D_DECLARE_COMMON_BINOPS_FUNCS(V, bits, vecType, T) \
	INTRAZ_D_DECLARE_BINOP_FUNC(add, add, V, bits, vecType, T) \
	INTRAZ_D_DECLARE_BINOP_FUNC(sub, sub, V, bits, vecType, T)
#define INTRAZ_D_DECLARE_COMMON_BINOPS_FOR_ALL_TYPES(Tbits, bits) \
	INTRAZ_D_DECLARE_COMMON_BINOPS_FUNCS(__m##Tbits, bits, ps, float) \
	INTRAZ_D_DECLARE_COMMON_BINOPS_FUNCS(__m##Tbits##i, bits, epi8, int8) \
	INTRAZ_D_DECLARE_COMMON_BINOPS_FUNCS(__m##Tbits##i, bits, epi16, int16) \
	INTRAZ_D_DECLARE_COMMON_BINOPS_FUNCS(__m##Tbits##i, bits, epi32, int32) \
	INTRAZ_D_DECLARE_COMMON_BINOPS_FUNCS(__m##Tbits##i, bits, epi64, int64) \
	INTRAZ_D_DECLARE_COMMON_BINOPS_FUNCS(__m##Tbits##d, bits, pd, double)
INTRAZ_D_DECLARE_COMMON_BINOPS_FOR_ALL_TYPES(128,) //SSE/SSE2
INTRAZ_D_DECLARE_COMMON_BINOPS_FOR_ALL_TYPES(256, 256) //AVX/AVX2
INTRAZ_D_DECLARE_COMMON_BINOPS_FOR_ALL_TYPES(512, 512) //AVX512
#undef INTRAZ_D_DECLARE_COMMON_BINOPS_FOR_ALL_TYPES
#undef INTRAZ_D_DECLARE_COMMON_BINOPS_FUNCS

// simd_mul/simd_div for all FP vector types //
//////////////////////////////////////
#define INTRAZ_D_DECLARE_FLOAT_MUL_DIV(Vbase, bits)
	INTRAZ_D_DECLARE_BINOP_FUNC(mul, mul, Vbase, bits, ps, float) \
	INTRAZ_D_DECLARE_BINOP_FUNC(div, div, Vbase##d, bits, pd, double)
INTRAZ_D_DECLARE_FLOAT_MUL_DIV(__m128,); //SSE/SSE2
INTRAZ_D_DECLARE_FLOAT_MUL_DIV(__m256, 256); //AVX
INTRAZ_D_DECLARE_FLOAT_MUL_DIV(__m512, 512); //AVX512

#undef INTRAZ_D_DECLARE_FLOAT_MUL_DIV

// simd_mul for all 16/32/64-bit signed integral vector types //
////////////////////////////////////////////////////////////////
INTRAZ_D_DECLARE_BINOP_FUNC(mul, mullo, __m128i,, epi16, int16) //SSE2
INTRAZ_D_DECLARE_BINOP_FUNC(mul, mullo, __m128i,, epi32, int32) //SSE4.1
INTRAZ_D_DECLARE_BINOP_FUNC(mul, mullo, __m128i,, epi64, int64) //AVX512
INTRAZ_D_DECLARE_BINOP_FUNC(mul, mullo, __m256i, 256, epi16, int16) //AVX2
INTRAZ_D_DECLARE_BINOP_FUNC(mul, mullo, __m256i, 256, epi32, int32)
INTRAZ_D_DECLARE_BINOP_FUNC(mul, mullo, __m256i, 256, epi64, int64) //AVX512
INTRAZ_D_DECLARE_BINOP_FUNC(mul, mullo, __m512i, 512, epi16, int16)
INTRAZ_D_DECLARE_BINOP_FUNC(mul, mullo, __m512i, 512, epi32, int32)
INTRAZ_D_DECLARE_BINOP_FUNC(mul, mullo, __m512i, 512, epi64, int64)

// Declare intrinsics simd_and/simd_or/simd_xor/simd_andnot for all vector types //
///////////////////////////////////////////////////////////////////////////////////
#define INTRAZ_D_DECLARE_LOGOPS(V, bits, vecType) \
	INTRAZ_D_DECLARE_BINOP_FUNC(and, and, V, bits, vecType, void) \
	INTRAZ_D_DECLARE_BINOP_FUNC(or, or, V, bits, vecType, void) \
	INTRAZ_D_DECLARE_BINOP_FUNC(xor, xor, V, bits, vecType, void) \
	INTRAZ_D_DECLARE_BINOP_FUNC(andnot, andnot, V, bits, vecType, void)
INTRAZ_D_DECLARE_LOGOPS(__m128, , ps) //SSE
INTRAZ_D_DECLARE_LOGOPS(__m128d, , pd) //SSE2
INTRAZ_D_DECLARE_LOGOPS(__m256, 256, ps) //AVX
INTRAZ_D_DECLARE_LOGOPS(__m256d, 256, pd)
INTRAZ_D_DECLARE_LOGOPS(__m512, 512, ps)
INTRAZ_D_DECLARE_LOGOPS(__m512d, 512, pd)
INTRAZ_D_DECLARE_LOGOPS(__m128i, , si128) //SSE
INTRAZ_D_DECLARE_LOGOPS(__m256i, 256, si256) //AVX2
INTRAZ_D_DECLARE_LOGOPS(__m512i, 512, epi32) //AVX512

#undef INTRAZ_D_DECLARE_LOGOPS


#define INTRAZ_D_DECLARE_MIN_MAX(V, bits, vecType, T) \
	INTRAZ_D_DECLARE_BINOP_FUNC(min, min, V, bits, vecType, T) \
	INTRAZ_D_DECLARE_BINOP_FUNC(max, max, V, bits, vecType, T)

#define INTRAZ_D_DECLARE_MIN_MAX_FOR_ALL_TYPES(Vbase, bits) \
	INTRAZ_D_DECLARE_MIN_MAX(Vbase##i, bits, epi8, int8) \
	INTRAZ_D_DECLARE_MIN_MAX(Vbase##i, bits, epu8, uint8) \
	INTRAZ_D_DECLARE_MIN_MAX(Vbase##i, bits, epi16, int16) \
	INTRAZ_D_DECLARE_MIN_MAX(Vbase##i, bits, epu16, uint16) \
	INTRAZ_D_DECLARE_MIN_MAX(Vbase##i, bits, epi32, int32) \
	INTRAZ_D_DECLARE_MIN_MAX(Vbase##i, bits, epu32, uint32) \
	INTRAZ_D_DECLARE_MIN_MAX(Vbase##i, bits, epi64, int64) \
	INTRAZ_D_DECLARE_MIN_MAX(Vbase##i, bits, epu64, uint64) \
	INTRAZ_D_DECLARE_MIN_MAX(Vbase, bits, ps, float) \
	INTRAZ_D_DECLARE_MIN_MAX(Vbase##d, bits, pd, double)
INTRAZ_D_DECLARE_MIN_MAX_FOR_ALL_TYPES(__m128,)
INTRAZ_D_DECLARE_MIN_MAX_FOR_ALL_TYPES(__m256, 256)
INTRAZ_D_DECLARE_MIN_MAX_FOR_ALL_TYPES(__m512, 512)

#undef INTRAZ_D_DECLARE_MIN_MAX_FOR_ALL_TYPES
#undef INTRAZ_D_DECLARE_INT_MIN_MAX

// simd_shl/simd_shr for all 16/32/64-bit integral vector types with scalar or vector bitshift //
/////////////////////////////////////////////////////////////////////////////////////////////////
#define INTRAZ_D_DECLARE_SHIFT_OPS(V, bits, vecType, signedT) \
	INTRAZ_D_DECLARE_BINOP_VS_FUNC(shl, slli, V, bits, vecType, signedT) \
	INTRAZ_D_DECLARE_BINOP_VS_FUNC(shr, srai, V, bits, vecType, signedT) \
	INTRAZ_D_DECLARE_BINOP_VS_FUNC(shr, srli, V, bits, vecType, u##signedT) \
	INTRAZ_D_DECLARE_BINOP_FUNC(shl, sllv, V, bits, vecType, signedT) \
	INTRAZ_D_DECLARE_BINOP_FUNC(shr, srav, V, bits, vecType, signedT) \
	INTRAZ_D_DECLARE_BINOP_FUNC(shr, srlv, V, bits, vecType, u##signedT)
INTRAZ_D_DECLARE_SHIFT_OPS(__m128i,, epi16, int16);
INTRAZ_D_DECLARE_SHIFT_OPS(__m128i,, epi32, int32);
INTRAZ_D_DECLARE_SHIFT_OPS(__m128i,, epi64, int64);
INTRAZ_D_DECLARE_SHIFT_OPS(__m256i, 256, epi16, int16);
INTRAZ_D_DECLARE_SHIFT_OPS(__m256i, 256, epi32, int32);
INTRAZ_D_DECLARE_SHIFT_OPS(__m256i, 256, epi64, int64);
INTRAZ_D_DECLARE_SHIFT_OPS(__m512i, 512, epi16, int16);
INTRAZ_D_DECLARE_SHIFT_OPS(__m512i, 512, epi32, int32);
INTRAZ_D_DECLARE_SHIFT_OPS(__m512i, 512, epi64, int64);
#undef INTRAZ_D_DECLARE_SHIFT_OPS

// Declare intrinsics simd_cmp(gt|eq|[lt|le|ge|neq]) for all SSE/AVX integral and SSE FP types //
/////////////////////////////////////////////////////////////////////////////////////////////////
#define INTRAZ_D_DECLARE_SSE_AVX2_INT_CMPOPS(V, bits, vecType, T) \
	INTRAZ_D_DECLARE_BINOP_FUNC(cmpgt, cmpgt, V, bits, vecType, T) \
	INTRAZ_D_DECLARE_BINOP_FUNC(cmpeq, cmpeq, V, bits, vecType, T)
INTRAZ_D_DECLARE_SSE_AVX2_INT_CMPOPS(__m128i, , epi8, int8) //SSE2
INTRAZ_D_DECLARE_SSE_AVX2_INT_CMPOPS(__m128i, , epi16, int16)
INTRAZ_D_DECLARE_SSE_AVX2_INT_CMPOPS(__m128i, , epi32, int32)
INTRAZ_D_DECLARE_SSE_AVX2_INT_CMPOPS(__m128i, , epi64, int64) //SSE 4.1
INTRAZ_D_DECLARE_SSE_AVX2_INT_CMPOPS(__m256i, 256, epi8, int8) //AVX
INTRAZ_D_DECLARE_SSE_AVX2_INT_CMPOPS(__m256i, 256, epi16, int16)
INTRAZ_D_DECLARE_SSE_AVX2_INT_CMPOPS(__m256i, 256, epi32, int32)
INTRAZ_D_DECLARE_SSE_AVX2_INT_CMPOPS(__m256i, 256, epi64, int64)
#undef INTRAZ_D_DECLARE_SSE_AVX2_INT_CMPOPS

#define INTRAZ_D_DECLARE_SSE_AVX_FLOAT_CMPOPS(V, bits, vecType, T) \
	INTRAZ_D_DECLARE_BINOP_FUNC(cmpgt, cmpgt, V, bits, vecType, T) \
	INTRAZ_D_DECLARE_BINOP_FUNC(cmpeq, cmpeq, V, bits, vecType, T) \
	INTRAZ_D_DECLARE_BINOP_FUNC(cmplt, cmplt, V, bits, vecType, T) \
	INTRAZ_D_DECLARE_BINOP_FUNC(cmple, cmple, V, bits, vecType, T) \
	INTRAZ_D_DECLARE_BINOP_FUNC(cmpge, cmpge, V, bits, vecType, T) \
	INTRAZ_D_DECLARE_BINOP_FUNC(cmpneq, cmpneq, V, bits, vecType, T)
INTRAZ_D_DECLARE_SSE_AVX_FLOAT_CMPOPS(__m128, , ps, float) //SSE
INTRAZ_D_DECLARE_SSE_AVX_FLOAT_CMPOPS(__m128d, , pd, double) //SSE2
#undef INTRAZ_D_DECLARE_SSE_AVX_FLOAT_CMPOPS

// Declare intrinsics simd_cmp(gt|eq)_mask for all AVX512 integral types //
///////////////////////////////////////////////////////////////////////////
#define INTRAZ_D_DECLARE_AVX512_INT_CMPOPS(vecType, numMaskBits) \
	INTRAZ_D_DECLARE_BINOP_MASK_FUNC(cmpgt, cmpgt, __m512i, 512, vecType, numMaskBits) \
	INTRAZ_D_DECLARE_BINOP_MASK_FUNC(cmpeq, cmpeq, __m512i, 512, vecType, numMaskBits)
INTRAZ_D_DECLARE_AVX512_INT_CMPOPS(epi8, 64)
INTRAZ_D_DECLARE_AVX512_INT_CMPOPS(epu8, 64)
INTRAZ_D_DECLARE_AVX512_INT_CMPOPS(epi16, 32)
INTRAZ_D_DECLARE_AVX512_INT_CMPOPS(epu16, 32)
INTRAZ_D_DECLARE_AVX512_INT_CMPOPS(epi32, 16)
INTRAZ_D_DECLARE_AVX512_INT_CMPOPS(epu32, 16)
INTRAZ_D_DECLARE_AVX512_INT_CMPOPS(epi64, 8)
INTRAZ_D_DECLARE_AVX512_INT_CMPOPS(epu64, 8)
#undef INTRAZ_D_DECLARE_AVX512_INT_CMPOPS

#elif defined(__ARM_NEON)
//TODO
#endif

#define INTRAZ_D_SIMD_OPERATOR_SCALAR_IMPL(op) T arr1[N], arr2[N]; SimdStore(arr1, *this); SimdStore(arr2, rhs); \
	for(int i = 0; i < N; i++) arr1[i] op= arr2[i]; \
	return SimdLoad<N>(arr1);

#define INTRAZ_D_SIMD_OPERATOR_SCALAR_IMPL_SCALAR(op) T arr1[N]; SimdStore(arr1, *this); \
	for(int i = 0; i < N; i++) arr1[i] op= rhs; \
	return SimdLoad<N>(arr1);
}

template<typename T, size_t N> struct SimdVector;
template<size_t N, typename T> INTRA_FORCEINLINE SimdVector<T, N> SimdLoadAligned(const T* ptr) noexcept;
template<typename T, size_t N> INTRA_FORCEINLINE void SimdStoreAligned(T* dst, SimdVector<T, N> v) noexcept;
template<size_t N, typename T> INTRA_FORCEINLINE SimdVector<T, N> SimdLoad(const T* ptr) noexcept;
template<typename T, size_t N> INTRA_FORCEINLINE void SimdStore(T* dst, SimdVector<T, N> v) noexcept;

template<typename T, size_t N>
INTRA_FORCEINLINE SimdVector<T, N> SimdVectorFilled(T valueToReplicate) noexcept;

template<size_t i0, size_t i1, size_t i2, size_t i3, typename T>
INTRA_FORCEINLINE SimdVector<T, 4> SimdShuffle(SimdVector<T, 4> v) noexcept;

template<typename T, size_t N>
INTRA_FORCEINLINE SimdVector<T, N> SimdSelect(
	SimdVector<T, N> ifFalse, SimdVector<T, N> ifTrue, SimdVector<TToIntegral<T>, N> broadBoolMask) noexcept;

template<typename T, size_t N> struct SimdVector
{
	static_assert(CBasicIntegral<T> || CBasicFloatingPoint<T>);
	static_assert(!CSame<T, long double>, "Use double instead of long double.");
	static_assert(!CChar<T>, "Use integer types instead of char/char8_t/char16_t/char32_t.");
	
private:
	using V = TPackAt<sizeof(T)*N / 16 - 1,
		TPackAt<CBasicIntegral<T>? 0: CSame<float, T>? 1: 2, z_D::__m128i, z_D::__m128, z_D::__m128d>,
		TPackAt<CBasicIntegral<T>? 0: CSame<float, T>? 1: 2, z_D::__m256i, z_D::__m256, z_D::__m256d>,
		void,
		TPackAt<CBasicIntegral<T>? 0: CSame<float, T>? 1: 2, z_D::__m512i, z_D::__m512, z_D::__m512d>
	>;

	static constexpr bool SupportedByTarget =
		(sizeof(V) == 16 ||
			sizeof(V) == 32 && (CBasicIntegral<T>? Config::TargetHasAVX2: Config::TargetHasAVX) ||
			sizeof(V) == 64 && Config::TargetHasAVX512F
		) &&
		!CVoid<V>;


	using TInt = TToIntegral<T>;
	using EquivIntVector = SimdVector<TInt>;

public:
	V mV;

	SimdVector() = default;
	template<typename V1> requires(sizeof(V1) == sizeof(V) && alignof(V1) == alignof(V))
	explicit INTRA_FORCEINLINE SimdVector(V1 v) noexcept: mV(reinterpret_cast<V&>(v)) {}

	template<typename V1> requires(sizeof(V1) == sizeof(V) && alignof(V1) == alignof(V))
	explicit INTRA_FORCEINLINE operator V1() const noexcept {return reinterpret_cast<V1&>(mV);}

	explicit INTRA_FORCEINLINE SimdVector(T x0, T x1=0) noexcept requires(N == 2)
	{
		if constexpr(CSame<T, double>) mV = z_D::_mm_set_pd(x1, x0);
		else if constexpr(sizeof(T) == sizeof(int64)) mV = z_D::_mm_set_epi64x(int64(x1), int64(x0));
		else static_assert(false);
	}

	explicit INTRA_FORCEINLINE SimdVector(T x0, T x1=0, T x2=0, T x3=0) noexcept requires(N == 4)
	{
		if constexpr(CSame<T, float>) mV = z_D::_mm_set_ps(x3, x2, x1, x0);
		else if constexpr(CSame<T, double>) mV = z_D::_mm256_set_pd(x3, x2, x1, x0);
		else if constexpr(sizeof(T) == sizeof(int64)) mV = z_D::_mm256_set_epi64x(int64(x3), int64(x2), int64(x1), int64(x0));
		else if constexpr(sizeof(T) == sizeof(int)) mV = z_D::_mm_set_epi32(int(x3), int(x2), int(x1), int(x0));
		else static_assert(false);
	}

	explicit INTRA_FORCEINLINE SimdVector(T x0, T x1=0, T x2=0, T x3=0, T x4=0, T x5=0, T x6=0, T x7=0) noexcept requires(N == 8)
	{
		if constexpr(CSame<T, float>) mV = z_D::_mm256_set_ps(x7, x6, x5, x4, x3, x2, x1, x0);
		else if constexpr(CSame<T, double>)
			mV = z_D::_mm512_set_pd(x7, x6, x5, x4, x3, x2, x1, x0);
		else if constexpr(sizeof(T) == sizeof(int64))
			mV = z_D::_mm512_set_epi64(int64(x7), int64(x6), int64(x5), int64(x4), int64(x3), int64(x2), int64(x1), int64(x0));
		else if constexpr(sizeof(T) == sizeof(int))
			mV = z_D::_mm256_set_epi32(int(x7), int(x6), int(x5), int(x4), int(x3), int(x2), int(x1), int(x0));
		else if constexpr(sizeof(T) == sizeof(short))
			mV = z_D::_mm_set_epi16(short(x7), short(x6), short(x5), short(x4), short(x3), short(x2), short(x1), short(x0));
		else static_assert(false);
	}

	explicit INTRA_FORCEINLINE SimdVector(
		T x0, T x1=0, T x2=0, T x3=0, T x4=0, T x5=0, T x6=0, T x7=0,
		T x8=0, T x9=0, T x10=0, T x11=0, T x12=0, T x13=0, T x14=0, T x15=0) noexcept requires(N == 16)
	{
		if constexpr(CSame<T, float>)
			mV = z_D::_mm512_set_ps(x15, x14, x13, x12, x11, x10, x9, x8, x7, x6, x5, x4, x3, x2, x1, x0);
		else if constexpr(sizeof(T) == sizeof(int))
			mV = z_D::_mm512_set_epi32(
				int(x15), int(x14), int(x13), int(x12), int(x11), int(x10), int(x9), int(x8),
				int(x7), int(x6), int(x5), int(x4), int(x3), int(x2), int(x1), int(x0));
		else if constexpr(sizeof(T) == sizeof(short))
			mV = z_D::_mm256_set_epi16(
				short(x15), short(x14), short(x13), short(x12), short(x11), short(x10), short(x9), short(x8),
				short(x7), short(x6), short(x5), short(x4), short(x3), short(x2), short(x1), short(x0));
		else if constexpr(sizeof(T) == sizeof(char))
			mV = z_D::_mm_set_epi8(char(x15), char(x14), char(x13), char(x12), char(x11), char(x10), char(x9), char(x8),
				char(x7), char(x6), char(x5), char(x4), char(x3), char(x2), char(x1), char(x0));
		else static_assert(false);
	}

	explicit INTRA_FORCEINLINE SimdVector(
		T x0, T x1=0, T x2=0, T x3=0, T x4=0, T x5=0, T x6=0, T x7=0,
		T x8=0, T x9=0, T x10=0, T x11=0, T x12=0, T x13=0, T x14=0, T x15=0,
		T x16=0, T x17=0, T x18=0, T x19=0, T x20=0, T x21=0, T x22=0, T x23=0,
		T x24=0, T x25=0, T x26=0, T x27=0, T x28=0, T x29=0, T x30=0, T x31=0) noexcept requires(N == 32)
	{
		if constexpr(sizeof(T) == sizeof(short))
			mV = z_D::_mm512_set_epi16(
				short(x31), short(x30), short(x29), short(x28), short(x27), short(x26), short(x25), short(x24),
				short(x23), short(x22), short(x21), short(x20), short(x19), short(x18), short(x17), short(x16),
				short(x15), short(x14), short(x13), short(x12), short(x11), short(x10), short(x9), short(x8),
				short(x7), short(x6), short(x5), short(x4), short(x3), short(x2), short(x1), short(x0));
		else if constexpr(sizeof(T) == sizeof(char))
			mV = z_D::_mm256_set_epi8(
				char(x31), char(x30), char(x29), char(x28), char(x27), char(x26), char(x25), char(x24),
				char(x23), char(x22), char(x21), char(x20), char(x19), char(x18), char(x17), char(x16),
				char(x15), char(x14), char(x13), char(x12), char(x11), char(x10), char(x9), char(x8),
				char(x7), char(x6), char(x5), char(x4), char(x3), char(x2), char(x1), char(x0));
		else static_assert(false);
	}

	

	INTRA_FORCEINLINE SimdVector operator+(SimdVector rhs) const noexcept
	{
		if constexpr(Config::TargetMaxSimdLength<T> >= N &&
			!CSame<decltype(z_D::simd_add<T>(mV, rhs.mV)), TUndefined>)
			return SimdVector(z_D::simd_add<TToSigned<T>>(mV, rhs.mV));
		INTRAZ_D_SIMD_OPERATOR_SCALAR_IMPL(+);
	}

	INTRA_FORCEINLINE SimdVector operator-(SimdVector rhs) const noexcept
	{
		if constexpr(Config::TargetMaxSimdLength<T> >= N &&
			!CSame<decltype(z_D::simd_sub<T>(mV, rhs.mV)), TUndefined>)
			return SimdVector(z_D::simd_sub<TToSigned<T>>(mV, rhs.mV));
		INTRAZ_D_SIMD_OPERATOR_SCALAR_IMPL(-);
	}

	INTRA_FORCEINLINE SimdVector operator*(SimdVector rhs) const noexcept
	{
		if constexpr(Config::TargetMaxSimdLength<T> >= N)
		{
		#ifdef __SSE__
			if constexpr(!CBasicSigned<T>) return SimdVector<TToSigned<T>, N>(*this) * SimdVector<TToSigned<T>, N>(rhs);
			if constexpr(!Config::TargetHasAVX512F && CSame<T, int64> && N == 2)
			{
				const auto mul = SimdVector<uint64, N>(z_D::_mm_mul_epu32(mV, rhs.mV));
				const auto s1 = SimdShuffle<1, 0, 3, 2>(SimdVector<uint32, 4>(*this));
				const auto s2 = SimdShuffle<1, 0, 3, 2>(SimdVector<uint32, 4>(rhs));
				const auto mul1 = SimdVector<uint64, N>(z_D::_mm_mul_epu32(rhs.mV, s1.mV));
				const auto mul2 = SimdVector<uint64, N>(z_D::_mm_mul_epu32(mV, s2.mV));
				return SimdVector(mul + ((mul1 + mul2) << 32));
			}
			else if constexpr(!Config::TargetHasSSE41 && CSame<T, int32> && N == 4)
			{
				const auto mul1 = z_D::_mm_mul_epu32(mV, rhs.mV);
				const auto mul2 = z_D::_mm_mul_epu32(z_D::_mm_srli_si128(mV, 4), z_D::_mm_srli_si128(rhs.mV, 4));
				const auto s1 = SimdShuffle<0, 2, 0, 0>(mul1);
				const auto s2 = SimdShuffle<0, 2, 0, 0>(mul2);
				return SimdVector(z_D::_mm_unpacklo_epi32(s1.mV, s2.mV));
			}
			else if constexpr(CSame<T, int8>)
			{
				const auto a16 = SimdVector<uint16, N/2>(*this);
				const auto b16 = SimdVector<uint16, N/2>(rhs);
				const auto dstEven = a16 * b16;
				const auto dstOdd = (a16 >> 8) * (b16 >> 8));
				SimdVector<uint16, N/2> mask;
				if constexpr(Config::TargetHasAVX2)
					mask = (dstEven & SimdVectorFilled<N/2, uint16>(0xFF)); //AVX2?
				else mask = (dstEven << 8) >> 8;
				return SimdVector((dstOdd << 8) | mask);
			}
			else if constexpr(!Config::TargetHasAVX512F && CSame<T, int64> && N == 4)
			{
				//TODO: implement using AVX2
			}
			else
		#endif
				if constexpr(!CSame<decltype(z_D::simd_mul<T>(mV, rhs.mV)), TUndefined>)
					return SimdVector(z_D::simd_mul<T>(mV, rhs.mV));
		}
		INTRAZ_D_SIMD_OPERATOR_SCALAR_IMPL(*);
	}

	INTRA_FORCEINLINE SimdVector operator/(SimdVector rhs) const
	{
		if constexpr(Config::TargetMaxSimdLength<T> >= N &&
			!CSame<decltype(z_D::simd_div<T>(mV, rhs.mV)), TUndefined>)
			return SimdVector(z_D::simd_div<T>(mV, rhs.mV));
		INTRAZ_D_SIMD_OPERATOR_SCALAR_IMPL(/);
	}

	INTRA_FORCEINLINE SimdVector operator%(SimdVector rhs) const requires CBasicIntegral<T>
	{
		INTRAZ_D_SIMD_OPERATOR_SCALAR_IMPL(%);
	}

	INTRA_FORCEINLINE SimdVector operator&(SimdVector rhs) const noexcept requires CBasicIntegral<T>
	{
		if constexpr(Config::TargetMaxSimdLength<T> >= N &&
			!CSame<decltype(z_D::simd_and<void>(mV, rhs.mV)), TUndefined>)
			return SimdVector(z_D::simd_and<void>(mV, rhs.mV));
		INTRAZ_D_SIMD_OPERATOR_SCALAR_IMPL(&);
	}
	INTRA_FORCEINLINE SimdVector operator|(SimdVector rhs) const noexcept requires CBasicIntegral<T>
	{
		if constexpr(Config::TargetMaxSimdLength<T> >= N &&
			!CSame<decltype(z_D::simd_or<void>(mV, rhs.mV)), TUndefined>)
			return SimdVector(z_D::simd_or<void>(mV, rhs.mV));
		INTRAZ_D_SIMD_OPERATOR_SCALAR_IMPL(|);
	}
	INTRA_FORCEINLINE SimdVector operator^(SimdVector rhs) const noexcept requires CBasicIntegral<T>
	{
		if constexpr(Config::TargetMaxSimdLength<T> >= N &&
			!CSame<decltype(z_D::simd_xor<void>(mV, rhs.mV)), TUndefined>)
			return SimdVector(z_D::simd_xor<void>(mV, rhs.mV));
		INTRAZ_D_SIMD_OPERATOR_SCALAR_IMPL(^);
	}
	INTRA_FORCEINLINE SimdVector operator~() const noexcept requires CBasicIntegral<T>
	{
		return *this ^ T(-1);
	}


	INTRA_FORCEINLINE SimdVector operator<<(int bits) const noexcept requires CBasicIntegral<T>
	{
		if constexpr(Config::TargetMaxSimdLength<T> >= N)
		{
			if constexpr(sizeof(T) == sizeof(int8))
			{
				return SimdVector(SimdVector<uint16, N/2>(mV) << bits) & SimdVectorFilled<uint8, N>(0xFFu << (bits & 15)));
			}
			else if constexpr(!CSame<decltype(z_D::simd_shl<void>(mV, rhs.mV)), TUndefined>)
				return SimdVector(z_D::simd_shl<TToSigned<T>>(mV, rhs.mV));
		}
		INTRAZ_D_SIMD_OPERATOR_SCALAR_IMPL_SCALAR(<<);
	}

	INTRA_FORCEINLINE SimdVector operator>>(int bits) const noexcept requires CBasicIntegral<T>
	{
		if constexpr(Config::TargetMaxSimdLength<T> >= N)
		{
			if constexpr(CSame<T, int8>)
			{
				return SimdVector(SimdVector<uint16, N/2>(mV) << bits) & SimdVectorFilled<uint8, N>(0xFFu >> bits));
			}
			else if constexpr(CSame<T, uint8>)
			{
				if constexpr(N == 16) return SimdVector(z_D::_mm_packs_epi16(
					SimdVector<int16, N/2>(z_D::_mm_unpacklo_epi8(mV, mV)) >> (8 + bits),
					SimdVector<int16, N/2>(z_D::_mm_unpackhi_epi8(mV, mV)) >> (8 + bits)
				));
				else if constexpr(N == 32) return SimdVector(z_D::_mm256_packs_epi16(
					SimdVector<int16, N/2>(z_D::_mm256_unpacklo_epi8(mV, mV)) >> (8 + bits),
					SimdVector<int16, N/2>(z_D::_mm256_unpackhi_epi8(mV, mV)) >> (8 + bits)
				));
				else if constexpr(N == 64) return SimdVector(z_D::_mm512_packs_epi16(
					SimdVector<int16, N/2>(z_D::_mm512_unpacklo_epi8(mV, mV)) >> (8 + bits),
					SimdVector<int16, N/2>(z_D::_mm512_unpackhi_epi8(mV, mV)) >> (8 + bits)
				));
			}
			else if constexpr(CSame<T, int64> && !Config::TargetHasAVX512F)
			{
				if constexpr(N == 2)
				{
					if(bits <= 32)
					{
						const auto sra = SimdVector<int32, N*2>(*this) >> bits;  // a >> b signed dwords
						const auto srl = SimdVector<uint64, N>(*this) >> bits;   // a >> b unsigned qwords
						return SimdSelect(SimdVector(srl), SimdVector(sra), SimdVector(SimdVector<int32, N*2>{0, -1, 0, -1})); // mask for signed high part
					}
					else
					{
						const auto sign = SimdVector<int32, N*2>(*this) >> 31;       // sign of a
						const auto sra2 = SimdVector<int32, N*2>(*this) >> (b - 32); // a >> (b-32) signed dwords
						const auto sra3 = SimdVector<uint64, N>(sra2) >> 32;         // a >> (b-32) >> 32 (second shift unsigned qword)
						return SimdSelect(SimdVector(sra3), SimdVector(sign), SimdVector(SimdVector<int32, N*2>{0, -1, 0, -1})); // mask for high part containing only sign
					}
				}
			}
			else if constexpr(!CSame<decltype(z_D::simd_shr<void>(mV, rhs.mV)), TUndefined>)
				return SimdVector(z_D::simd_shl<TToSigned<T>>(mV, rhs.mV));
		}
		INTRAZ_D_SIMD_OPERATOR_SCALAR_IMPL_SCALAR(>>);
	}

	INTRA_FORCEINLINE SimdVector operator<<(SimdVector rhs) const noexcept requires CBasicIntegral<T>
	{
		if constexpr(Config::TargetMaxSimdLength<T> >= N)
		{
			if constexpr(!Config::TargetHasAVX2) {}
			else if constexpr(!Config::TargetHasAVX512F && sizeof(T) == sizeof(int16)) {}
			else if constexpr(!CSame<decltype(z_D::simd_shl<T>(mV, rhs.mV)), TUndefined>)
				return SimdVector(z_D::simd_shl<T>(mV, rhs.mV));
		}
		INTRAZ_D_SIMD_OPERATOR_SCALAR_IMPL(<<);
	}

	INTRA_FORCEINLINE SimdVector operator>>(SimdVector rhs) const noexcept requires CBasicIntegral<T>
	{
		if constexpr(Config::TargetMaxSimdLength<T> >= N)
		{
			if constexpr(!Config::TargetHasAVX2) {}
			else if constexpr(!Config::TargetHasAVX512F && sizeof(T) == sizeof(int16)) {}
			else if constexpr(!CSame<decltype(z_D::simd_shr<T>(mV, rhs.mV)), TUndefined>)
				return SimdVector(z_D::simd_shr<T>(mV, rhs.mV));
		}
		INTRAZ_D_SIMD_OPERATOR_SCALAR_IMPL(>>);
	}

	INTRA_FORCEINLINE EquivIntVector operator>(SimdVector rhs) const noexcept
	{
		if constexpr(Config::TargetMaxSimdLength<T> >= N)
		{
			if constexpr(!CBasicSigned<T>)
			{
				const auto signMask = SimdVectorFilled<T, N>(SignBitMaskOf<T>);
				return SimdVector<TToSigned<T>, N>((*this ^ signMask) > (rhs ^ signMask));
			}
			else if constexpr(!Config::TargetHasSSE42)
			{
				return SimdShuffle<1, 1, 3, 3>(
					(EquivIntVector(z_D::simd_andnot<void>(rhs.mV, mV)) |
						EquivIntVector(z_D::simd_andnot<void>((*this ^ rhs).mV, (*this - rhs).mV))) >> 31);
			}
			else if constexpr(sizeof(SimdVector) == 32 && CBasicFloatingPoint<T>)
			{
				if constexpr(CSame<T, float>) return EquivIntVector(z_D::_mm256_cmp_ps(mV, rhs.mV, 30)); //ordered nonsignaling
				else if constexpr(CSame<T, double>) return EquivIntVector(z_D::_mm256_cmp_pd(mV, rhs.mV, 30));
			}
			else if constexpr(sizeof(SimdVector) == 64)
			{
				if constexpr(CSame<T, float>) return EquivIntVector(z_D::_mm512_movm_epi32(z_D::_mm512_cmp_ps_mask(mV, rhs.mV, 30)));
				else if constexpr(CSame<T, double>) return EquivIntVector(z_D::_mm512_movm_epi64(z_D::_mm512_cmp_pd_mask(mV, rhs.mV, 30)));
				else if constexpr(CSame<T, int8>) return EquivIntVector(z_D::_mm512_movm_epi8(z_D::simd_cmpgt_mask<T>(mV, rhs.mV)));
				else if constexpr(CSame<T, int16>) return EquivIntVector(z_D::_mm512_movm_epi16(z_D::simd_cmpgt_mask<T>(mV, rhs.mV)));
				else if constexpr(CSame<T, int32>) return EquivIntVector(z_D::_mm512_movm_epi32(z_D::simd_cmpgt_mask<T>(mV, rhs.mV)));
				else if constexpr(CSame<T, int64>) return EquivIntVector(z_D::_mm512_movm_epi64(z_D::simd_cmpgt_mask<T>(mV, rhs.mV)));
			}
			else if constexpr(!CSame<decltype(z_D::simd_cmpgt<T>(mV, rhs.mV)), TUndefined>)
				return EquivIntVector(z_D::simd_cmpgt<T>(mV, rhs.mV));
		}
		INTRAZ_D_SIMD_OPERATOR_SCALAR_IMPL(>);
	}

	INTRA_FORCEINLINE EquivIntVector operator<(SimdVector rhs) const noexcept
	{
		if constexpr(CBasicIntegral<T>) return rhs > *this;
		else if constexpr(sizeof(SimdVector) == 16)
		{
			if constexpr(CSame<T, float>) return EquivIntVector(z_D::_mm_cmplt_ps(mV, rhs.mV));
			else return EquivIntVector(z_D::_mm_cmplt_pd(mV, rhs.mV));
		}
		else if constexpr(sizeof(SimdVector) == 32)
		{
			if constexpr(CSame<T, float>) return EquivIntVector(z_D::_mm256_cmp_ps(mV, rhs.mV, 17)); //ordered nonsignaling
			else return EquivIntVector(z_D::_mm256_cmp_pd(mV, rhs.mV, 17));
		}
		else if constexpr(sizeof(SimdVector) == 64)
		{
			if constexpr(CSame<T, float>) return EquivIntVector(z_D::_mm512_movm_epi32(z_D::_mm512_cmp_ps_mask(mV, rhs.mV, 17)));
			else return EquivIntVector(z_D::_mm512_movm_epi64(z_D::_mm512_cmp_pd_mask(mV, rhs.mV, 17)));
		}
	}
	INTRA_FORCEINLINE EquivIntVector operator<=(SimdVector rhs) const noexcept
	{
		if constexpr(CBasicIntegral<T>) return ~(*this > rhs);
		else if constexpr(sizeof(SimdVector) == 16)
		{
			if constexpr(CSame<T, float>) return EquivIntVector(z_D::_mm_cmple_ps(mV, rhs.mV));
			else return EquivIntVector(z_D::_mm_cmple_pd(mV, rhs.mV));
		}
		else if constexpr(sizeof(SimdVector) == 32)
		{
			if constexpr(CSame<T, float>) return EquivIntVector(z_D::_mm256_cmp_ps(mV, rhs.mV, 18)); //ordered nonsignaling
			else return EquivIntVector(z_D::_mm256_cmp_pd(mV, rhs.mV, 18));
		}
		else if constexpr(sizeof(SimdVector) == 64)
		{
			if constexpr(CSame<T, float>) return EquivIntVector(z_D::_mm512_movm_epi32(z_D::_mm512_cmp_ps_mask(mV, rhs.mV, 18)));
			else return EquivIntVector(z_D::_mm512_movm_epi64(z_D::_mm512_cmp_pd_mask(mV, rhs.mV, 18)));
		}
	}
	INTRA_FORCEINLINE EquivIntVector operator>=(SimdVector rhs) const noexcept
	{
		if constexpr(CBasicIntegral<T>) return ~(rhs > *this);
		else if constexpr(sizeof(SimdVector) == 16)
		{
			if constexpr(CSame<T, float>) return EquivIntVector(z_D::_mm_cmpge_ps(mV, rhs.mV));
			else return EquivIntVector(z_D::_mm_cmpge_pd(mV, rhs.mV));
		}
		else if constexpr(sizeof(SimdVector) == 32)
		{
			if constexpr(CSame<T, float>) return EquivIntVector(z_D::_mm256_cmp_ps(mV, rhs.mV, 29)); //ordered nonsignaling
			else return EquivIntVector(z_D::_mm256_cmp_pd(mV, rhs.mV, 29));
		}
		else if constexpr(sizeof(SimdVector) == 64)
		{
			if constexpr(CSame<T, float>) return EquivIntVector(z_D::_mm512_movm_epi32(z_D::_mm512_cmp_ps_mask(mV, rhs.mV, 29)));
			else return EquivIntVector(z_D::_mm512_movm_epi64(z_D::_mm512_cmp_pd_mask(mV, rhs.mV, 29)));
		}
	}

	INTRA_FORCEINLINE EquivIntVector operator==(SimdVector rhs) const noexcept
	{
		if constexpr(Config::TargetMaxSimdLength<T> >= N)
		{
			if constexpr(!CBasicSigned<T>) return SimdVector<TToSigned<T>, N>(*this) == SimdVector<TToSigned<T>, N>(rhs);
			else if constexpr(!Config::TargetHasSSE42 && CSame<T, int64>)
			{
				const auto dwordsEqual  = SimdVector<int32, 4>(*this) == SimdVector<int32, 4>(rhs);
				const auto swappedLowHighDwords = SimdShuffle<1, 0, 3, 2>(dwordsEqual); // swap low and high dwords
				return EquivIntVector(SimdShuffle<1, 1, 3, 3>((dwordsEqual & swappedLowHighDwords) >> 31));
			}
			else if constexpr(sizeof(SimdVector) == 32 && CBasicFloatingPoint<T>)
			{
				if constexpr(CSame<T, float>) return EquivIntVector(z_D::_mm256_cmp_ps(mV, rhs.mV, 0)); //ordered nonsignaling
				else if constexpr(CSame<T, double>) return EquivIntVector(z_D::_mm256_cmp_pd(mV, rhs.mV, 0));
			}
			else if constexpr(sizeof(SimdVector) == 64)
			{
				if constexpr(CSame<T, float>) return EquivIntVector(z_D::_mm512_movm_epi32(z_D::_mm512_cmp_ps_mask(mV, rhs.mV, 0)));
				else if constexpr(CSame<T, double>) return EquivIntVector(z_D::_mm512_movm_epi64(z_D::_mm512_cmp_pd_mask(mV, rhs.mV, 0)));
				else if constexpr(CSame<T, int8>) return EquivIntVector(z_D::_mm512_movm_epi8(z_D::simd_cmpeq_mask<T>(mV, rhs.mV)));
				else if constexpr(CSame<T, int16>) return EquivIntVector(z_D::_mm512_movm_epi16(z_D::simd_cmpeq_mask<T>(mV, rhs.mV)));
				else if constexpr(CSame<T, int32>) return EquivIntVector(z_D::_mm512_movm_epi32(z_D::simd_cmpeq_mask<T>(mV, rhs.mV)));
				else if constexpr(CSame<T, int64>) return EquivIntVector(z_D::_mm512_movm_epi64(z_D::simd_cmpeq_mask<T>(mV, rhs.mV)));
			}
			else if constexpr(!CSame<decltype(z_D::simd_cmpeq<T>(mV, rhs.mV)), TUndefined>)
				return EquivIntVector(z_D::simd_cmpeq<T>(mV, rhs.mV));
		}
		INTRAZ_D_SIMD_OPERATOR_SCALAR_IMPL(==);
	}
	INTRA_FORCEINLINE EquivIntVector operator!=(SimdVector rhs) const noexcept
	{
		if constexpr(CBasicIntegral<T>) return ~(*this == rhs);
		else if constexpr(sizeof(SimdVector) == 16)
		{
			if constexpr(CSame<T, float>) return EquivIntVector(z_D::_mm_cmpneq_ps(mV, rhs.mV));
			else return EquivIntVector(z_D::_mm_cmpneq_pd(mV, rhs.mV));
		}
		else if constexpr(sizeof(SimdVector) == 32)
		{
			if constexpr(CSame<T, float>) return EquivIntVector(z_D::_mm256_cmp_ps(mV, rhs.mV, 12)); //ordered nonsignaling
			else return EquivIntVector(z_D::_mm256_cmp_pd(mV, rhs.mV, 12));
		}
		else if constexpr(sizeof(SimdVector) == 64)
		{
			if constexpr(CSame<T, float>) return EquivIntVector(z_D::_mm512_movm_epi32(z_D::_mm512_cmp_ps_mask(mV, rhs.mV, 12)));
			else return EquivIntVector(z_D::_mm512_movm_epi64(z_D::_mm512_cmp_pd_mask(mV, rhs.mV, 12)));
		}
	}

	INTRA_FORCEINLINE EquivIntVector operator-() noexcept {return T(0) - *this;}
	INTRA_FORCEINLINE EquivIntVector operator!() noexcept {return *this == T(0);}
	INTRA_FORCEINLINE EquivIntVector operator&&(SimdVector rhs) noexcept {return (*this != T(0)) & (rhs != T(0));}
	INTRA_FORCEINLINE EquivIntVector operator||(SimdVector rhs) noexcept {return (*this != T(0)) | (rhs != T(0));}


	INTRA_FORCEINLINE SimdVector operator+(T rhs) noexcept {return *this + SimdVectorFilled<T, N>(rhs);}
	INTRA_FORCEINLINE SimdVector operator-(T rhs) noexcept {return *this - SimdVectorFilled<T, N>(rhs);}
	INTRA_FORCEINLINE SimdVector operator*(T rhs) noexcept {return *this * SimdVectorFilled<T, N>(rhs);}
	INTRA_FORCEINLINE SimdVector operator/(T rhs) {return *this / SimdVectorFilled<T, N>(rhs);}

	INTRA_FORCEINLINE EquivIntVector operator>(T rhs) {return *this > SimdVectorFilled<T, N>(rhs);}
	INTRA_FORCEINLINE EquivIntVector operator<(T rhs) {return *this < SimdVectorFilled<T, N>(rhs);}
	INTRA_FORCEINLINE EquivIntVector operator>=(T rhs) {return *this >= SimdVectorFilled<T, N>(rhs);}
	INTRA_FORCEINLINE EquivIntVector operator<=(T rhs) {return *this <= SimdVectorFilled<T, N>(rhs);}
	INTRA_FORCEINLINE EquivIntVector operator==(T rhs) {return *this == SimdVectorFilled<T, N>(rhs);}
	INTRA_FORCEINLINE EquivIntVector operator!=(T rhs) {return *this != SimdVectorFilled<T, N>(rhs);}

	INTRA_FORCEINLINE EquivIntVector operator&&(T s) noexcept {return (*this != T(0)) & (s? TInt(-1): TInt(0));}


	INTRA_FORCEINLINE SimdVector& operator+=(SimdVector rhs) noexcept {return *this = *this + rhs;}
	INTRA_FORCEINLINE SimdVector& operator-=(SimdVector rhs) noexcept {return *this = *this - rhs;}
	INTRA_FORCEINLINE SimdVector& operator*=(SimdVector rhs) noexcept {return *this = *this * rhs;}
	INTRA_FORCEINLINE SimdVector& operator/=(SimdVector rhs) {return *this = *this / rhs;}

	INTRA_FORCEINLINE SimdVector& operator+=(T rhs) noexcept {return *this = *this + rhs;}
	INTRA_FORCEINLINE SimdVector& operator-=(T rhs) noexcept {return *this = *this - rhs;}
	INTRA_FORCEINLINE SimdVector& operator*=(T rhs) noexcept {return *this = *this * rhs;}
	INTRA_FORCEINLINE SimdVector& operator/=(T rhs) {return *this = *this / rhs;}

	INTRA_FORCEINLINE SimdVector& operator|=(SimdVector rhs) noexcept requires CBasicIntegral<T> {return *this = *this | rhs;}
	INTRA_FORCEINLINE SimdVector& operator&=(SimdVector rhs) noexcept requires CBasicIntegral<T> {return *this = *this & rhs;}
	INTRA_FORCEINLINE SimdVector& operator^=(SimdVector rhs) noexcept requires CBasicIntegral<T> {return *this = *this ^ rhs;}
	INTRA_FORCEINLINE SimdVector& operator<<=(SimdVector rhs) noexcept requires CBasicIntegral<T> {return *this = *this << rhs;}
	INTRA_FORCEINLINE SimdVector& operator>>=(SimdVector rhs) noexcept requires CBasicIntegral<T> {return *this = *this >> rhs;}

	INTRA_FORCEINLINE SimdVector& operator|=(T rhs) noexcept requires CBasicIntegral<T> {return *this = *this | rhs;}
	INTRA_FORCEINLINE SimdVector& operator&=(T rhs) noexcept requires CBasicIntegral<T> {return *this = *this & rhs;}
	INTRA_FORCEINLINE SimdVector& operator^=(T rhs) noexcept requires CBasicIntegral<T> {return *this = *this ^ rhs;}
	INTRA_FORCEINLINE SimdVector& operator<<=(T rhs) noexcept requires CBasicIntegral<T> {return *this = *this << rhs;}
	INTRA_FORCEINLINE SimdVector& operator>>=(T rhs) noexcept requires CBasicIntegral<T> {return *this = *this >> rhs;}
};
#undef INTRAZ_D_SIMD_OPERATOR_SCALAR_IMPL

template<class T, size_t N> INTRA_FORCEINLINE SimdVector<T, N> operator+(T lhs, SimdVector<T, N> rhs) noexcept {return SimdVectorFilled<T, N>(lhs) + rhs;}
template<class T, size_t N> INTRA_FORCEINLINE SimdVector<T, N> operator-(T lhs, SimdVector<T, N> rhs) noexcept {return SimdVectorFilled<T, N>(lhs) - rhs;}
template<class T, size_t N> INTRA_FORCEINLINE SimdVector<T, N> operator*(T lhs, SimdVector<T, N> rhs) noexcept {return SimdVectorFilled<T, N>(lhs) * rhs;}
template<class T, size_t N> INTRA_FORCEINLINE SimdVector<T, N> operator/(T lhs, SimdVector<T, N> rhs) noexcept {return SimdVectorFilled<T, N>(lhs) / rhs;}
template<class T, size_t N> INTRA_FORCEINLINE SimdVector<T, N> operator&(T lhs, SimdVector<T, N> rhs) noexcept {return SimdVectorFilled<T, N>(lhs) & rhs;}
template<class T, size_t N> INTRA_FORCEINLINE SimdVector<T, N> operator|(T lhs, SimdVector<T, N> rhs) noexcept {return SimdVectorFilled<T, N>(lhs) | rhs;}
template<class T, size_t N> INTRA_FORCEINLINE SimdVector<T, N> operator^(T lhs, SimdVector<T, N> rhs) noexcept {return SimdVectorFilled<T, N>(lhs) ^ rhs;}
template<class T, size_t N> INTRA_FORCEINLINE SimdVector<T, N> operator>(T lhs, SimdVector<T, N> rhs) noexcept {return SimdVectorFilled<T, N>(lhs) > rhs;}
template<class T, size_t N> INTRA_FORCEINLINE SimdVector<T, N> operator<(T lhs, SimdVector<T, N> rhs) noexcept {return SimdVectorFilled<T, N>(lhs) < rhs;}
template<class T, size_t N> INTRA_FORCEINLINE SimdVector<T, N> operator>=(T lhs, SimdVector<T, N> rhs) noexcept {return SimdVectorFilled<T, N>(lhs) >= rhs;}
template<class T, size_t N> INTRA_FORCEINLINE SimdVector<T, N> operator<=(T lhs, SimdVector<T, N> rhs) noexcept {return SimdVectorFilled<T, N>(lhs) <= rhs;}
template<class T, size_t N> INTRA_FORCEINLINE SimdVector<T, N> operator==(T lhs, SimdVector<T, N> rhs) noexcept {return SimdVectorFilled<T, N>(lhs) == rhs;}
template<class T, size_t N> INTRA_FORCEINLINE SimdVector<T, N> operator!=(T lhs, SimdVector<T, N> rhs) noexcept {return SimdVectorFilled<T, N>(lhs) != rhs;}
template<class T, size_t N> INTRA_FORCEINLINE SimdVector<T, N> operator&&(T lhs, SimdVector<T, N> rhs) noexcept {return SimdVectorFilled<T, N>(lhs) && rhs;}

namespace z_D {
// Implement Min and Max functors for SIMD types
template<class T, size_t N>
INTRA_FORCEINLINE auto Min_(SimdVector<T, N> a, SimdVector<T, N> b)
{
	if constexpr(!Config::TargetHasSSE41 && CAnyOf<T, int8, uint16, int32, uint32>) {}
	else if constexpr(!Config::TargetHasAVX512F && CAnyOf<T, int64, uint64>) {}
	else if constexpr(!CSame<decltype(z_D::simd_min<T>(a, b)), TUndefined>)
		return SimdVector<T, N>(z_D::simd_min<T>(a, b));
	return SimdSelect(a, b, a > b);
}

template<class T, size_t N>
INTRA_FORCEINLINE auto Max_(SimdVector<T, N> a, SimdVector<T, N> b)
{
	if constexpr(!Config::TargetHasSSE41 && CAnyOf<T, int8, uint16, int32, uint32>) {}
	else if constexpr(!Config::TargetHasAVX512F && CAnyOf<T, int64, uint64>) {}
	else if constexpr(!CSame<decltype(z_D::simd_max<T>(a, b)), TUndefined>)
		return SimdVector<T, N>(z_D::simd_max<T>(a, b));
	return SimdSelect(a, b, a < b);
}
}

} INTRA_END
#endif
//// </SimdVector implementation for MSVC>

namespace Intra { INTRA_BEGIN
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
	(CBasicIntegral<T> || CBasicFloatingPoint<T>) &&
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
			else if constexpr(CBasicIntegral<T> && N >= 4)
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
			else if constexpr(CBasicIntegral<T>)
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
			else if constexpr(CBasicIntegral<T>)
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
	if constexpr(CBasicIntegral<T>) return a & b;
	else return SimdVector<T, N>(SimdVector<TToIntegral<T>, N>(a) & SimdVector<TToIntegral<T>, N>(b));
}

namespace z_D {
template<typename T, size_t N> requires CBasicSigned<T>
inline SimdVector<T, N> Abs_(SimdVector<T, N> x)
{
	return SimdVector<T, N>(SimdVector<TToIntegral<T>, N>(x) & MaxValueOf<TToIntegral<T>>);
}
template<typename T, size_t N> requires CBasicFloatingPoint<T>
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

template<typename T, size_t N> requires CBasicFloatingPoint<T>
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

template<typename T, size_t N> requires CBasicFloatingPoint<T>
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

template<typename T, size_t N> requires CBasicFloatingPoint<T>
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

} INTRA_END



// runtime instruction support detection
namespace z_D {
#if defined(__i386__) || defined(__amd64__)
#ifdef _MSC_VER
extern "C" void __cpuid(int[4], int);
extern "C" uint64 _xgetbv(unsigned index);
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

struct TCpuInfo
{
	union {
		int Cpuid[33][4]{}; // EAX, EBX, EDX, ECX
		struct
		{
			// Cpuid[0]
			int MaxCpuIDParam;
			char ManufacturerID[12];

			// Cpuid[1]

			// eax
			uint32_t SteppingID: 4;
			uint32_t Model: 4;
			uint32_t FamilyID: 4;
			uint32_t ProcessorType: 2;
			uint32_t: 2;
			uint32_t ExtendedModelID: 4;
			uint32_t ExtendedFamilyID: 8;
			uint32_t: 4;

			// ebx
			uint8_t BrandIndex;
			uint8_t CacheLineSizeDiv8; // multiply by 8 to get cache line size; CLFSH must be set
			uint8_t MaxAddressableIDsForLogicalProcessors; // HTT must be set
			uint8_t LocalApicID;

			// edx
			bool FPU: 1;
			bool VME: 1;
			bool DE: 1;
			bool PSE: 1;
			bool TSC: 1;
			bool MSR: 1;
			bool PAE: 1;
			bool MCE: 1;
			bool CX8: 1;
			bool APIC: 1;
			bool: 1;
			bool SEP: 1;
			bool MTRR: 1;
			bool PGE: 1;
			bool MCA: 1;
			bool CMOV: 1;
			bool PAT: 1;
			bool PSE36: 1;
			bool PSN: 1; // Processor Serial Number supported and enabled
			bool CLFSH: 1;
			bool NX: 1; // Itanium-only
			bool DS: 1;
			bool ACPI: 1;
			bool MMX: 1;
			bool FXSR: 1;
			bool SSE: 1;
			bool SSE2: 1;
			bool SS: 1;
			bool HTT: 1;
			bool TM: 1;
			bool IA64: 1; // IA64 processor emulating x86
			bool PBE: 1;

			//ecx
			bool SSE3: 1;
			bool PCLMULQDQ: 1;
			bool DTES64: 1;
			bool MONITOR: 1;
			bool DSCPL: 1;
			bool VMX: 1;
			bool SMX: 1;
			bool EST: 1;
			bool TM2: 1;
			bool SSSE3: 1;
			bool CNXT_ID: 1;
			bool SDBG: 1;
			bool FMA: 1;
			bool CX16: 1; // CMPXCHG16B
			bool XTPR: 1;
			bool PDCM: 1;
			bool: 1;
			bool PSID: 1;
			bool DCA: 1;
			bool SSE41: 1;
			bool SSE42: 1;
			bool X2APIC: 1;
			bool MOVBE: 1;
			bool POPCNT: 1;
			bool TSC_DEADLINE: 1;
			bool AES_NI: 1;
			bool XSAVE: 1;
			bool OSXSAVE: 1;
			bool AVX: 1; // requires OSXSAVE and OS support, must be checked together
			bool F16C: 1;
			bool RDRAND: 1;
			bool Hypervisor: 1;

			// Cpuid[2]
			int CacheAndTLBCaps[4]; // a list of descriptors indicating cache and TLB capabilities

			// Cpuid[3]
			int ProcessorSerialNumber[4]; // not implemented in modern processors

			// Cpuid[4]
			int IntelTopology[4]; // Intel-only

			int: 32;
			int: 32;
			int: 32;
			int: 32;

			// Cpuid[6]

			// eax
			bool DigitalThermalSensor: 1;
			bool IntelTurboBoost: 1;
			bool AlwaysRunningAPICTimer: 1;
			bool PowerLimitNotification: 1;
			bool ExtendedClockModulationDuty: 1;
			bool PackageThermalManagement: 1;
			bool HardwareControlledPerformanceStates: 1;
			bool HWPNotification: 1;
			bool HWPActivityWindow: 1;
			bool HWPEnergyPerformancePreferenceControl: 1;
			bool HWPPackageLevelControl: 1;
			bool: 1;
			bool HardwareDutyCycling: 1;
			bool IntelTurboBoostMaxTechnology3: 1;
			bool InterruptsUponChangesToHWPCaps: 1;
			bool HWPPECIOverrideSupported: 1;
			bool FlexibleHWP: 1;
			bool FastAccessModeMSRSupported: 1;
			bool HardwareFeedback: 1;
			bool IgnoredIdleHWPRequest: 1;
			bool: 1;
			bool HwpCtlSupported: 1;
			bool IntelThreadDirector: 1;
			bool ThermInterrupSupported: 1;
			bool: 8;

			// ebx
			uint32_t NumInterruptThresholdsInDigitalThermalSensor: 4;
			uint32_t: 28;

			// edx
			bool PerformanceCapabilityReportingSupported: 1;
			bool EfficiencyCapabilityReportingSupported: 1;
			bool: 6;
			uint8_t HardwareFeedbackInterfaceStructSize: 4; //(in units of 4 Kbytes) minus 1
			uint8_t: 4;
			uint16_t ThisLogicalProcessorRowIndex;

			// ecx
			bool EffectiveFrequencyInterfaceSupported: 1;
			bool ACNT2: 1;
			bool: 1;
			bool PerformanceEnergyBias: 1;
			bool: 4;
			uint8_t NumSupportedIntelThreadDirectors;
			uint16_t: 16;

			// Cpuid[7]
			int MaxCpuID7Param;

			// ebx
			bool FSGSBASE: 1;
			bool TscAdjust: 1;
			bool SoftwareGuardExtensions: 1;
			bool BMI1: 1;
			bool HLE: 1; // only on Intel CPUs, need to check manufacturer
			bool AVX2: 1;
			bool FdpExceptionOnly: 1;
			bool SupervisorModeExecutionPrevention: 1;
			bool BMI2: 1;
			bool ERMS: 1;
			bool INVPCID: 1;
			bool RTM: 1; // only on Intel CPUs, need to check manufacturer
			bool RdtmOrPqm: 1;
			bool X87FpuCSAndDSDeprecated: 1;
			bool MemoryProtectionExtensions: 1;
			bool RdtaOrPqe: 1;
			bool AVX512F: 1;
			bool AVX512DQ: 1;
			bool RDSEED: 1;
			bool ADX: 1;
			bool SupervisorModeAccessPrevention: 1;
			bool AVX512IFMA: 1;
			bool PCOMMIT: 1; // deprecated
			bool CLFLUSHOPT: 1;
			bool CLWB: 1; // Cache Line Writeback instruction
			bool IntelProcessorTrace: 1;
			bool AVX512PF: 1;
			bool AVX512ER: 1;
			bool AVX512CD: 1;
			bool SHA: 1;

			int: 32;

			bool PREFETCHWT1: 1;
			bool: 7;
			bool: 8;
			bool: 8;
			bool: 8;

		#if 0
			static bool LAHF(void) { return CPU_Rep.f_81_ECX_[0]; }

			static bool LZCNT(void) { return CPU_Rep.f_81_ECX_[5]; }

			static bool SSE4a(void) { return CPU_Rep.isAMD_ && CPU_Rep.f_81_ECX_[6]; }
			static bool XOP(void) { return CPU_Rep.isAMD_ && CPU_Rep.f_81_ECX_[11]; }
			static bool TBM(void) { return CPU_Rep.isAMD_ && CPU_Rep.f_81_ECX_[21]; }

			static bool SYSCALL(void) { return CPU_Rep.isIntel_ && CPU_Rep.f_81_EDX_[11]; }
			static bool MMXEXT(void) { return CPU_Rep.isAMD_ && CPU_Rep.f_81_EDX_[22]; }
			static bool RDTSCP(void) { return CPU_Rep.isIntel_ && CPU_Rep.f_81_EDX_[27]; }
			static bool _3DNOWEXT(void) { return CPU_Rep.isAMD_ && CPU_Rep.f_81_EDX_[30]; }
			static bool _3DNOW(void) { return CPU_Rep.isAMD_ && CPU_Rep.f_81_EDX_[31]; }
		#endif
		};
	};
	INTRA_NOINLINE void Init()
	{
		for(int i = 0; i <= 32 && i <= MaxCpuIDParam; i++)
		{
			z_D::__cpuid(Cpuid[i], i);
			auto tmp = Cpuid[i][2];
			Cpuid[i][2] = Cpuid[i][3];
			Cpuid[i][3] = tmp;
		}
	}
} CpuCaps;

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

template<typename T> using TIdentity = T;

#ifdef __GNUC__
#define INTRA_DISPATCHED(funcName, ...) TIdentity<__VA_ARGS__> funcName __attribute__((ifunc(#funcName "_dispatch"))); extern "C" TIdentity<__VA_ARGS__>* funcName ## _dispatch()
#else
namespace z_D {
template<auto** FuncPtr> struct GenTrampoline;
template<typename R, typename... Args, R(**FuncPtr)(Args...)> struct GenTrampoline<FuncPtr>
{
	template<class F> static R Trampoline(Args... args) {return (*FuncPtr = F()())(args...);}
	template<typename F> auto operator*(F dispatcher) const {return &Trampoline<F>;}
};
}
#define INTRA_DISPATCHED(funcName, ...) TIdentity<__VA_ARGS__>* funcName = z_D::GenTrampoline<&funcName>() * []()
#endif

INTRA_DISPATCHED(myfunc, int(int))
{
	int instrset = SupportedInstructionSets() & 0xFF;
	printf("instrset = %i\n", instrset);
	if(instrset >= 10) return myfuncAVX512;
	if(instrset >= 8) return myfuncAVX2;
	return myfuncGeneric;
};

} INTRA_END
