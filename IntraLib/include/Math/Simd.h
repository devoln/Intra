#pragma once

#include "Core/Core.h"
#include "Math/MathEx.h"

#if(INTRA_PLATFORM_ARCH==INTRA_PLATFORM_X86 || INTRA_PLATFORM_ARCH==INTRA_PLATFORM_X86_64)

#define INTRA_SIMD_NONE 0
#define INTRA_SIMD_MMX 1
#define INTRA_SIMD_SSE 2
#define INTRA_SIMD_SSE2 3
#define INTRA_SIMD_SSE3 4
#define INTRA_SIMD_SSSE3 5
#define INTRA_SIMD_SSE4_1 6
#define INTRA_SIMD_SSE4_2 7
#define INTRA_SIMD_AVX 8
#define INTRA_SIMD_AVX2 9

#ifndef INTRA_MIN_SIMD_SUPPORT
//#define INTRA_MIN_SIMD_SUPPORT INTRA_SIMD_SSE2
#define INTRA_MIN_SIMD_SUPPORT INTRA_SIMD_NONE
#endif

#if(INTRA_MIN_SIMD_SUPPORT>INTRA_SIMD_NONE)
#include <mmintrin.h>  //MMX
#if(INTRA_MIN_SIMD_SUPPORT>INTRA_SIMD_MMX)
#include <xmmintrin.h> //SSE
#if(INTRA_MIN_SIMD_SUPPORT>INTRA_SIMD_SSE)
#include <emmintrin.h> //SSE2
#if(INTRA_MIN_SIMD_SUPPORT>INTRA_SIMD_SSE2)
#include <pmmintrin.h> //SSE3
#if(INTRA_MIN_SIMD_SUPPORT>INTRA_SIMD_SSE3)
#include <tmmintrin.h> //SSSE3
#if(INTRA_MIN_SIMD_SUPPORT>INTRA_SIMD_SSSE3)
#include <smmintrin.h> //SSE4.1
#if(INTRA_MIN_SIMD_SUPPORT>INTRA_SIMD_SSE4_1)
#include <nmmintrin.h> //SSE4.2
#if(INTRA_MIN_SIMD_SUPPORT>INTRA_SIMD_SSE4_2)
#include <immintrin.h> //AVX
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif

#elif defined(__ARM_NEON__) || defined(__ARM_NEON)
#include <arm_neon.h>
#endif



#if((INTRA_PLATFORM_ARCH==INTRA_PLATFORM_X86 || INTRA_PLATFORM_ARCH==INTRA_PLATFORM_X86_64) && INTRA_MIN_SIMD_SUPPORT>=INTRA_SIMD_MMX && INTRA_MIN_SIMD_SUPPORT<=INTRA_SIMD_AVX2)
namespace Intra { namespace Simd {

	typedef __m128i int4;
	typedef int4 int4arg;
	
	typedef __m128 float4;
	typedef float4 float4arg;

	typedef __m128d double2;
	typedef double2 double2arg;
	//typedef __m256d double4;
	//typedef double4 double4arg;

	forceinline float4 SetFloat4(float s) {return _mm_set1_ps(s);}
	forceinline int4 SetInt4(int s) {return _mm_set1_epi32(s);}
	forceinline double2 SetDouble2(double s) {return _mm_set1_pd(s);}

	forceinline int4 SetInt4(int x, int y, int z, int w) {return _mm_set_epi32(x, y, z, w);}
	forceinline float4 SetFloat4(float x, float y, float z, float w) {return _mm_set_ps(x, y, z, w);}
	forceinline double2 SetDouble2(double x, double y) {return _mm_set_pd(x,y);}

	forceinline float4 SetFloat4(const float xyzw[4]) {return _mm_load_ps(xyzw);}
	forceinline float4 SetFloat4U(const float xyzw[4]) {return _mm_loadu_ps(xyzw);}
	forceinline int4 SetInt4(const int xyzw[4]) {return _mm_set_epi32(xyzw[0], xyzw[1], xyzw[2], xyzw[3]);}
	forceinline double2 SetDouble2(const double xy[2]) {return _mm_load_pd(xy);}
	forceinline double2 SetDouble2U(const double xy[2]) {return _mm_loadu_pd(xy);}

	forceinline void Get(float* dst, float4arg v) {_mm_store_ps(dst, v);}
	forceinline void GetU(float* dst, float4arg v) {_mm_storeu_ps(dst, v);}
	forceinline float GetX(float4arg v) {float dst; _mm_store_ss(&dst, v); return dst;}
	forceinline double GetX(double2arg v) {double dst; _mm_store_sd(&dst, v); return dst;}

