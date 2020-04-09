#pragma once

#include "Intra/Core.h"

#if defined(__i386__) || defined(__amd64__)
#if INTRA_SIMD_LEVEL >= INTRA_SIMD_LEVEL_SSE
#include <xmmintrin.h>
#endif
#if INTRA_SIMD_LEVEL >= INTRA_SIMD_LEVEL_SSE2
#include <emmintrin.h>
#endif
#if INTRA_SIMD_LEVEL >= INTRA_SIMD_LEVEL_SSE3
#include <pmmintrin.h>
#endif
#if INTRA_SIMD_LEVEL >= INTRA_SIMD_LEVEL_SSSE3
#include <tmmintrin.h>
#endif
#if INTRA_SIMD_LEVEL >= INTRA_SIMD_LEVEL_SSE4_1
#include <smmintrin.h>
#endif
#if INTRA_SIMD_LEVEL >= INTRA_SIMD_LEVEL_SSE4_2
#include <nmmintrin.h>
#endif
#if INTRA_SIMD_LEVEL >= INTRA_SIMD_LEVEL_AVX
#include <immintrin.h>
#endif
#endif

#if(!defined(__FMA__) && !defined(__GNUC__) && !defined(__clang__) && defined(INTRA_SIMD_LEVEL) && INTRA_SIMD_LEVEL >= INTRA_SIMD_LEVEL_AVX2)
//#define __FMA__ 1
#endif

namespace Simd {

INTRA_DEFINE_CONCEPT_REQUIRES(CSimdWrapper, T::VectorSize);


#ifdef INTRA_SIMD_LEVEL

template<typename T> INTRA_FORCEINLINE T INTRA_VECTORCALL Set(TScalarOf<T> v) noexcept { return T::Set(v); }

#if(INTRA_SIMD_LEVEL >= INTRA_SIMD_LEVEL_SSE2)

#define INTRA_SIMD_INT4_SUPPORT

struct float4;

struct int4
{
	__m128i Vec;

	enum {VectorSize = 4};

	INTRA_FORCEINLINE int4() = default;
	INTRA_FORCEINLINE int4(int x, int y, int z, int w) noexcept: Vec(_mm_set_epi32(w, z, y, x)) {}

	INTRA_FORCEINLINE int4 INTRA_VECTORCALL operator+(int4 rhs) const noexcept {return _mm_add_epi32(Vec, rhs.Vec);}
	INTRA_FORCEINLINE int4 INTRA_VECTORCALL operator-(int4 rhs) const noexcept {return _mm_sub_epi32(Vec, rhs.Vec);}
	INTRA_FORCEINLINE int4 INTRA_VECTORCALL operator*(int4 rhs) const noexcept
	{
#if(INTRA_SIMD_LEVEL >= INTRA_SIMD_LEVEL_SSE4_1)
		return _mm_mullo_epi32(Vec, rhs.Vec);
#else
		__m128i tmp1 = _mm_mul_epu32(Vec, rhs.Vec);
		__m128i tmp2 = _mm_mul_epu32(_mm_srli_si128(Vec, 4), _mm_srli_si128(rhs.Vec, 4));
		return _mm_unpacklo_epi32(_mm_shuffle_epi32(tmp1, _MM_SHUFFLE(0, 0, 2, 0)), _mm_shuffle_epi32(tmp2, _MM_SHUFFLE(0, 0, 2, 0)));
#endif
	}

	INTRA_FORCEINLINE int4 INTRA_VECTORCALL operator/(int4 rhs) const
	{
		alignas(16) int a[4], b[4];
		_mm_store_si128(reinterpret_cast<__m128i*>(a), Vec);
		_mm_store_si128(reinterpret_cast<__m128i*>(b), rhs.Vec);
		return {a[0] / b[0], a[1] / b[1], a[2] / b[2], a[3] / b[3]};
	}

	INTRA_FORCEINLINE int4 INTRA_VECTORCALL operator&(int4 rhs) const noexcept {return _mm_and_si128(Vec, rhs.Vec);}
	INTRA_FORCEINLINE int4 INTRA_VECTORCALL operator|(int4 rhs) const noexcept {return _mm_or_si128(Vec, rhs.Vec);}
	INTRA_FORCEINLINE int4 INTRA_VECTORCALL operator^(int4 rhs) const noexcept {return _mm_xor_si128(Vec, rhs.Vec);}
	INTRA_FORCEINLINE int4 INTRA_VECTORCALL operator~() const noexcept {return _mm_xor_si128(Vec, _mm_set1_epi32(-1));}

	INTRA_FORCEINLINE int4 INTRA_VECTORCALL operator<<(int bits) const noexcept {return _mm_slli_epi32(Vec, bits);}
	INTRA_FORCEINLINE int4 INTRA_VECTORCALL operator>>(int bits) const noexcept {return _mm_srai_epi32(Vec, bits);}

	INTRA_FORCEINLINE int4 INTRA_VECTORCALL operator<<(int4 rhs) const noexcept
	{
#if(INTRA_SIMD_LEVEL >= INTRA_SIMD_LEVEL_AVX2)
		return _mm_srlv_epi32(Vec, rhs.Vec);
#else
		alignas(16) int a[4], b[4];
		_mm_store_si128(reinterpret_cast<__m128i*>(a), Vec);
		_mm_store_si128(reinterpret_cast<__m128i*>(b), rhs.Vec);
		return {a[0] << b[0], a[1] << b[1], a[2] << b[2], a[3] << b[3]};
#endif
	}

	INTRA_FORCEINLINE int4 INTRA_VECTORCALL operator>>(int4 rhs) const noexcept
	{
#if(INTRA_SIMD_LEVEL >= INTRA_SIMD_LEVEL_AVX2)
		return _mm_srav_epi32(Vec, rhs.Vec);
#else
		alignas(16) int a[4], b[4];
		_mm_store_si128(reinterpret_cast<__m128i*>(a), Vec);
		_mm_store_si128(reinterpret_cast<__m128i*>(b), rhs.Vec);
		return {a[0] >> b[0], a[1] >> b[1], a[2] >> b[2], a[3] >> b[3]};
#endif
	}


