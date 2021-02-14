#pragma once

#include "Intra/Core.h"
#include "Intra/Math/Math.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Matrix3.h"

/** This header file provides GLSL and HLSL-like wrappers to make porting GLSL or HLSL to C++ easier.

  Include this file, use namespace Intra::Math and Intra::ShaderMath to make vector classes and functions available.
*/

#ifndef NOMINMAX
#define NOMINMAX
#undef min
#undef max
#endif

namespace Intra { INTRA_BEGIN
namespace ShaderMath {

template<typename T> [[nodiscard]] constexpr T min(const T& t1, const T& t2) {return Min(t1, t2);}
template<typename T> [[nodiscard]] constexpr T max(const T& t1, const T& t2) {return Max(t1, t2);}

template<typename T> [[nodiscard]] constexpr T sign(T x) {return Sign(x);}
template<typename T> [[nodiscard]] constexpr T abs(T x) {return Abs(x);}

template<typename T> [[nodiscard]] T floor(T v) {return Floor(v);}
template<typename T> [[nodiscard]] T ceil(T v) {return Ceil(v);}
template<typename T> [[nodiscard]] T round(T v) {return Round(v);}

template<typename T> [[nodiscard]] T fract(T v) {return Fract(v);}

template<typename T> [[nodiscard]] T sin(T v) {return Sin(v);}
template<typename T> [[nodiscard]] T cos(T v) {return Cos(v);}
template<typename T> [[nodiscard]] T tan(T v) {return Tan(v);}

template<typename T> [[nodiscard]] T sinh(T v) {return Sinh(v);}
template<typename T> [[nodiscard]] T cosh(T v) {return Cosh(v);}
template<typename T> [[nodiscard]] T tanh(T v) {return Tanh(v);}

template<typename T> [[nodiscard]] T asin(T v) {return Asin(v);}
template<typename T> [[nodiscard]] T acos(T v) {return Acos(v);}
template<typename T> [[nodiscard]] T atan(T v) {return Atan(v);}

template<typename T> [[nodiscard]] T sqrt(T v) {return Sqrt(v);}
template<typename T> [[nodiscard]] T log(T v) {return Log(v);}

template<typename T> [[nodiscard]] T mod(T x, T y) {return Mod(x, y);}
template<typename T> [[nodiscard]] T pow(T x, T power) {return Pow(x, power);}

template<typename T, typename N, typename X> [[nodiscard]] constexpr
auto clamp(T v, N minv, X maxv) {return Clamp(v, minv, maxv);}

template<typename T, typename U> [[nodiscard]] constexpr T mix(T x, T y, U factor) {return LinearMix(x, y, factor);}

template<typename T> [[nodiscard]] constexpr T step(T edge, T value) {return Step(edge, value);}
template<typename T> [[nodiscard]] constexpr T smoothstep(T edge0, T edge1, T value) {return SmoothStep(edge0, edge1, value);}


template<typename T> constexpr T dot(const Vector2<T>& l, const Vector2<T>& r) {return Dot(l, r);}
template<typename T> T length(const Vector2<T>& v) {return Length(v);}
template<typename T> T distance(const Vector2<T>& l, const Vector2<T>& r) {return Distance(l, r);}
template<typename T> Vector2<T> normalize(const Vector2<T>& v) {return Normalize(v);}
template<typename T> constexpr Vector2<T> reflect(const Vector2<T>& incident, const Vector2<T>& normal) {return Reflect(incident, normal);}
template<typename T> Vector2<T> refract(const Vector2<T>& I, const Vector2<T>& N, float eta) {return Refract(I, N, eta);}
template<typename T> constexpr Vector2<T> faceforward(const Vector2<T>& N, const Vector2<T>& I, const Vector2<T>& Nref) {return FaceForward(N, I, Nref);}

typedef Vector2<float> vec2;
typedef Vector2<double> dvec2;
typedef Vector2<int> ivec2;
typedef Vector2<unsigned> uvec2;
typedef Vector2<bool> bvec2;


template<typename T> constexpr T dot(const Vector3<T>& l, const Vector3<T>& r) {return Dot(l, r);}
template<typename T> T length(const Vector3<T>& v) {return Length(v);}
template<typename T> T distance(const Vector3<T>& l, const Vector3<T>& r) {return Distance(l, r);}
template<typename T> Vector3<T> normalize(const Vector3<T>& v) {return Normalize(v);}
template<typename T> constexpr Vector3<T> reflect(const Vector3<T>& incident, const Vector3<T>& normal) {return Reflect(incident, normal);}
template<typename T> Vector3<T> refract(const Vector3<T>& I, const Vector3<T>& N, float eta) {return Refract(I, N, eta);}
template<typename T> constexpr Vector3<T> faceforward(const Vector3<T>& N, const Vector3<T>& I, const Vector3<T>& Nref) {return FaceForward(N, I, Nref);}
template<typename T> constexpr Vector3<T> cross(const Vector3<T>& v1, const Vector3<T>& v2) {return Cross(v1, v2);}

typedef Vector3<float> vec3;
typedef Vector3<double> dvec3;
typedef Vector3<int> ivec3;
typedef Vector3<unsigned> uvec3;
typedef Vector3<bool> bvec3;


template<typename T> constexpr T dot(const Vector4<T>& l, const Vector4<T>& r) {return Dot(l, r);}
template<typename T> T length(const Vector4<T>& v) {return Length(v);}
template<typename T> T distance(const Vector4<T>& l, const Vector4<T>& r) {return Distance(l, r);}
template<typename T> Vector4<T> normalize(const Vector4<T>& v) {return Normalize(v);}
template<typename T> constexpr Vector4<T> reflect(const Vector4<T>& incident, const Vector4<T>& normal) {return Reflect(incident, normal);}
template<typename T> Vector4<T> refract(const Vector4<T>& I, const Vector4<T>& N, float eta) {return Refract(I, N, eta);}
template<typename T> constexpr Vector4<T> faceforward(const Vector4<T>& N, const Vector4<T>& I, const Vector4<T>& Nref) {return FaceForward(N, I, Nref);}

typedef Vector4<float> vec4;
typedef Vector4<double> dvec4;
typedef Vector4<int> ivec4;
typedef Vector4<unsigned> uvec4;
typedef Vector4<bool> bvec4;


typedef Matrix3<float> mat3;
typedef Matrix3<double> dmat3;
typedef Matrix3<unsigned> umat3;
typedef Matrix3<int> imat3;
typedef Matrix3<bool> bmat3;

typedef Matrix3<float> mat3x3;
typedef Matrix3<double> dmat3x3;
typedef Matrix3<unsigned> umat3x3;
typedef Matrix3<int> imat3x3;
typedef Matrix3<bool> bmat3x3;

template<typename T> constexpr Matrix3<T> transpose(const Matrix3<T>& m) {return Transpose(m);}
template<typename T> Matrix3<T> inverse(const Matrix3<T>& m) {return Inverse(m);}

typedef Matrix3<float> float3x3;
typedef Matrix3<double> double3x3;
typedef Matrix3<unsigned> uint3x3;
typedef Matrix3<int> int3x3;
typedef Matrix3<bool> bool3x3;

//TODO: implement more functions from GLSL and HLSL

}
} INTRA_END