	forceinline float4 Abs(float4arg x)
	{
		static const float4 sign_mask = _mm_set_ps1(-0.0f);
		return _mm_andnot_ps(x, sign_mask);
	}

	forceinline double2 Abs(double2arg x)
	{
		static const double2 sign_mask = _mm_set1_pd(-0.0);
		return _mm_andnot_pd(sign_mask, x);
	}

	forceinline float4 Add(float4arg a, float4arg b) {return _mm_add_ps(a, b);}
	forceinline double2 Add(double2arg a, double2arg b) {return _mm_add_pd(a, b);}

	forceinline float4 Sub(float4arg a, float4arg b) {return _mm_sub_ps(a, b);}
	forceinline double2 Sub(double2arg a, double2arg b) {return _mm_sub_pd(a, b);}

	forceinline float4 Mul(float4arg a, float4arg b) {return _mm_mul_ps(a, b);}
	forceinline double2 Mul(double2arg a, double2arg b) {return _mm_mul_pd(a, b);}

	forceinline float4 Dot4(float4arg a, float4arg b)
	{
		float4 m = _mm_mul_ps(a, b);
		float4 t = _mm_add_ps(m, _mm_shuffle_ps(m, m, _MM_SHUFFLE(2, 3, 0, 1)));
		return _mm_add_ps(t, _mm_shuffle_ps(t, t, _MM_SHUFFLE(1, 0, 3, 2)));
	}

	forceinline double2 Dot2(double2arg a, double2arg b)
	{
		double2 m = _mm_mul_pd(a, b);
		return _mm_add_pd(m, _mm_shuffle_pd(m, m, _MM_SHUFFLE2(1, 0)));
	}

	forceinline float Dot(float4arg a, float4arg b)
	{
		return GetX(Dot4(a, b));
	}

	forceinline double Dot(double2arg a, double2arg b) {return GetX(Dot2(a, b));}

	forceinline float4 Min(float4arg a, float4arg b) {return _mm_min_ps(a, b);}
	forceinline double2 Min(double2arg a, double2arg b) {return _mm_min_pd(a, b);}
	forceinline float4 Max(float4arg a, float4arg b) {return _mm_max_ps(a, b);}
	forceinline double2 Max(double2arg a, double2arg b) {return _mm_max_pd(a, b);}

	forceinline float4 MinSingle(float4arg a, float4arg b) {return _mm_min_ss(a, b);}
	forceinline double2 MinSingle(double2arg a, double2arg b) {return _mm_min_sd(a, b);}
	forceinline float4 MaxSingle(float4arg a, float4arg b) {return _mm_max_ss(a, b);}
	forceinline double2 MaxSingle(double2arg a, double2arg b) {return _mm_max_sd(a, b);}

	forceinline float4 Negate(float4arg a)
	{
		static const float4 sign_mask = _mm_set_ps1(-0.0);
		return _mm_xor_ps(a, sign_mask);
	}

	forceinline double2 Negate(double2arg a)
	{
		static const double2 sign_mask = _mm_set1_pd(-0.0);
		return _mm_xor_pd(a, sign_mask);
	}

	forceinline float4 Sqrt(float4arg a)
	{
		return _mm_sqrt_ps(a);
	}

	forceinline double2 Sqrt(double2arg a)
	{
		return _mm_sqrt_pd(a);
	}

	// a,b,c,d -> b,c,d,a
	forceinline float4 Rotate(float4arg ps)
	{
		return _mm_shuffle_ps(ps,(ps), 0x39);
	}

	forceinline float4 Z1W1Z2W2(float4arg a, float4arg b)
	{
		return _mm_movehl_ps(a, b);
	}

	forceinline bool EqualX(float4arg a, float4arg b)
	{
		return _mm_comieq_ss(a, b)!=0;
	}

	forceinline bool EqualX(double2arg a, double2arg b)
	{
		return _mm_comieq_sd(a, b)!=0;
	}

	forceinline bool NotEqualX(float4arg a, float4arg b)
	{
		return _mm_comineq_ss(a, b)!=0;
	}

	forceinline bool NotEqualX(double2arg a, double2arg b)
	{
		return _mm_comineq_sd(a, b)!=0;
	}

	forceinline bool LessX(float4arg a, float4arg b)
	{
		return _mm_comilt_ss(a, b)!=0;
	}

