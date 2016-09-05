#pragma once

#if(INTRA_PLATFORM_ARCH==INTRA_PLATFORM_X86 || INTRA_PLATFORM_ARCH==INTRA_PLATFORM_X86_64)

#define SIMD_NONE 0
#define SIMD_MMX 1
#define SIMD_SSE 2
#define SIMD_SSE2 3
#define SIMD_SSE3 4
#define SIMD_SSSE3 5
#define SIMD_SSE4_1 6
#define SIMD_SSE4_2 7
#define SIMD_AVX 8

#ifndef MIN_SIMD_SUPPORT
#define MIN_SIMD_SUPPORT SIMD_SSE2
#endif

#if(MIN_SIMD_SUPPORT>SIMD_NONE)
#include <mmintrin.h>  //MMX
#if(MIN_SIMD_SUPPORT>SIMD_MMX)
#include <xmmintrin.h> //SSE
#if(MIN_SIMD_SUPPORT>SIMD_SSE)
#include <emmintrin.h> //SSE2
#if(MIN_SIMD_SUPPORT>SIMD_SSE2)
#include <pmmintrin.h> //SSE3
#if(MIN_SIMD_SUPPORT>SIMD_SSE3)
#include <tmmintrin.h> //SSSE3
#if(MIN_SIMD_SUPPORT>SIMD_SSSE3)
#include <smmintrin.h> //SSE4.1
#if(MIN_SIMD_SUPPORT>SIMD_SSE4_1)
#include <nmmintrin.h> //SSE4.2
#if(MIN_SIMD_SUPPORT>SIMD_SSE4_2)
#include <immintrin.h> //AVX
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif

#elif PLATFORM==INTRA_PLATFORM_ARM
#include <arm_neon.h>
#endif


namespace Intra { namespace Simd {
	forceinline bool ToBool(int x) {return *(bool*)&x;} //Быстрый перевод в bool, если x принимает значения только 0 или 1
#if INTRA_PLATFORM_ARCH==INTRA_PLATFORM_X86 || INTRA_PLATFORM_ARCH==INTRA_PLATFORM_X86_64
	typedef __m128i int4;
	typedef int4 int4arg;
	
	typedef __m128 float4;
	typedef float4 float4arg;

	typedef __m128d double2;
	typedef double2 double2arg;
	//typedef __m256d double4;
	//typedef double4 double4arg;

	forceinline float4 Set(float s) {return _mm_set1_ps(s);}
	forceinline int4 Set(int s) {return _mm_set1_epi32(s);}

	forceinline double2 Set(double s) {return _mm_set1_pd(s);}
	forceinline float4 Set(float x, float y, float z, float w) {return _mm_set_ps(x, y, z, w);}
	forceinline double2 Set(double x, double y) {return _mm_set_pd(x,y);}

	forceinline float4 Set(const float xyzw[4]) {return _mm_load_ps(xyzw);}
	forceinline float4 SetU(const float xyzw[4]) {return _mm_loadu_ps(xyzw);}
	forceinline double2 Set(const double xy[2]) {return _mm_load_pd(xy);}
	forceinline double2 SetU(const double xy[2]) {return _mm_loadu_pd(xy);}

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

	forceinline float4 Add(float4arg a, float4arg b)
	{
		return _mm_add_ps(a, b);
	}

	forceinline double2 Add(double2arg a, double2arg b)
	{
		return _mm_add_pd(a, b);
	}

	forceinline float4 Sub(float4arg a, float4arg b)
	{
		return _mm_sub_ps(a, b);
	}

	forceinline double2 Sub(double2arg a, double2arg b)
	{
		return _mm_sub_pd(a, b);
	}

	forceinline float4 Mul(float4arg a, float4arg b)
	{
		return _mm_mul_ps(a, b);
	}

	forceinline double2 Mul(double2arg a, double2arg b)
	{
		return _mm_mul_pd(a, b);
	}

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

	forceinline double Dot(double2arg a, double2arg b)
	{
		return GetX(Dot2(a, b));
	}

	forceinline float4 Min(float4arg a, float4arg b)
	{
		return _mm_min_ps(a, b);
	}

	forceinline double2 Min(double2arg a, double2arg b)
	{
		return _mm_min_pd(a, b);
	}

	forceinline float4 Max(float4arg a, float4arg b)
	{
		return _mm_max_ps(a, b);
	}

	forceinline double2 Max(double2arg a, double2arg b)
	{
		return _mm_max_pd(a, b);
	}

	forceinline float4 MinSingle(float4arg a, float4arg b)
	{
		return _mm_min_ss(a, b);
	}

	forceinline double2 MinSingle(double2arg a, double2arg b)
	{
		return _mm_min_sd(a, b);
	}

	forceinline float4 MaxSingle(float4arg a, float4arg b)
	{
		return _mm_max_ss(a, b);
	}

	forceinline double2 MaxSingle(double2arg a, double2arg b)
	{
		return _mm_max_sd(a, b);
	}

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
		return ToBool(_mm_comieq_ss(a, b));
	}

	forceinline bool EqualX(double2arg a, double2arg b)
	{
		return ToBool(_mm_comieq_sd(a, b));
	}

	forceinline bool NotEqualX(float4arg a, float4arg b)
	{
		return ToBool(_mm_comineq_ss(a, b));
	}

	forceinline bool NotEqualX(double2arg a, double2arg b)
	{
		return ToBool(_mm_comineq_sd(a, b));
	}

	forceinline bool LessX(float4arg a, float4arg b)
	{
		return ToBool(_mm_comilt_ss(a, b));
	}

	forceinline bool LessX(double2arg a, double2arg b)
	{
		return ToBool(_mm_comilt_sd(a, b));
	}