	INTRA_FORCEINLINE int4 INTRA_VECTORCALL operator>(int4 rhs) const noexcept {return _mm_cmpgt_epi32(Vec, rhs.Vec);}
	INTRA_FORCEINLINE int4 INTRA_VECTORCALL operator<(int4 rhs) const noexcept {return _mm_cmplt_epi32(Vec, rhs.Vec);}
	INTRA_FORCEINLINE int4 INTRA_VECTORCALL operator>=(int4 rhs) const noexcept {return ~operator<(rhs);}
	INTRA_FORCEINLINE int4 INTRA_VECTORCALL operator<=(int4 rhs) const noexcept {return ~operator>(rhs);}
	INTRA_FORCEINLINE int4 INTRA_VECTORCALL operator==(int4 rhs) const noexcept {return _mm_cmpeq_epi32(Vec, rhs.Vec);}
	INTRA_FORCEINLINE int4 INTRA_VECTORCALL operator!=(int4 rhs) const noexcept {return ~operator==(rhs);}

	INTRA_FORCEINLINE int operator[](int i) const
	{
		alignas(16) int arr[4];
		_mm_store_si128(reinterpret_cast<__m128i*>(arr), Vec);
		return arr[i];
	}

	INTRA_FORCEINLINE int4(__m128i vec) noexcept: Vec(vec) {}
	INTRA_FORCEINLINE operator __m128i() const {return Vec;}

	static INTRA_FORCEINLINE int4 Set(int v) noexcept {return _mm_set1_epi32(v);}
};

INTRA_FORCEINLINE int4 INTRA_VECTORCALL Load4(const int* src) {return _mm_loadu_si128(reinterpret_cast<const __m128i*>(src));}
INTRA_FORCEINLINE int4 INTRA_VECTORCALL Load4Aligned(const int* src) {return _mm_load_si128(reinterpret_cast<const __m128i*>(src));}
INTRA_FORCEINLINE void INTRA_VECTORCALL Store(int4 v, int* dst) {_mm_storeu_si128(reinterpret_cast<__m128i*>(dst), v.Vec);}
INTRA_FORCEINLINE void INTRA_VECTORCALL StoreAligned(int4 v, int* dst) {_mm_store_si128(reinterpret_cast<__m128i*>(dst), v.Vec);}

template<int i0, int i1, int i2, int i3> INTRA_FORCEINLINE int4 INTRA_VECTORCALL Shuffle(int4 v) noexcept
{
	static_assert(
		0 <= i0 && i0 <= 3 &&
		0 <= i1 && i1 <= 3 &&
		0 <= i2 && i2 <= 3 &&
		0 <= i3 && i3 <= 3,
		"Valid range of shuffle indices is [0; 3]");
	return _mm_shuffle_epi32(v.Vec, v.Vec, _MM_SHUFFLE(i3, i2, i1, i0));
}

INTRA_FORCEINLINE int4 INTRA_VECTORCALL Shuffle(int4 v, int4 indices) noexcept
{
#if(INTRA_SIMD_LEVEL >= INTRA_SIMD_LEVEL_SSSE3)
	indices = indices << 2;
	indices.Vec = _mm_shuffle_epi8(indices.Vec, _mm_set_epi8(12, 12, 12, 12, 8, 8, 8, 8, 4, 4, 4, 4, 0, 0, 0, 0));
	indices.Vec = _mm_add_epi8(indices.Vec, _mm_set_epi8(3, 2, 1, 0, 3, 2, 1, 0, 3, 2, 1, 0, 3, 2, 1, 0));
	return _mm_shuffle_epi8(v.Vec, indices.Vec);
#else
	return {v[indices[0]], v[indices[1]], v[indices[2]], v[indices[3]]};
#endif
}

template<int n> INTRA_FORCEINLINE int4 INTRA_VECTORCALL RotateLeft(int4 v) noexcept
{
	return Shuffle<n & 3, (n + 1) & 3, (n + 2) & 3, (n + 3) & 3>(v);
}

template<int n> INTRA_FORCEINLINE int4 INTRA_VECTORCALL RotateRight(int4 v) noexcept {return RotateLeft<-n>(v);}

INTRA_FORCEINLINE int4 INTRA_VECTORCALL Min(int4 a, int4 b) noexcept
{
#if(INTRA_SIMD_LEVEL >= INTRA_SIMD_LEVEL_SSE4_1)
	return _mm_min_epi32(a.Vec, b.Vec);
#else
	__m128i mask = _mm_cmplt_epi32(a.Vec, b.Vec);
	return _mm_or_si128(_mm_and_si128(a.Vec, mask), _mm_andnot_si128(mask, b.Vec));
#endif
}

INTRA_FORCEINLINE int4 INTRA_VECTORCALL Max(int4 a, int4 b) noexcept
{
#if(INTRA_SIMD_LEVEL >= INTRA_SIMD_LEVEL_SSE4_1)
	return _mm_max_epi32(a.Vec, b.Vec);
#else
	__m128i mask = _mm_cmpgt_epi32(a.Vec, b.Vec);
	return _mm_or_si128(_mm_and_si128(a.Vec, mask), _mm_andnot_si128(mask, b.Vec));
#endif
}

INTRA_FORCEINLINE int INTRA_VECTORCALL HorSum(int4 v) noexcept {return v[0] + v[1] + v[2] + v[3];}

INTRA_FORCEINLINE int4 INTRA_VECTORCALL UnsignedRightBitShift(int4 x, int bits) noexcept {return _mm_srli_epi32(x.Vec, bits);}

#define INTRA_SIMD_FLOAT4_SUPPORT

struct float4
{
	__m128 Vec;

	enum { VectorSize = 4 };

	INTRA_FORCEINLINE float4() = default;
	INTRA_FORCEINLINE float4(float x, float y, float z, float w) noexcept: Vec(_mm_set_ps(w, z, y, x)) {}