	forceinline bool LessX(double2arg a, double2arg b)
	{
		return _mm_comilt_sd(a, b)!=0;
	}

	forceinline bool LessEqualX(float4arg a, float4arg b)
	{
		return _mm_comile_ss(a, b)!=0;
	}

	forceinline bool LessEqualX(double2arg a, double2arg b)
	{
		return _mm_comile_sd(a, b)!=0;
	}

	forceinline bool GreaterX(float4arg a, float4arg b)
	{
		return _mm_comigt_ss(a, b)!=0;
	}

	forceinline bool GreaterX(double2arg a, double2arg b)
	{
		return _mm_comigt_sd(a, b)!=0;
	}

	forceinline bool GreaterEqualX(float4arg a, float4arg b)
	{
		return _mm_comige_ss(a, b)!=0;
	}

	forceinline bool GreaterEqualX(double2arg a, double2arg b)
	{
		return _mm_comige_sd(a, b)!=0;
	}

}}

#elif(INTRA_PLATFORM_ARCH==INTRA_PLATFORM_PowerPC && INTRA_MIN_SIMD_SUPPORT!=INTRA_SIMD_NONE)

namespace Intra { namespace Simd {

	typedef __vector4 float4;
	typedef float4 float4arg;

}}

#elif(defined(__ARM_NEON) || defined(__ARM_NEON__))

namespace Intra { namespace Simd {

typedef uint32x2_t uint2;
typedef uint16x4_t ushort4;
typedef uint8x8_t byte8;
typedef int32x2_t int2;
typedef int16x4_t short4;
typedef int8x8_t sbyte8;
typedef uint64x1_t ullong1;
typedef int64x1_t llong1;
typedef float32x2_t float2;
typedef uint32x4_t uint4;
typedef uint16x8_t ushort8;
typedef uint8x16_t byte16;
typedef int32x4_t int4;
typedef int16x8_t short8;
typedef int8x16_t sbyte16;
typedef uint64x2_t ullong2;
typedef int64x2_t llong2;
typedef float32x4_t float4;

typedef uint32x2_t uint2arg;
typedef uint16x4_t ushort4arg;
typedef uint8x8_t byte8arg;
typedef int32x2_t int2arg;
typedef int16x4_t short4arg;
typedef int8x8_t sbyte8arg;
typedef uint64x1_t ullong1arg;
typedef int64x1_t llong1arg;
typedef float32x2_t float2arg;
typedef uint32x4_t uint4arg;
typedef uint16x8_t ushort8arg;
typedef uint8x16_t byte16arg;
typedef int32x4_t int4arg;
typedef int16x8_t short8arg;
typedef int8x16_t sbyte16arg;
typedef uint64x2_t ullong2arg;
typedef int64x2_t llong2arg;
typedef float32x4_t float4arg;

struct double2 {double v[2];};
typedef const double2& double2arg;
struct double4 {double v[4];};
typedef const double4& double4arg;

forceinline float2 SetFloat2(float s) {return vdup_n_f32(s);}
forceinline float4 SetFloat4(float s) {return vdupq_n_f32(s);}
forceinline int2 SetInt2(int s) {return vdup_n_s32(s);}
forceinline int4 SetInt4(int s) {return vdupq_n_s32(s);}

forceinline int2 SetInt2(const int xy[2]) {return vld1_s32(xy);}
forceinline int4 SetInt4(const int xyzw[4]) {return vld1q_s32(xyzw);}
forceinline float2 SetFloat2(const float xy[2]) {return vld1_f32(xy);}
forceinline float4 SetFloat4(const float xyzw[4]) {return vld1q_f32(xyzw);}

forceinline int2 SetInt2(int x, int y) {int xy[]={x, y}; return SetInt2(xy);}
forceinline int4 SetInt4(int x, int y, int z, int w) {int xyzw[]={x, y, z, w}; return SetInt4(xyzw);}
forceinline float2 SetFloat2(float x, float y) {float xy[]={x, y}; return SetFloat2(xy);}
forceinline float4 SetFloat4(float x, float y, float z, float w) {float xyzw[]={x, y, z, w}; return SetFloat4(xyzw);}

forceinline void Get(float* dst, float2arg v) {vst1_f32(dst, v);}
forceinline void Get(float* dst, float4arg v) {vst1q_f32(dst, v);}
forceinline float GetX(float2arg v) {float dst[2]; Get(dst, v); return dst[0];}
forceinline float GetX(float4arg v) {float dst[4]; Get(dst, v); return dst[0];}
forceinline void Get(int* dst, int2arg v) {vst1_s32(dst, v);}
forceinline void Get(int* dst, int4arg v) {vst1q_s32(dst, v);}
forceinline int GetX(int2arg v) {int dst[2]; Get(dst, v); return dst[0];}
forceinline int GetX(int4arg v) {int dst[4]; Get(dst, v); return dst[0];}

forceinline int2 Add(int2arg a, int2arg b) {return vadd_s32(a, b);}
forceinline int4 Add(int4arg a, int4arg b) {return vaddq_s32(a, b);}
forceinline float2 Add(float2arg a, float2arg b) {return vadd_f32(a, b);}
forceinline float4 Add(float4arg a, float4arg b) {return vaddq_f32(a, b);}

forceinline int2 Mul(int2arg a, int2arg b) {return vmul_s32(a, b);}
forceinline int4 Mul(int4arg a, int4arg b) {return vmulq_s32(a, b);}
forceinline float2 Mul(float2arg a, float2arg b) {return vmul_f32(a, b);}
forceinline float4 Mul(float4arg a, float4arg b) {return vmulq_f32(a, b);}

forceinline int2 Sub(int2arg a, int2arg b) {return vsub_s32(a, b);}
forceinline int4 Sub(int4arg a, int4arg b) {return vsubq_s32(a, b);}
forceinline float2 Sub(float2arg a, float2arg b) {return vsub_f32(a, b);}
forceinline float4 Sub(float4arg a, float4arg b) {return vsubq_f32(a, b);}

}}

