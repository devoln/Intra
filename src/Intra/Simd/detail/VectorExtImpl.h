#pragma once

namespace Simd {

typedef int int4 __attribute__((vector_size(16), __may_alias__));
typedef float float4 __attribute__((vector_size(16), __may_alias__));

typedef int int8 __attribute__((vector_size(32), __may_alias__));
typedef float float8 __attribute__((vector_size(32), __may_alias__));

#define INTRA_SIMD_INT4_SUPPORT
#define INTRA_SIMD_FLOAT4_SUPPORT
#define INTRA_SIMD_INT8_SUPPORT
#define INTRA_SIMD_FLOAT8_SUPPORT

template<typename T> INTRA_FORCEINLINE T Set(TScalarOf<T> v) noexcept
{
	T result;
	for(int i = 0; i<sizeof(result)/sizeof(result[0]); i++) result[i] = v;
	return result;
}

INTRA_FORCEINLINE int4 Load4(const int* src) {return *reinterpret_cast<const int4*>(src);}
INTRA_FORCEINLINE int4 Load4Aligned(const int* src) {return *reinterpret_cast<const int4*>(INTRA_ASSUME_ALIGNED(src, sizeof(int4));}
INTRA_FORCEINLINE float4 Load4(const float* src) {return *reinterpret_cast<const float4*>(src);}
INTRA_FORCEINLINE float4 Load4Aligned(const float* src) {return *reinterpret_cast<const float4*>(INTRA_ASSUME_ALIGNED(src, sizeof(float4));}
INTRA_FORCEINLINE int8 Load8(const int* src) {return *reinterpret_cast<const int8*>(src);}
INTRA_FORCEINLINE int8 Load8Aligned(const int* src) {return *reinterpret_cast<const int8*>(INTRA_ASSUME_ALIGNED(src, sizeof(int8));}
INTRA_FORCEINLINE float8 Load8(const float* src) {return *reinterpret_cast<const float8*>(src));}
INTRA_FORCEINLINE float8 Load8Aligned(const float* src) {return *reinterpret_cast<const float8*>(INTRA_ASSUME_ALIGNED(src, sizeof(float8));}
template<typename T> INTRA_FORCEINLINE void Store(T v, TScalarOf<T>* dst) {*reinterpret_cast<T*>(dst) = v;}
template<typename T> INTRA_FORCEINLINE void StoreAligned(T v, TScalarOf<T>* dst) {*reinterpret_cast<T*>(INTRA_ASSUME_ALIGNED(dst, sizeof(T))) = v;}

template<int i0, int i1, int i2, int i3, typename T> INTRA_FORCEINLINE T Shuffle(T v) noexcept
{
	static_assert(
		0 <= i0 && i0 <= 3 &&
		0 <= i1 && i1 <= 3 &&
		0 <= i2 && i2 <= 3 &&
		0 <= i3 && i3 <= 3,
		"Valid range of shuffle indices is [0; 3]");
#ifdef __clang__
	return __builtin_shufflevector(v, int4{i0, i1, i2, i3});
#else
	return __builtin_shuffle(v, int4{i0,i1,i2,i3});
#endif
}

template<int i0, int i1, int i2, int i3, int i4, int i5, int i6, int i7, typename T> INTRA_FORCEINLINE T Shuffle(const T& v) noexcept
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
	return __builtin_shuffle(v, int8{i0,i1,i2,i3,i4,i5,i6,i7});
}

template<int n, typename T> INTRA_FORCEINLINE Requires<sizeof(T)/sizeof(T{}[0]) == 4, T> RotateLeft(const T& v) noexcept
{
	return Shuffle<n & 3, (n + 1) & 3, (n + 2) & 3, (n + 3) & 3>(v);
}

template<int n, typename T> INTRA_FORCEINLINE Requires<sizeof(T)/sizeof(T{}[0]) == 8, T> RotateLeft(const T& v) noexcept
{
	return Shuffle<n & 7, (n + 1) & 7, (n + 2) & 7, (n + 3) & 7, (n + 4) & 7, (n + 5) & 7, (n + 6) & 7, (n + 7) & 7>(v);
}

template<int n, typename T> INTRA_FORCEINLINE T RotateRight(const T& v) noexcept { return RotateLeft<-n>(v); }

template<typename T> INTRA_FORCEINLINE T Shuffle(T v, IntAnalogOf<T> indices) noexcept
{
	return __builtin_shuffle(v, indices);
}

#ifdef __clang__
template<typename T> INTRA_FORCEINLINE T Min(T a, T b) noexcept
{
	return ((a < b) & IntAnalogOf<T>(a)) | (~(a < b) & IntAnalogOf<T>(b));
}

template<typename T> INTRA_FORCEINLINE T Max(T a, T b) noexcept
{
	return ((a > b) & IntAnalogOf<T>(a)) | (~(a > b) & IntAnalogOf<T>(b));
}
#else
template<typename T> INTRA_FORCEINLINE T Min(T a, T b) noexcept { return a < b? a: b; }
template<typename T> INTRA_FORCEINLINE T Max(T a, T b) noexcept { return a > b? a: b; }
#endif

INTRA_FORCEINLINE int4 TruncateToInt(float4 v) noexcept
{
	return int4{int(v[0]), int(v[1]), int(v[2]), int(v[3])};
}

INTRA_FORCEINLINE float4 CastToFloat(int4 v) noexcept
{
	return float4{float(v[0]), float(v[1]), float(v[2]), float(v[3])};
}

INTRA_FORCEINLINE int8 TruncateToInt(float8 v) noexcept
{
	return int8{int(v[0]), int(v[1]), int(v[2]), int(v[3]), int(v[4]), int(v[5]), int(v[6]), int(v[7])};
}

INTRA_FORCEINLINE float8 CastToFloat(int8 v) noexcept
{
	return float8{float(v[0]), float(v[1]), float(v[2]), float(v[3]), float(v[4]), float(v[5]), float(v[6]), float(v[7])};
}

template<int i0a, int i1a, int i2b, int i3b, typename T> INTRA_FORCEINLINE T Shuffle22(const T& a, const T& b) noexcept
{
	static_assert(
		0 <= i0a && i0a <= 3 &&
		0 <= i1a && i1a <= 3 &&
		0 <= i2b && i2b <= 3 &&
		0 <= i3b && i3b <= 3,
		"Valid range of shuffle indices is [0; 3]");
	return T{a[i0a], a[i1a], b[i2b], b[i3b]};
}

INTRA_FORCEINLINE float4 INTRA_VECTORCALL InterleaveLow(float4 a, float4 b) noexcept {return float4{a[0], b[0], a[1], b[1]};}
INTRA_FORCEINLINE int4 INTRA_VECTORCALL InterleaveLow(int4 a, int4 b) noexcept {return int4{a[0], b[0], a[1], b[1]};}
INTRA_FORCEINLINE float4 INTRA_VECTORCALL InterleaveHigh(float4 a, float4 b) noexcept {return float4{a[2], b[2], a[3], b[3]};}
INTRA_FORCEINLINE int4 INTRA_VECTORCALL InterleaveHigh(int4 a, int4 b) noexcept {return int4{a[2], b[2], a[3], b[3]};}

INTRA_FORCEINLINE float8 INTRA_VECTORCALL InterleaveLow(float8 a, float8 b) noexcept {return float8{a[0], b[0], a[1], b[1], a[2], b[2], a[3], b[3]};}
INTRA_FORCEINLINE int8 INTRA_VECTORCALL InterleaveLow(int8 a, int8 b) noexcept {return int8{a[0], b[0], a[1], b[1], a[2], b[2], a[3], b[3]};}
INTRA_FORCEINLINE float8 INTRA_VECTORCALL InterleaveHigh(float8 a, float8 b) noexcept {return float8{a[4], b[4], a[5], b[5], a[6], b[6], a[7], b[7]};}
INTRA_FORCEINLINE int8 INTRA_VECTORCALL InterleaveHigh(int8 a, int8 b) noexcept {return int8{a[4], b[4], a[5], b[5], a[6], b[6], a[7], b[7]};}

INTRA_FORCEINLINE void End() noexcept {}

INTRA_FORCEINLINE int4 INTRA_VECTORCALL UnsignedRightBitShift(int4 x, int bits) noexcept
{
	return int4{int(unsigned(x[0]) >> bits), int(unsigned(x[1]) >> bits), int(unsigned(x[2]) >> bits), int(unsigned(x[3]) >> bits)};
}

INTRA_FORCEINLINE int8 INTRA_VECTORCALL UnsignedRightBitShift(int8 x, int bits) noexcept
{
	return int8{
		int(unsigned(x[0]) >> bits), int(unsigned(x[1]) >> bits), int(unsigned(x[2])) >> bits, int(unsigned(x[3]) >> bits),
		int(unsigned(x[4]) >> bits), int(unsigned(x[5]) >> bits), int(unsigned(x[6]) >> bits), int(unsigned(x[7]) >> bits)
	};
}

}