	INTRA_FORCEINLINE float4 INTRA_VECTORCALL operator+(float4 rhs) const noexcept {return _mm_add_ps(Vec, rhs.Vec);}
	INTRA_FORCEINLINE float4 INTRA_VECTORCALL operator-(float4 rhs) const noexcept {return _mm_sub_ps(Vec, rhs.Vec);}
	INTRA_FORCEINLINE float4 INTRA_VECTORCALL operator*(float4 rhs) const noexcept {return _mm_mul_ps(Vec, rhs.Vec);}
	INTRA_FORCEINLINE float4 INTRA_VECTORCALL operator/(float4 rhs) const {return _mm_div_ps(Vec, rhs.Vec);}

	INTRA_FORCEINLINE int4 INTRA_VECTORCALL operator>(float4 rhs) const {return int4(float4(_mm_cmpgt_ps(Vec, rhs.Vec)));}
	INTRA_FORCEINLINE int4 INTRA_VECTORCALL operator<(float4 rhs) const {return int4(float4(_mm_cmplt_ps(Vec, rhs.Vec)));}
	INTRA_FORCEINLINE int4 INTRA_VECTORCALL operator>=(float4 rhs) const {return int4(float4(_mm_cmpge_ps(Vec, rhs.Vec)));}
	INTRA_FORCEINLINE int4 INTRA_VECTORCALL operator<=(float4 rhs) const {return int4(float4(_mm_cmple_ps(Vec, rhs.Vec)));}
	INTRA_FORCEINLINE int4 INTRA_VECTORCALL operator==(float4 rhs) const noexcept {return int4(float4(_mm_cmpeq_ps(Vec, rhs.Vec)));}
	INTRA_FORCEINLINE int4 INTRA_VECTORCALL operator!=(float4 rhs) const noexcept {return int4(float4(_mm_cmpneq_ps(Vec, rhs.Vec)));}

	INTRA_FORCEINLINE explicit float4(const int4& v) noexcept: Vec(_mm_castsi128_ps(v.Vec)) {}
	INTRA_FORCEINLINE explicit operator int4() const noexcept {return _mm_castps_si128(Vec);}

	INTRA_FORCEINLINE float operator[](int i) const
	{
		alignas(16) float arr[4];
		_mm_store_ps(arr, Vec);
		return arr[i];
	}

	INTRA_FORCEINLINE operator __m128() const noexcept {return Vec;}
	INTRA_FORCEINLINE float4(__m128 vec) noexcept: Vec(vec) {}

	static INTRA_FORCEINLINE float4 Set(float v) noexcept {return _mm_set1_ps(v);}
};

INTRA_FORCEINLINE float4 INTRA_VECTORCALL Load4(const float* src) noexcept {return _mm_loadu_ps(src);}
INTRA_FORCEINLINE float4 INTRA_VECTORCALL Load4Aligned(const float* src) noexcept {return _mm_load_ps(src);}
INTRA_FORCEINLINE void INTRA_VECTORCALL Store(float4 v, float* dst) noexcept {return _mm_storeu_ps(dst, v.Vec);}
INTRA_FORCEINLINE void INTRA_VECTORCALL StoreAligned(float4 v, float* dst) noexcept {return _mm_store_ps(dst, v.Vec);}

INTRA_FORCEINLINE int4 INTRA_VECTORCALL TruncateToInt(float4 v) noexcept
{
	return _mm_cvttps_epi32(v.Vec);
}

INTRA_FORCEINLINE float4 INTRA_VECTORCALL CastToFloat(int4 i4) noexcept {return _mm_cvtepi32_ps(i4.Vec);}

template<int i0a, int i1a, int i2b, int i3b> INTRA_FORCEINLINE int4 INTRA_VECTORCALL Shuffle22(int4 a, int4 b) noexcept
{
	static_assert(
		0 <= i0a && i0a <= 3 &&
		0 <= i1a && i1a <= 3 &&
		0 <= i2b && i2b <= 3 &&
		0 <= i3b && i3b <= 3,
		"Valid range of shuffle indices is [0; 3]");
	return _mm_shuffle_epi32(a.Vec, b.Vec, _MM_SHUFFLE(i3b, i2b, i1a, i0a));
}

template<int i0a, int i1a, int i2b, int i3b> INTRA_FORCEINLINE float4 INTRA_VECTORCALL Shuffle22(float4 a, float4 b) noexcept
{
	static_assert(
		0 <= i0a && i0a <= 3 &&
		0 <= i1a && i1a <= 3 &&
		0 <= i2b && i2b <= 3 &&
		0 <= i3b && i3b <= 3,
		"Valid range of shuffle indices is [0; 3]");
	return _mm_shuffle_ps(a.Vec, b.Vec, _MM_SHUFFLE(i3b, i2b, i1a, i0a));
}

template<int i0, int i1, int i2, int i3> INTRA_FORCEINLINE float4 INTRA_VECTORCALL Shuffle(float4 v) noexcept
{
	static_assert(
		0 <= i0 && i0 <= 3 &&
		0 <= i1 && i1 <= 3 &&
		0 <= i2 && i2 <= 3 &&
		0 <= i3 && i3 <= 3,
		"Valid range of shuffle indices is [0; 3]");
	return _mm_shuffle_ps(v.Vec, v.Vec, _MM_SHUFFLE(i3, i2, i1, i0));
}

INTRA_FORCEINLINE float4 INTRA_VECTORCALL Shuffle(float4 v, int4 indices) noexcept
{
#if(INTRA_SIMD_LEVEL >= INTRA_SIMD_LEVEL_SSSE3)
	indices = indices << 2;
	indices.Vec = _mm_shuffle_epi8(indices.Vec, _mm_set_epi8(12, 12, 12, 12, 8, 8, 8, 8, 4, 4, 4, 4, 0, 0, 0, 0));
	indices.Vec = _mm_add_epi8(indices.Vec, _mm_set_epi8(3, 2, 1, 0, 3, 2, 1, 0, 3, 2, 1, 0, 3, 2, 1, 0));
	return float4(int4(_mm_shuffle_epi8(int4(v).Vec, indices.Vec)));
#else
	return {v[indices[0]], v[indices[1]], v[indices[2]], v[indices[3]]};
#endif
}

template<int n> INTRA_FORCEINLINE float4 INTRA_VECTORCALL RotateLeft(float4 v) noexcept
{
	return Shuffle<n & 3, (n + 1) & 3, (n + 2) & 3, (n + 3) & 3>(v);
}

template<int n> INTRA_FORCEINLINE float4 INTRA_VECTORCALL RotateRight(float4 v) noexcept {return RotateLeft<-n>(v);}


INTRA_FORCEINLINE float4 INTRA_VECTORCALL InterleaveLow(float4 a, float4 b) noexcept {return _mm_unpacklo_ps(a.Vec, b.Vec);}
INTRA_FORCEINLINE int4 INTRA_VECTORCALL InterleaveLow(int4 a, int4 b) noexcept {return _mm_unpacklo_epi32(a.Vec, b.Vec);}
INTRA_FORCEINLINE float4 INTRA_VECTORCALL InterleaveHigh(float4 a, float4 b) noexcept {return _mm_unpackhi_ps(a.Vec, b.Vec);}
INTRA_FORCEINLINE int4 INTRA_VECTORCALL InterleaveHigh(int4 a, int4 b) noexcept {return _mm_unpackhi_epi32(a.Vec, b.Vec);}

INTRA_FORCEINLINE float INTRA_VECTORCALL HorSum(float4 v) noexcept
{
#if INTRA_SIMD_LEVEL >= INTRA_SIMD_LEVEL_SSE3
	__m128 tmp0 = _mm_hadd_ps(v.Vec, v.Vec);
	__m128 tmp1 = _mm_hadd_ps(tmp0, tmp0);
#else
	__m128 tmp0 = _mm_add_ps(v.Vec, _mm_movehl_ps(v.Vec, v.Vec));
	__m128 tmp1 = _mm_add_ss(tmp0, _mm_shuffle_ps(tmp0, tmp0, 1));
#endif
	return _mm_cvtss_f32(tmp1);
}

INTRA_FORCEINLINE float4 INTRA_VECTORCALL Max(float4 a, float4 b) noexcept {return _mm_max_ps(a.Vec, b.Vec);}
INTRA_FORCEINLINE float4 INTRA_VECTORCALL Min(float4 a, float4 b) noexcept {return _mm_min_ps(a.Vec, b.Vec);}

#endif

#if(INTRA_SIMD_LEVEL >= INTRA_SIMD_LEVEL_AVX2)

#define INTRA_SIMD_INT8_SUPPORT

struct float8;

struct int8
{
	__m256i Vec;