	forceinline bool LessEqualX(float4arg a, float4arg b)
	{
		return ToBool(_mm_comile_ss(a, b));
	}

	forceinline bool LessEqualX(double2arg a, double2arg b)
	{
		return ToBool(_mm_comile_sd(a, b));
	}

	forceinline bool GreaterX(float4arg a, float4arg b)
	{
		return ToBool(_mm_comigt_ss(a, b));
	}

	forceinline bool GreaterX(double2arg a, double2arg b)
	{
		return ToBool(_mm_comigt_sd(a, b));
	}

	forceinline bool GreaterEqualX(float4arg a, float4arg b)
	{
		return ToBool(_mm_comige_ss(a, b));
	}

	forceinline bool GreaterEqualX(double2arg a, double2arg b)
	{
		return ToBool(_mm_comige_sd(a, b));
	}

#elif INTRA_PLATFORM_ARCH==INTRA_PLATFORM_PowerPC
	typedef __vector4 float4;
	typedef float4 float4arg;
#else
	struct float4 {float v[4];};
	typedef const float4& float4arg;
	struct double2 {double v[2];};
	typedef const double2& double2arg;
	struct double4 {double v[4];};
	typedef const double4& double4arg;


	forceinline float4 Set(float s) {return {s,s,s,s};}
	forceinline int4 Set(int s) {return {s,s,s,s};}

	forceinline double2 Set(double s) {return {s,s};}
	forceinline float4 Set(float x, float y, float z, float w) {return {x,y,z,w};}
	forceinline double2 Set(double x, double y) {return {x,y};}

	forceinline void Get(float* dst, float4arg v) {dst[0]=v.v[0]; dst[1]=v.v[1]; dst[2]=v.v[2]; dst[3]=v.v[3];}
	forceinline float GetX(float4arg v) {return v.v[0];}
	forceinline double GetX(double2arg v) {return v.v[0];}

	forceinline float4 Abs(float4arg x)
	{
		return {Math::Abs(x.v[0]), Math::Abs(x.v[1]), Math::Abs(x.v[2]), Math::Abs(x.v[3])};
	}

	forceinline double2 Abs(double2arg x)
	{
		return {Math::Abs(x.v[0]), Math::Abs(x.v[1])};
	}

	forceinline float4 Add(float4arg a, float4arg b)
	{
		return {a.v[0]+b.v[0], a.v[1]+b.v[1], a.v[2]+b.v[2], a.v[3]+b.v[3]};
	}

	forceinline double2 Add(double2arg a, double2arg b)
	{
		return {a.v[0]+b.v[0], a.v[1]+b.v[1]};
	}

	forceinline float4 Sub(float4arg a, float4arg b)
	{
		return {a.v[0]-b.v[0], a.v[1]-b.v[1], a.v[2]-b.v[2], a.v[3]-b.v[3]};
	}

	forceinline double2 Sub(double2arg a, double2arg b)
	{
		return {a.v[0]-b.v[0], a.v[1]-b.v[1]};
	}

	forceinline float4 Mul(float4arg a, float4arg b)
	{
		return {a.v[0]*b.v[0], a.v[1]*b.v[1], a.v[2]*b.v[2], a.v[3]*b.v[3]};
	}

	forceinline double2 Mul(double2arg a, double2arg b)
	{
		return {a.v[0]*b.v[0], a.v[1]*b.v[1]};
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
		return Set(Dot(a, b));
	}

	forceinline double2 Dot2(double2arg a, double2arg b)
	{
		return Set(Dot(a, b));
	}


	forceinline float4 Min(float4arg a, float4arg b)
	{
		return {Math::Min(a.v[0], b.v[0]), Math::Min(a.v[1], b.v[1]), Math::Min(a.v[2], b.v[2]), Math::Min(a.v[3], b.v[3])};
	}

	forceinline double2 Min(double2arg a, double2arg b)
	{
		return {Math::Min(a.v[0], b.v[0]), Math::Min(a.v[1], b.v[1])};
	}

	forceinline float4 Max(float4arg a, float4arg b)
	{
		return {Math::max(a.v[0], b.v[0]), Math::max(a.v[1], b.v[1]), Math::max(a.v[2], b.v[2]), Math::max(a.v[3], b.v[3])};
	}

	forceinline double2 Max(double2arg a, double2arg b)
	{
		return {Math::max(a.v[0], b.v[0]), Math::max(a.v[1], b.v[1])};
	}

	forceinline float4 MinSingle(float4arg a, float4arg b)
	{
		return {Math::Min(a.v[0], b.v[0]), a.v[1], a.v[2], a.v[3]};
	}

	forceinline double2 MinSingle(double2arg a, double2arg b)
	{
		return {Math::Min(a.v[0], b.v[0]), a.v[1]};
	}

	forceinline float4 MaxSingle(float4arg a, float4arg b)
	{
		return {Math::max(a.v[0], b.v[0]), a.v[1], a.v[2], a.v[3]};
	}

	forceinline double2 MaxSingle(double2arg a, double2arg b)
	{
		return {Math::max(a.v[0], b.v[0]), a.v[1]};
	}

	forceinline float4 Negate(float4arg a)
	{
		return {-a.v[0], -a.v[1], -a.v[2], -a.v[3]};
	}

	forceinline double2 Negate(double2arg a)
	{
		return {-a.v[0], -a.v[1]};
	}

	// a,b,c,d -> b,c,d,a
	forceinline float4 Rotate(float4arg a)
	{
		return {a.v[1], a.v[2], a.v[3], a.v[0]};
	}

	forceinline float4 Z1W1Z2W2(float4arg a, float4arg b)
	{
		return {a.v[2], a.v[3], b.v[2], b.v[3]};
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


#endif

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