#else

namespace Intra { namespace Simd {

	struct int4 {int v[4];};
	struct float4 {float v[4];};
	typedef const int4& int4arg;
	typedef const float4& float4arg;
	struct double2 {double v[2];};
	typedef const double2& double2arg;
	struct double4 {double v[4];};
	typedef const double4& double4arg;


	forceinline float4 SetFloat4(float s) {return {{s,s,s,s}};}
	forceinline int4 SetInt4(int s) {return {{s,s,s,s}};}

	forceinline double2 SetDouble2(double s) {return {{s,s}};}
	forceinline float4 SetFloat4(float x, float y, float z, float w) {return {{x,y,z,w}};}
	forceinline double2 SetDouble2(double x, double y) {return {{x,y}};}

	forceinline void Get(float* dst, float4arg v) {dst[0]=v.v[0]; dst[1]=v.v[1]; dst[2]=v.v[2]; dst[3]=v.v[3];}
	forceinline float GetX(float4arg v) {return v.v[0];}
	forceinline double GetX(double2arg v) {return v.v[0];}

	forceinline float4 Abs(float4arg x)
	{
		return {{Math::Abs(x.v[0]), Math::Abs(x.v[1]), Math::Abs(x.v[2]), Math::Abs(x.v[3])}};
	}

	forceinline double2 Abs(double2arg x)
	{
		return {{Math::Abs(x.v[0]), Math::Abs(x.v[1])}};
	}

	forceinline float4 Add(float4arg a, float4arg b)
	{
		return {{a.v[0]+b.v[0], a.v[1]+b.v[1], a.v[2]+b.v[2], a.v[3]+b.v[3]}};
	}

	forceinline double2 Add(double2arg a, double2arg b)
	{
		return {{a.v[0]+b.v[0], a.v[1]+b.v[1]}};
	}

	forceinline float4 Sub(float4arg a, float4arg b)
	{
		return {{a.v[0]-b.v[0], a.v[1]-b.v[1], a.v[2]-b.v[2], a.v[3]-b.v[3]}};
	}

	forceinline double2 Sub(double2arg a, double2arg b)
	{
		return {{a.v[0]-b.v[0], a.v[1]-b.v[1]}};
	}

	forceinline float4 Mul(float4arg a, float4arg b)
	{
		return {{a.v[0]*b.v[0], a.v[1]*b.v[1], a.v[2]*b.v[2], a.v[3]*b.v[3]}};
	}

	forceinline double2 Mul(double2arg a, double2arg b)
	{
		return {{a.v[0]*b.v[0], a.v[1]*b.v[1]}};
	}

	forceinline float Dot(float4arg a, float4arg b)
	{
		return a.v[0]*b.v[0] + a.v[1]*b.v[1] + a.v[2]*b.v[2] + a.v[3]*b.v[3];
	}

	forceinline double Dot(double2arg a, double2arg b)
	{
		return a.v[0]*b.v[0] + a.v[1]*b.v[1];
	}