	enum { VectorSize = 8 };

	INTRA_FORCEINLINE int8() = default;
	INTRA_FORCEINLINE int8(int x1, int x2, int x3, int x4, int x5, int x6, int x7, int x8) noexcept:
		Vec(_mm256_set_epi32(x8, x7, x6, x5, x4, x3, x2, x1))
	{}

	INTRA_FORCEINLINE int8 INTRA_VECTORCALL operator+(int8 rhs) const noexcept {return _mm256_add_epi32(Vec, rhs.Vec);}
	INTRA_FORCEINLINE int8 INTRA_VECTORCALL operator-(int8 rhs) const noexcept {return _mm256_sub_epi32(Vec, rhs.Vec);}
	INTRA_FORCEINLINE int8 INTRA_VECTORCALL operator*(int8 rhs) const noexcept {return _mm256_mul_epi32(Vec, rhs.Vec);}
	INTRA_FORCEINLINE int8 INTRA_VECTORCALL operator/(int8 rhs) const
	{
		alignas(32) int a[8], b[8];
		_mm256_storeu_si256(reinterpret_cast<__m256i*>(a), Vec);
		_mm256_storeu_si256(reinterpret_cast<__m256i*>(b), rhs.Vec);
		return {a[0] / b[0], a[1] / b[1], a[2] / b[2], a[3] / b[3], a[4] / b[4], a[5] / b[5], a[6] / b[6], a[7] / b[7]};
	}

	INTRA_FORCEINLINE int8 INTRA_VECTORCALL operator&(int8 rhs) const noexcept {return _mm256_and_si256(Vec, rhs.Vec);}
	INTRA_FORCEINLINE int8 INTRA_VECTORCALL operator|(int8 rhs) const noexcept {return _mm256_or_si256(Vec, rhs.Vec);}
	INTRA_FORCEINLINE int8 INTRA_VECTORCALL operator^(int8 rhs) const noexcept {return _mm256_xor_si256(Vec, rhs.Vec);}
	INTRA_FORCEINLINE int8 INTRA_VECTORCALL operator~() const noexcept {return _mm256_xor_si256(Vec, _mm256_set1_epi32(-1));}

	INTRA_FORCEINLINE int8 INTRA_VECTORCALL operator<<(int bits) const noexcept {return _mm256_slli_epi32(Vec, bits);}
	INTRA_FORCEINLINE int8 INTRA_VECTORCALL operator>>(int bits) const noexcept {return _mm256_srai_epi32(Vec, bits);}

	INTRA_FORCEINLINE int8 INTRA_VECTORCALL operator<<(int8 rhs) const noexcept {return _mm256_srlv_epi32(Vec, rhs.Vec);}
	INTRA_FORCEINLINE int8 INTRA_VECTORCALL operator>>(int8 rhs) const noexcept {return _mm256_srav_epi32(Vec, rhs.Vec);}


	INTRA_FORCEINLINE int8 INTRA_VECTORCALL operator>(int8 rhs) const noexcept {return _mm256_cmpgt_epi32(Vec, rhs.Vec);}
	INTRA_FORCEINLINE int8 INTRA_VECTORCALL operator<(int8 rhs) const noexcept {return rhs < *this;}
	INTRA_FORCEINLINE int8 INTRA_VECTORCALL operator>=(int8 rhs) const noexcept {return ~(rhs > *this);}
	INTRA_FORCEINLINE int8 INTRA_VECTORCALL operator<=(int8 rhs) const noexcept {return ~operator>(rhs);}
	INTRA_FORCEINLINE int8 INTRA_VECTORCALL operator==(int8 rhs) const noexcept {return _mm256_cmpeq_epi32(Vec, rhs.Vec);}
	INTRA_FORCEINLINE int8 INTRA_VECTORCALL operator!=(int8 rhs) const noexcept {return ~operator==(rhs);}

	INTRA_FORCEINLINE int operator[](int i) const
	{
		alignas(32) int arr[8];
		_mm256_store_si256(reinterpret_cast<__m256i*>(arr), Vec);
		return arr[i];
	}

