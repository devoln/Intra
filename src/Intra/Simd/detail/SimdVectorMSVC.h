#pragma once

#include "Intra/Type.h"

INTRA_BEGIN

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
	static_assert(CIntegral<T> || CFloatingPoint<T>);
	static_assert(!CSame<T, long double>, "Use double instead of long double.");
	static_assert(!CChar<T>, "Use integer types instead of char/char8_t/char16_t/char32_t.");
	
private:
	using V = TPackAt<sizeof(T)*N / 16 - 1,
		TPackAt<CIntegral<T>? 0: CSame<float, T>? 1: 2, z_D::__m128i, z_D::__m128, z_D::__m128d>,
		TPackAt<CIntegral<T>? 0: CSame<float, T>? 1: 2, z_D::__m256i, z_D::__m256, z_D::__m256d>,
		void,
		TPackAt<CIntegral<T>? 0: CSame<float, T>? 1: 2, z_D::__m512i, z_D::__m512, z_D::__m512d>
	>;

	static constexpr bool SupportedByTarget =
		(sizeof(V) == 16 ||
			sizeof(V) == 32 && (CIntegral<T>? Config::TargetHasAVX2: Config::TargetHasAVX) ||
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
			if constexpr(!CSigned<T>) return SimdVector<TToSigned<T>, N>(*this) * SimdVector<TToSigned<T>, N>(rhs);
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

	INTRA_FORCEINLINE SimdVector operator%(SimdVector rhs) const requires CIntegral<T>
	{
		INTRAZ_D_SIMD_OPERATOR_SCALAR_IMPL(%);
	}

	INTRA_FORCEINLINE SimdVector operator&(SimdVector rhs) const noexcept requires CIntegral<T>
	{
		if constexpr(Config::TargetMaxSimdLength<T> >= N &&
			!CSame<decltype(z_D::simd_and<void>(mV, rhs.mV)), TUndefined>)
			return SimdVector(z_D::simd_and<void>(mV, rhs.mV));
		INTRAZ_D_SIMD_OPERATOR_SCALAR_IMPL(&);
	}
	INTRA_FORCEINLINE SimdVector operator|(SimdVector rhs) const noexcept requires CIntegral<T>
	{
		if constexpr(Config::TargetMaxSimdLength<T> >= N &&
			!CSame<decltype(z_D::simd_or<void>(mV, rhs.mV)), TUndefined>)
			return SimdVector(z_D::simd_or<void>(mV, rhs.mV));
		INTRAZ_D_SIMD_OPERATOR_SCALAR_IMPL(|);
	}
	INTRA_FORCEINLINE SimdVector operator^(SimdVector rhs) const noexcept requires CIntegral<T>
	{
		if constexpr(Config::TargetMaxSimdLength<T> >= N &&
			!CSame<decltype(z_D::simd_xor<void>(mV, rhs.mV)), TUndefined>)
			return SimdVector(z_D::simd_xor<void>(mV, rhs.mV));
		INTRAZ_D_SIMD_OPERATOR_SCALAR_IMPL(^);
	}
	INTRA_FORCEINLINE SimdVector operator~() const noexcept requires CIntegral<T>
	{
		return *this ^ T(-1);
	}


	INTRA_FORCEINLINE SimdVector operator<<(int bits) const noexcept requires CIntegral<T>
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

	INTRA_FORCEINLINE SimdVector operator>>(int bits) const noexcept requires CIntegral<T>
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

	INTRA_FORCEINLINE SimdVector operator<<(SimdVector rhs) const noexcept requires CIntegral<T>
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

	INTRA_FORCEINLINE SimdVector operator>>(SimdVector rhs) const noexcept requires CIntegral<T>
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
			if constexpr(!CSigned<T>)
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
			else if constexpr(sizeof(SimdVector) == 32 && CFloatingPoint<T>)
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
		if constexpr(CIntegral<T>) return rhs > *this;
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
		if constexpr(CIntegral<T>) return ~(*this > rhs);
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
		if constexpr(CIntegral<T>) return ~(rhs > *this);
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
			if constexpr(!CSigned<T>) return SimdVector<TToSigned<T>, N>(*this) == SimdVector<TToSigned<T>, N>(rhs);
			else if constexpr(!Config::TargetHasSSE42 && CSame<T, int64>)
			{
				const auto dwordsEqual  = SimdVector<int32, 4>(*this) == SimdVector<int32, 4>(rhs);
				const auto swappedLowHighDwords = SimdShuffle<1, 0, 3, 2>(dwordsEqual); // swap low and high dwords
				return EquivIntVector(SimdShuffle<1, 1, 3, 3>((dwordsEqual & swappedLowHighDwords) >> 31));
			}
			else if constexpr(sizeof(SimdVector) == 32 && CFloatingPoint<T>)
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
		if constexpr(CIntegral<T>) return ~(*this == rhs);
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

	INTRA_FORCEINLINE SimdVector& operator|=(SimdVector rhs) noexcept requires CIntegral<T> {return *this = *this | rhs;}
	INTRA_FORCEINLINE SimdVector& operator&=(SimdVector rhs) noexcept requires CIntegral<T> {return *this = *this & rhs;}
	INTRA_FORCEINLINE SimdVector& operator^=(SimdVector rhs) noexcept requires CIntegral<T> {return *this = *this ^ rhs;}
	INTRA_FORCEINLINE SimdVector& operator<<=(SimdVector rhs) noexcept requires CIntegral<T> {return *this = *this << rhs;}
	INTRA_FORCEINLINE SimdVector& operator>>=(SimdVector rhs) noexcept requires CIntegral<T> {return *this = *this >> rhs;}

	INTRA_FORCEINLINE SimdVector& operator|=(T rhs) noexcept requires CIntegral<T> {return *this = *this | rhs;}
	INTRA_FORCEINLINE SimdVector& operator&=(T rhs) noexcept requires CIntegral<T> {return *this = *this & rhs;}
	INTRA_FORCEINLINE SimdVector& operator^=(T rhs) noexcept requires CIntegral<T> {return *this = *this ^ rhs;}
	INTRA_FORCEINLINE SimdVector& operator<<=(T rhs) noexcept requires CIntegral<T> {return *this = *this << rhs;}
	INTRA_FORCEINLINE SimdVector& operator>>=(T rhs) noexcept requires CIntegral<T> {return *this = *this >> rhs;}
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

INTRA_END