	forceinline float4 Dot4(float4arg a, float4arg b)
	{
		return SetFloat4(Dot(a, b));
	}

	forceinline double2 Dot2(double2arg a, double2arg b)
	{
		return SetDouble2(Dot(a, b));
	}


	forceinline float4 Min(float4arg a, float4arg b)
	{
		return {{Math::Min(a.v[0], b.v[0]), Math::Min(a.v[1], b.v[1]), Math::Min(a.v[2], b.v[2]), Math::Min(a.v[3], b.v[3])}};
	}

	forceinline double2 Min(double2arg a, double2arg b)
	{
		return {{Math::Min(a.v[0], b.v[0]), Math::Min(a.v[1], b.v[1])}};
	}

	forceinline float4 Max(float4arg a, float4arg b)
	{
		return {{Math::Max(a.v[0], b.v[0]), Math::Max(a.v[1], b.v[1]), Math::Max(a.v[2], b.v[2]), Math::Max(a.v[3], b.v[3])}};
	}

	forceinline double2 Max(double2arg a, double2arg b)
	{
		return {{Math::Max(a.v[0], b.v[0]), Math::Max(a.v[1], b.v[1])}};
	}

	forceinline float4 MinSingle(float4arg a, float4arg b)
	{
		return {{Math::Min(a.v[0], b.v[0]), a.v[1], a.v[2], a.v[3]}};
	}

	forceinline double2 MinSingle(double2arg a, double2arg b)
	{
		return {{Math::Min(a.v[0], b.v[0]), a.v[1]}};
	}

	forceinline float4 MaxSingle(float4arg a, float4arg b)
	{
		return {{Math::Max(a.v[0], b.v[0]), a.v[1], a.v[2], a.v[3]}};
	}

	forceinline double2 MaxSingle(double2arg a, double2arg b)
	{
		return {{Math::Max(a.v[0], b.v[0]), a.v[1]}};
	}

	forceinline float4 Negate(float4arg a)
	{
		return {{-a.v[0], -a.v[1], -a.v[2], -a.v[3]}};
	}

	forceinline double2 Negate(double2arg a)
	{
		return {{-a.v[0], -a.v[1]}};
	}

	// a,b,c,d -> b,c,d,a
	forceinline float4 Rotate(float4arg a)
	{
		return {{a.v[1], a.v[2], a.v[3], a.v[0]}};
	}

	forceinline float4 Z1W1Z2W2(float4arg a, float4arg b)
	{
		return {{a.v[2], a.v[3], b.v[2], b.v[3]}};
	}

	forceinline bool EqualX(float4arg a, float4arg b)
	{
		return a.v[0]==b.v[0];
	}

	forceinline bool EqualX(double2arg a, double2arg b)
	{
		return a.v[0]==b.v[0];
	}

	forceinline bool NotEqualX(float4arg a, float4arg b)
	{
		return a.v[0]!=b.v[0];
	}

	forceinline bool NotEqualX(double2arg a, double2arg b)
	{
		return a.v[0]!=b.v[0];
	}

	forceinline bool LessX(float4arg a, float4arg b)
	{
		return a.v[0]<b.v[0];
	}

	forceinline bool LessX(double2arg a, double2arg b)
	{
		return a.v[0]<b.v[0];
	}

	forceinline bool LessEqualX(float4arg a, float4arg b)
	{
		return a.v[0]<=b.v[0];
	}

	forceinline bool LessEqualX(double2arg a, double2arg b)
	{
		return a.v[0]<=b.v[0];
	}

	forceinline bool GreaterX(float4arg a, float4arg b)
	{
		return a.v[0]>b.v[0];
	}

	forceinline bool GreaterX(double2arg a, double2arg b)
	{
		return a.v[0]>b.v[0];
	}

	forceinline bool GreaterEqualX(float4arg a, float4arg b)
	{
		return a.v[0]>=b.v[0];
	}

	forceinline bool GreaterEqualX(double2arg a, double2arg b)
	{
		return a.v[0]>=b.v[0];
	}

}}

#endif

namespace Intra { namespace Simd {

	namespace Impl
	{
		template<typename T> struct simd2;
		template<typename T> struct simd4;
		template<> struct simd4<float> {typedef float4 Type;};
		template<> struct simd2<double> {typedef double2 Type;};
		//template<> struct simd4<double> {typedef double4 Type;};
	}
	template<typename T> using simd4 = typename Impl::simd4<T>::Type;
}}