	INTRA_FORCEINLINE int8(__m256i vec) noexcept: Vec(vec) {}
	INTRA_FORCEINLINE operator __m256i() const { return Vec; }

	static INTRA_FORCEINLINE int8 INTRA_VECTORCALL Set(int v) noexcept {return _mm256_set1_epi32(v);}
};

INTRA_FORCEINLINE int8 INTRA_VECTORCALL Load8(const int* src) noexcept {return _mm256_loadu_si256(reinterpret_cast<const __m256i*>(src));}
INTRA_FORCEINLINE int8 INTRA_VECTORCALL Load8Aligned(const int* src) noexcept {return _mm256_load_si256(reinterpret_cast<const __m256i*>(src));}

INTRA_FORCEINLINE void INTRA_VECTORCALL Store(int8 v, int* dst) {_mm256_storeu_si256(reinterpret_cast<__m256i*>(dst), v.Vec);}
INTRA_FORCEINLINE void INTRA_VECTORCALL StoreAligned(int8 v, int* dst) {_mm256_store_si256(reinterpret_cast<__m256i*>(dst), v.Vec);}

template<int i0, int i1, int i2, int i3> INTRA_FORCEINLINE int8 INTRA_VECTORCALL Shuffle(int8 v) noexcept
{
	static_assert(
		0 <= i0 && i0 <= 7 &&
		0 <= i1 && i1 <= 7 &&
		0 <= i2 && i2 <= 7 &&
		0 <= i3 && i3 <= 7 &&
		0 <= i4 && i4 <= 7 &&
		0 <= i5 && i5 <= 7 &&
		0 <= i6 && i6 <= 7 &&
		0 <= i7 && i7 <= 7,
		"Valid range of shuffle indices is [0; 7]");
	__m128i h = _mm256_extractf128_si256(v.Vec, 0);
	__m128i l = _mm256_extractf128_si256(v.Vec, 1);
	//TODO: проверить, что порядок правильный
	l = _mm_shuffle_ps(l, l, _MM_SHUFFLE(i3, i2, i1, i0));
	h = _mm_shuffle_ps(h, h, _MM_SHUFFLE(i7, i6, i5, i4));
	return _mm256_set_m128i(h, l);
}

template<int n> INTRA_FORCEINLINE int8 INTRA_VECTORCALL RotateLeft(int8 v) noexcept
{
	return Shuffle<n & 7, (n + 1) & 7, (n + 2) & 7, (n + 3) & 7, (n + 4) & 7, (n + 5) & 7, (n + 6) & 7, (n + 7) & 7>();
}

template<int n> INTRA_FORCEINLINE int8 INTRA_VECTORCALL RotateRight(int8 v) noexcept { return RotateLeft<-n>(v); }

INTRA_FORCEINLINE int8 INTRA_VECTORCALL InterleaveLow(int8 a, int8 b) noexcept
{
	return _mm256_permute2f128_si256(_mm256_unpacklo_epi32(a.Vec, b.Vec), _mm256_unpackhi_epi32(a.Vec, b.Vec), 0x20);
}

INTRA_FORCEINLINE int8 INTRA_VECTORCALL InterleaveHigh(int8 a, int8 b) noexcept
{
	return _mm256_permute2f128_si256(_mm256_unpacklo_epi32(a.Vec, b.Vec), _mm256_unpackhi_epi32(a.Vec, b.Vec), 0x31);
}


INTRA_FORCEINLINE int8 INTRA_VECTORCALL Max(int8 a, int8 b) noexcept {return _mm256_max_epi32(a.Vec, b.Vec);}
INTRA_FORCEINLINE int8 INTRA_VECTORCALL Min(int8 a, int8 b) noexcept {return _mm256_min_epi32(a.Vec, b.Vec);}

INTRA_FORCEINLINE int8 INTRA_VECTORCALL UnsignedRightBitShift(int8 x, int bits) noexcept {return _mm256_srli_epi32(x.Vec, bits);}

#endif

#if(INTRA_SIMD_LEVEL >= INTRA_SIMD_LEVEL_AVX)

#define INTRA_SIMD_FLOAT8_SUPPORT

struct float8
{
	__m256 Vec;

	enum { VectorSize = 8 };

	INTRA_FORCEINLINE float8() = default;
	INTRA_FORCEINLINE float8(float x1, float x2, float x3, float x4, float x5, float x6, float x7, float x8) noexcept:
		Vec(_mm256_set_ps(x8, x7, x6, x5, x4, x3, x2, x1))
	{}

	INTRA_FORCEINLINE float8 INTRA_VECTORCALL operator+(float8 rhs) const noexcept {return _mm256_add_ps(Vec, rhs.Vec);}
	INTRA_FORCEINLINE float8 INTRA_VECTORCALL operator-(float8 rhs) const noexcept {return _mm256_sub_ps(Vec, rhs.Vec);}
	INTRA_FORCEINLINE float8 INTRA_VECTORCALL operator*(float8 rhs) const noexcept {return _mm256_mul_ps(Vec, rhs.Vec);}
	INTRA_FORCEINLINE float8 INTRA_VECTORCALL operator/(float8 rhs) const {return _mm256_div_ps(Vec, rhs.Vec);}

#if(INTRA_SIMD_LEVEL >= INTRA_SIMD_LEVEL_AVX2)
	INTRA_FORCEINLINE int8 INTRA_VECTORCALL operator>(float8 rhs) const {return int8(float8(_mm256_cmp_ps(Vec, rhs.Vec, _CMP_GT_OQ)));}
	INTRA_FORCEINLINE int8 INTRA_VECTORCALL operator<(float8 rhs) const {return int8(float8(_mm256_cmp_ps(Vec, rhs.Vec, _CMP_LT_OQ)));}
	INTRA_FORCEINLINE int8 INTRA_VECTORCALL operator>=(float8 rhs) const {return int8(float8(_mm256_cmp_ps(Vec, rhs.Vec, _CMP_GE_OQ)));}
	INTRA_FORCEINLINE int8 INTRA_VECTORCALL operator<=(float8 rhs) const {return int8(float8(_mm256_cmp_ps(Vec, rhs.Vec, _CMP_LE_OQ)));}
	INTRA_FORCEINLINE int8 INTRA_VECTORCALL operator==(float8 rhs) const noexcept {return int8(float8(_mm256_cmp_ps(Vec, rhs.Vec, _CMP_EQ_OQ)));}
	INTRA_FORCEINLINE int8 INTRA_VECTORCALL operator!=(float8 rhs) const noexcept {return int8(float8(_mm256_cmp_ps(Vec, rhs.Vec, _CMP_NEQ_OQ)));}

	INTRA_FORCEINLINE explicit float8(const int8& v): Vec(_mm256_castsi256_ps(v.Vec)) {}
	INTRA_FORCEINLINE explicit operator int8() const noexcept {return _mm256_castps_si256(Vec);}
#endif

	INTRA_FORCEINLINE float operator[](int i) const
	{
		alignas(32) float arr[8];
		_mm256_store_ps(arr, Vec);
		return arr[i];
	}

	INTRA_FORCEINLINE operator __m256() const noexcept {return Vec;}
	INTRA_FORCEINLINE float8(__m256 vec) noexcept: Vec(vec) {}

	static INTRA_FORCEINLINE float8 INTRA_VECTORCALL Set(float v) noexcept {return _mm256_set1_ps(v);}
};

INTRA_FORCEINLINE float8 INTRA_VECTORCALL Load8(const float* src) {return _mm256_loadu_ps(src);}
INTRA_FORCEINLINE float8 INTRA_VECTORCALL Load8Aligned(const float* src) {return _mm256_load_ps(src);}
INTRA_FORCEINLINE void INTRA_VECTORCALL Store(float8 v, float* dst) {_mm256_storeu_ps(dst, v.Vec);}
INTRA_FORCEINLINE void INTRA_VECTORCALL StoreAligned(float8 v, float* dst) {_mm256_store_ps(dst, v.Vec);}

INTRA_FORCEINLINE void End() noexcept { _mm256_zeroupper(); }

INTRA_FORCEINLINE float8 INTRA_VECTORCALL Max(float8 a, float8 b) noexcept {return _mm256_max_ps(a.Vec, b.Vec);}
INTRA_FORCEINLINE float8 INTRA_VECTORCALL Min(float8 a, float8 b) noexcept {return _mm256_min_ps(a.Vec, b.Vec);}

INTRA_FORCEINLINE float INTRA_VECTORCALL HorSum(float8 v)
{
	__m256 tmp0 = _mm256_hadd_ps(v.Vec, v.Vec);
	__m256 tmp1 = _mm256_hadd_ps(tmp0, tmp0);
	alignas(32) float arr[8];
	_mm256_store_ps(arr, tmp1);
	return arr[0] + arr[4];
}

template<int i0, int i1, int i2, int i3, int i4, int i5, int i6, int i7> INTRA_FORCEINLINE
float8 INTRA_VECTORCALL Shuffle(float8 v) noexcept
{
	static_assert(
		0 <= i0 && i0 <= 7 &&
		0 <= i1 && i1 <= 7 &&
		0 <= i2 && i2 <= 7 &&
		0 <= i3 && i3 <= 7 &&
		0 <= i4 && i4 <= 7 &&
		0 <= i5 && i5 <= 7 &&
		0 <= i6 && i6 <= 7 &&
		0 <= i7 && i7 <= 7,
		"Valid range of shuffle indices is [0; 7]");
	__m128 h = _mm256_extractf128_ps(v.Vec, 0);
	__m128 l = _mm256_extractf128_ps(v.Vec, 1);
	//TODO: проверить, что порядок правильный
	l = _mm_shuffle_ps(l, l, _MM_SHUFFLE(i3, i2, i1, i0));
	h = _mm_shuffle_ps(h, h, _MM_SHUFFLE(i7, i6, i5, i4));
	return _mm256_set_m128(h, l);
}

template<int n> INTRA_FORCEINLINE float8 INTRA_VECTORCALL RotateLeft(float8 v) noexcept
{
	return Shuffle<n & 7, (n + 1) & 7, (n + 2) & 7, (n + 3) & 7, (n + 4) & 7, (n + 5) & 7, (n + 6) & 7, (n + 7) & 7>(v);
}

template<int n> INTRA_FORCEINLINE float8 INTRA_VECTORCALL RotateRight(float8 v) noexcept {return RotateLeft<-n>(v);}

INTRA_FORCEINLINE float8 INTRA_VECTORCALL InterleaveLow(float8 a, float8 b) noexcept
{
	return _mm256_permute2f128_ps(_mm256_unpacklo_ps(a.Vec, b.Vec), _mm256_unpackhi_ps(a.Vec, b.Vec), 0x20);
}

INTRA_FORCEINLINE float8 INTRA_VECTORCALL InterleaveHigh(float8 a, float8 b) noexcept
{
	return _mm256_permute2f128_ps(_mm256_unpacklo_ps(a.Vec, b.Vec), _mm256_unpackhi_ps(a.Vec, b.Vec), 0x31);
}

#endif

#if(INTRA_SIMD_LEVEL >= INTRA_SIMD_LEVEL_AVX2)
INTRA_FORCEINLINE int8 INTRA_VECTORCALL TruncateToInt(float8 v) noexcept {return _mm256_cvttps_epi32(v.Vec);}
INTRA_FORCEINLINE float8 INTRA_VECTORCALL CastToFloat(int8 v) noexcept {return _mm256_cvtepi32_ps(v.Vec);}
#endif

#endif

#if(!defined(INTRA_SIMD_LEVEL) || INTRA_SIMD_LEVEL < INTRA_SIMD_LEVEL_AVX)
INTRA_FORCEINLINE void End() noexcept {}
#endif


template<class T> INTRA_FORCEINLINE Requires<CSimdWrapper<T>, T&> INTRA_VECTORCALL operator+=(T& lhs, T rhs) noexcept {return lhs = lhs + rhs;}
template<class T> INTRA_FORCEINLINE Requires<CSimdWrapper<T>, T&> INTRA_VECTORCALL operator-=(T& lhs, T rhs) noexcept {return lhs = lhs - rhs;}
template<class T> INTRA_FORCEINLINE Requires<CSimdWrapper<T>, T&> INTRA_VECTORCALL operator*=(T& lhs, T rhs) noexcept {return lhs = lhs * rhs;}
template<class T> INTRA_FORCEINLINE Requires<CSimdWrapper<T>, T&> INTRA_VECTORCALL operator/=(T& lhs, T rhs) {return lhs = lhs / rhs;}

template<class T> INTRA_FORCEINLINE Requires<CSimdWrapper<T>, T&> operator+=(T& lhs, TScalarOf<T> rhs) noexcept { return lhs = lhs + rhs; }
template<class T> INTRA_FORCEINLINE Requires<CSimdWrapper<T>, T&> operator-=(T& lhs, TScalarOf<T> rhs) noexcept { return lhs = lhs - rhs; }
template<class T> INTRA_FORCEINLINE Requires<CSimdWrapper<T>, T&> operator*=(T& lhs, TScalarOf<T> rhs) noexcept { return lhs = lhs * rhs; }
template<class T> INTRA_FORCEINLINE Requires<CSimdWrapper<T>, T&> operator/=(T& lhs, TScalarOf<T> rhs) { return lhs = lhs / rhs; }

template<class T> INTRA_FORCEINLINE Requires<CSimdWrapper<T>, T> INTRA_VECTORCALL operator+(T lhs, TScalarOf<T> rhs) noexcept { return lhs + Set<T>(rhs); }
template<class T> INTRA_FORCEINLINE Requires<CSimdWrapper<T>, T> INTRA_VECTORCALL operator-(T lhs, TScalarOf<T> rhs) noexcept { return lhs - Set<T>(rhs); }
template<class T> INTRA_FORCEINLINE Requires<CSimdWrapper<T>, T> INTRA_VECTORCALL operator*(T lhs, TScalarOf<T> rhs) noexcept { return lhs * Set<T>(rhs); }
template<class T> INTRA_FORCEINLINE Requires<CSimdWrapper<T>, T> INTRA_VECTORCALL operator/(T lhs, TScalarOf<T> rhs) { return lhs / Set<T>(rhs); }

template<class T> INTRA_FORCEINLINE Requires<CSimdWrapper<T>, T> INTRA_VECTORCALL operator+(TScalarOf<T> lhs, T rhs) noexcept { return Set<T>(lhs) + rhs; }
template<class T> INTRA_FORCEINLINE Requires<CSimdWrapper<T>, T> INTRA_VECTORCALL operator-(TScalarOf<T> lhs, T rhs) noexcept { return Set<T>(lhs) - rhs; }
template<class T> INTRA_FORCEINLINE Requires<CSimdWrapper<T>, T> INTRA_VECTORCALL operator*(TScalarOf<T> lhs, T rhs) noexcept { return Set<T>(lhs)* rhs; }
template<class T> INTRA_FORCEINLINE Requires<CSimdWrapper<T>, T> INTRA_VECTORCALL operator/(TScalarOf<T> lhs, T rhs) { return Set<T>(lhs) / rhs; }

template<class T> INTRA_FORCEINLINE Requires<CSimdWrapper<T>, T> INTRA_VECTORCALL operator-(T a) noexcept { return Set<T>(0) - a; }

template<class T> INTRA_FORCEINLINE Requires<CSimdWrapper<T>, IntAnalogOf<T>> INTRA_VECTORCALL operator&(T lhs, TScalarOf<T> rhs) noexcept {return lhs & Set<T>(rhs);}
template<class T> INTRA_FORCEINLINE Requires<CSimdWrapper<T>, IntAnalogOf<T>> INTRA_VECTORCALL operator|(T lhs, TScalarOf<T> rhs) noexcept {return lhs | Set<T>(rhs);}
template<class T> INTRA_FORCEINLINE Requires<CSimdWrapper<T>, IntAnalogOf<T>> INTRA_VECTORCALL operator^(T lhs, TScalarOf<T> rhs) noexcept {return lhs ^ Set<T>(rhs);}

template<class T> INTRA_FORCEINLINE Requires<CSimdWrapper<T>, IntAnalogOf<T>> INTRA_VECTORCALL operator&(TScalarOf<T> lhs, T rhs) noexcept {return Set<T>(lhs)& rhs;}
template<class T> INTRA_FORCEINLINE Requires<CSimdWrapper<T>, IntAnalogOf<T>> INTRA_VECTORCALL operator|(TScalarOf<T> lhs, T rhs) noexcept {return Set<T>(lhs) | rhs;}
template<class T> INTRA_FORCEINLINE Requires<CSimdWrapper<T>, IntAnalogOf<T>> INTRA_VECTORCALL operator^(TScalarOf<T> lhs, T rhs) noexcept {return Set<T>(lhs) ^ rhs;}

template<class T> INTRA_FORCEINLINE Requires<CSimdWrapper<T>, IntAnalogOf<T>> INTRA_VECTORCALL operator>(T lhs, TScalarOf<T> rhs) {return lhs > Set<T>(rhs);}
//template<class T> INTRA_FORCEINLINE Requires<CSimdWrapper<T>, IntAnalogOf<T>> INTRA_VECTORCALL operator<(T lhs, TScalarOf<T> rhs) {return lhs < Set<T>(rhs);}
template<class T> INTRA_FORCEINLINE Requires<CSimdWrapper<T>, IntAnalogOf<T>> INTRA_VECTORCALL operator>=(T lhs, TScalarOf<T> rhs) {return lhs >= Set<T>(rhs);}
template<class T> INTRA_FORCEINLINE Requires<CSimdWrapper<T>, IntAnalogOf<T>> INTRA_VECTORCALL operator<=(T lhs, TScalarOf<T> rhs) {return lhs <= Set<T>(rhs);}
template<class T> INTRA_FORCEINLINE Requires<CSimdWrapper<T>, IntAnalogOf<T>> INTRA_VECTORCALL operator==(T lhs, TScalarOf<T> rhs) noexcept {return lhs == Set<T>(rhs);}
template<class T> INTRA_FORCEINLINE Requires<CSimdWrapper<T>, IntAnalogOf<T>> INTRA_VECTORCALL operator!=(T lhs, TScalarOf<T> rhs) noexcept {return lhs != Set<T>(rhs);}

template<class T> INTRA_FORCEINLINE Requires<CSimdWrapper<T>, IntAnalogOf<T>> INTRA_VECTORCALL operator>(TScalarOf<T> lhs, T rhs) {return Set<T>(rhs) > lhs;}
//template<class T> INTRA_FORCEINLINE EnableForSimdWrapper<T, IntAnalogOf<T>> INTRA_VECTORCALL operator<(TScalarOf<T> lhs, T rhs) {return Set<T>(rhs) < lhs;}
template<class T> INTRA_FORCEINLINE Requires<CSimdWrapper<T>, IntAnalogOf<T>> INTRA_VECTORCALL operator>=(TScalarOf<T> lhs, T rhs) {return Set<T>(lhs) >= rhs;}
template<class T> INTRA_FORCEINLINE Requires<CSimdWrapper<T>, IntAnalogOf<T>> INTRA_VECTORCALL operator<=(TScalarOf<T> lhs, T rhs) {return Set<T>(lhs) <= rhs;}
template<class T> INTRA_FORCEINLINE Requires<CSimdWrapper<T>, IntAnalogOf<T>> INTRA_VECTORCALL operator==(TScalarOf<T> lhs, T rhs) noexcept {return Set<T>(lhs) == rhs;}
template<class T> INTRA_FORCEINLINE Requires<CSimdWrapper<T>, IntAnalogOf<T>> INTRA_VECTORCALL operator!=(TScalarOf<T> lhs, T rhs) noexcept {return Set<T>(lhs) != rhs;}

template<class T> INTRA_FORCEINLINE Requires<CSimdWrapper<T>, IntAnalogOf<T>> INTRA_VECTORCALL operator!(T lhs) noexcept { return lhs == 0; }
template<class T> INTRA_FORCEINLINE Requires<CSimdWrapper<T>, IntAnalogOf<T>> INTRA_VECTORCALL operator&&(T lhs, T rhs) noexcept { return (lhs != 0) & (rhs != 0); }
template<class T> INTRA_FORCEINLINE Requires<CSimdWrapper<T>, IntAnalogOf<T>> INTRA_VECTORCALL operator||(T lhs, T rhs) noexcept { return (lhs != 0) | (rhs != 0); }
template<class T> INTRA_FORCEINLINE Requires<CSimdWrapper<T>, IntAnalogOf<T>> INTRA_VECTORCALL operator&&(TScalarOf<T> s, T v) noexcept { return s? v != 0: Set<IntAnalogOf<T>>(0); }
template<class T> INTRA_FORCEINLINE Requires<CSimdWrapper<T>, IntAnalogOf<T>> INTRA_VECTORCALL operator&&(T v, TScalarOf<T> s) noexcept { return (v != 0) & (s? -1: 0); }

template<typename T> Requires<T::VectorSize == 2, T> INTRA_VECTORCALL Shuffle(T v, IntAnalogOf<T> indices)
{
	return {v[indices[0]], v[indices[1]]};
}

template<typename T> Requires<T::VectorSize == 8, T> INTRA_VECTORCALL Shuffle(T v, IntAnalogOf<T> indices)
{
	return {v[indices[0]], v[indices[1]], v[indices[2]], v[indices[3]],
		v[indices[4]], v[indices[5]], v[indices[6]], v[indices[7]]};
}

template<typename T> Requires<T::VectorSize == 16, T> INTRA_VECTORCALL Shuffle(T v, IntAnalogOf<T> indices)
{
	return {v[indices[0]], v[indices[1]], v[indices[2]], v[indices[3]],
		v[indices[4]], v[indices[5]], v[indices[6]], v[indices[7]],
		v[indices[8]], v[indices[9]], v[indices[10]], v[indices[11]],
		v[indices[12]], v[indices[13]], v[indices[14]], v[indices[15]]};
}

template<class T> INTRA_FORCEINLINE Requires<CSimdWrapper<T>, T&> INTRA_VECTORCALL operator|=(T& lhs, T rhs) noexcept {return lhs = lhs | rhs;}
template<class T> INTRA_FORCEINLINE Requires<CSimdWrapper<T>, T&> INTRA_VECTORCALL operator&=(T& lhs, T rhs) noexcept {return lhs = lhs & rhs;}
template<class T> INTRA_FORCEINLINE Requires<CSimdWrapper<T>, T&> INTRA_VECTORCALL operator^=(T& lhs, T rhs) noexcept {return lhs = lhs ^ rhs;}
template<class T> INTRA_FORCEINLINE Requires<CSimdWrapper<T>, T&> INTRA_VECTORCALL operator<<=(T& lhs, T rhs) noexcept {return lhs = lhs << rhs;}
template<class T> INTRA_FORCEINLINE Requires<CSimdWrapper<T>, T&> INTRA_VECTORCALL operator>>=(T& lhs, T rhs) noexcept {return lhs = lhs >> rhs;}

template<class T> INTRA_FORCEINLINE Requires<CSimdWrapper<T>, T&> INTRA_VECTORCALL operator|=(T& lhs, TScalarOf<T> rhs) noexcept {return lhs = lhs | rhs;}
template<class T> INTRA_FORCEINLINE Requires<CSimdWrapper<T>, T&> INTRA_VECTORCALL operator&=(T& lhs, TScalarOf<T> rhs) noexcept {return lhs = lhs & rhs;}
template<class T> INTRA_FORCEINLINE Requires<CSimdWrapper<T>, T&> INTRA_VECTORCALL operator^=(T& lhs, TScalarOf<T> rhs) noexcept {return lhs = lhs ^ rhs;}
template<class T> INTRA_FORCEINLINE Requires<CSimdWrapper<T>, T&> INTRA_VECTORCALL operator<<=(T& lhs, TScalarOf<T> rhs) noexcept {return lhs = lhs << rhs;}
template<class T> INTRA_FORCEINLINE Requires<CSimdWrapper<T>, T&> INTRA_VECTORCALL operator>>=(T& lhs, TScalarOf<T> rhs) noexcept {return lhs = lhs >> rhs;}

}
