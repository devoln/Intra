#pragma once

#include "Cpp/Features.h"
#include "Cpp/Warnings.h"
#include "Cpp/Fundamental.h"
#include "Cpp/InfNan.h"
#include "Math/Math.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#ifdef _MSC_VER
#pragma warning(disable: 4201) //Не ругаться на использование расширения компилятора: union { struct { ... }; ...};
#endif

namespace Intra { namespace Math {

template<typename T> struct Vector3
{
	Vector3() = default;
	constexpr forceinline explicit Vector3(T XYZ): x(XYZ), y(XYZ), z(XYZ) {}
	template<typename U, typename V, typename S> constexpr  forceinline Vector3(U X, V Y, S Z): x(T(X)), y(T(Y)), z(T(Z)) {}
	template<typename U> explicit constexpr forceinline Vector3(const Vector2<U>& v, T Z=0): x(T(v.x)), y(T(v.y)), z(Z) {}
	template<typename U> constexpr forceinline Vector3(const Vector3<U>& v): x(T(v.x)), y(T(v.y)), z(T(v.z)) {}
	template<typename U> constexpr forceinline explicit Vector3(Vector4<U> v): x(T(v.x)), y(T(v.y)), z(T(v.z)) {}

	template<typename U> constexpr forceinline Vector3 operator+(const Vector3<U>& rhs) const
	{return {x+rhs.x, y+rhs.y, z+rhs.z};}

	template<typename U> forceinline Vector3& operator+=(const Vector3<U>& rhs)
	{x += rhs.x, y += rhs.y, z += rhs.z; return *this;}

	constexpr forceinline Vector3 plus(const T& rhs) const
	{return {x+rhs, y+rhs, z+rhs};}

	constexpr forceinline Vector3 minus(const T& rhs) const
	{return {x-rhs, y-rhs, z-rhs};}

	constexpr forceinline Vector3 operator-() const {return {-x, -y, -z};}

	template<typename U> constexpr forceinline Vector3 operator-(const Vector3<U>& rhs) const
	{return Vector3(x-rhs.x, y-rhs.y, z-rhs.z);}

	template<typename U> forceinline Vector3& operator-=(const Vector3<U>& rhs)
	{x-=rhs.x, y-=rhs.y, z-=rhs.z; return *this;}

	constexpr forceinline Vector3 operator*(T n) const {return {x*n, y*n, z*n};}

	forceinline Vector3& operator*=(T n) noexcept {x*=n, y*=n, z*=n; return *this;}

	template<typename U> constexpr forceinline Vector3 operator*(const Vector3<U>& rhs) const
	{return Vector3(x*rhs.x, y*rhs.y, z*rhs.z);}

	template<typename U> forceinline Vector3& operator*=(const Vector3<U>& rhs)
	{x*=rhs.x, y*=rhs.y, z*=rhs.z; return *this;}


	constexpr forceinline Vector3 operator/(T n) const {return {x/n, y/n, z/n};}

	forceinline Vector3& operator/=(T n) {x /= n, y /= n, z /= n; return *this;}

	template<typename U> constexpr forceinline Vector3 operator/(const Vector3<U>& rhs) const {return {x/rhs.x, y/rhs.y, z/rhs.z};}

	template<typename U> forceinline Vector3& operator/=(const Vector3<U>& rhs)
	{x /= rhs.x, y /= rhs.y, z /= rhs.z; return *this;}


	template<uint A, uint B> constexpr forceinline Vector2<T> swizzle() const noexcept
	{
		static_assert(A<3 && B<3, "Invalid swizzle arguments!");
		return {(&x)[A], (&x)[B]};
	}

	template<uint A, uint B, uint C> constexpr forceinline Vector3 swizzle() const noexcept
	{
		static_assert(A<3 && B<3 && C<3, "Invalid swizzle arguments!");
		return {(&x)[A], (&x)[B], (&x)[C]};
	}

	template<uint A, uint B, uint C, uint D> constexpr forceinline Vector4<T> swizzle() const noexcept
	{
		static_assert(A<3 && B<3 && C<3 && D<3, "Invalid swizzle arguments!");
		return {(&x)[A], (&x)[B], (&x)[C], (&x)[D]};
	}


	constexpr forceinline bool operator==(const Vector3& rhs) const noexcept
	{return (x==rhs.x && y==rhs.y && z==rhs.z);}

	constexpr forceinline bool operator!=(const Vector3& rhs) const noexcept {return !operator==(rhs);}

	forceinline bool operator==(Cpp::TNaN) const noexcept {return x + y + z == NaN;}
	forceinline bool operator!=(Cpp::TNaN) const noexcept {return !operator==(NaN);}


	constexpr forceinline Vector3 operator<<(uint rhs) const
	{return {x << rhs, y << rhs, z << rhs};}

	constexpr forceinline Vector3 operator>>(uint rhs) const
	{return {x >> rhs, y >> rhs, z >> rhs};}

	constexpr forceinline Vector3 operator&(T rhs) const noexcept
	{return {(x & rhs), (y & rhs), (z & rhs)};}

	constexpr forceinline Vector3 operator|(T rhs) const noexcept
	{return {(x | rhs), (y | rhs), (z | rhs)};}

	constexpr forceinline Vector3 operator^(T rhs) const noexcept
	{return {(x ^ rhs), (y ^ rhs), (z ^ rhs)};}


	constexpr forceinline Vector3 operator&(const Vector3& rhs) const noexcept
	{return {(x & rhs.x), (y & rhs.y), (z & rhs.z)};}

	constexpr forceinline Vector3 operator|(const Vector3& rhs) const noexcept
	{return {(x | rhs.x), (y | rhs.y), (z | rhs.y)};}

	constexpr forceinline Vector3 operator^(const Vector3& rhs) const noexcept
	{return {(x ^ rhs.x), (y ^ rhs.y), (z ^ rhs.z)};}

	INTRA_EXTENDED_CONSTEXPR forceinline T* Data() noexcept {return &x;}
	constexpr forceinline const T* Data() const noexcept {return &x;}
	constexpr forceinline size_t Length() const noexcept {return 3;}

	constexpr forceinline T& operator[](size_t index) {return (&x)[index];}
	constexpr forceinline T operator[](size_t index) const {return (&x)[index];}

	constexpr forceinline T MaxElement() const {return Max(xy.MaxElement(), z);}
	constexpr forceinline T MinElement() const {return Min(xy.MinElement(), z);}

	union
	{
		struct
		{
			T x;
			union
			{
				struct {T y, z;};
				Vector2<T> yz;
			};
		};
		Vector2<T> xy;
	};
};

template<typename T> forceinline bool operator==(Cpp::TNaN, const Vector3<T>& rhs) noexcept {return rhs==NaN;}
template<typename T> forceinline bool operator!=(Cpp::TNaN, const Vector3<T>& rhs) noexcept {return rhs!=NaN;}

template<typename T> INTRA_MATH_EXTENDED_CONSTEXPR Vector3<T> ClosestPointOnLine(const Vector3<T>& lineA, const Vector3<T>& lineB) const
{
	const Vector3<T> AB = Normalize(lineB-lineA);
	const T t = Dot(AB, (*this-lineA));
	if(t <= 0) return lineA;
	if(t >= Distance(lineA, lineB)) return lineB;
	return lineA + AB*t;
}

template<typename T> constexpr Vector3<T> Min(const Vector3<T>& v1, T v2)
{return {Min(v1.x, v2), Min(v1.y, v2), Min(v1.z, v2)};}

template<typename T> constexpr Vector3<T> Min(const Vector3<T>& v1, const Vector3<T>& v2)
{return {Min(v1.x, v2.x), Min(v1.y, v2.y), Min(v1.z, v2.z)};}

template<typename T> constexpr Vector3<T> Max(const Vector3<T>& v1, T v2)
{return {Max(v1.x, v2), Max(v1.y, v2), Max(v1.z, v2)};}

template<typename T> constexpr Vector3<T> Max(const Vector3<T>& v1, const Vector3<T>& v2)
{return {Max(v1.x, v2.x), Max(v1.y, v2.y), Max(v1.z, v2.z)};}

template<typename T> constexpr forceinline Vector3<T> operator*(T n, const Vector3<T>& v) {return v*n;}

template<typename T> constexpr T Dot(const Vector3<T>& l, const Vector3<T>& r) {return l.x*r.x + l.y*r.y + l.z*r.z;}

template<typename T> constexpr forceinline T LengthSqr(const Vector3<T>& v) {return Dot(v,v);}
template<typename T> INTRA_MATH_CONSTEXPR T Length(const Vector3<T>& v) {return Sqrt(Dot(v,v));}

template<typename T> INTRA_MATH_CONSTEXPR T Distance(const Vector3<T>& l, const Vector3<T>& r) {return Length(l-r);}

template<typename T> constexpr forceinline T DistanceSqr(const Vector3<T>& l, const Vector3<T>& r) noexcept {return LengthSqr(l-r);}



template<typename T> INTRA_MATH_CONSTEXPR Vector3<T> Normalize(const Vector3<T>& v) {return v/Length(v);}

template<typename T> constexpr Vector3<T> Reflect(const Vector3<T>& incident, const Vector3<T>& normal) {return incident - 2*Dot(incident, normal)*normal;}

template<typename T> INTRA_MATH_EXTENDED_CONSTEXPR Vector3<T> Refract(const Vector3<T>& I, const Vector3<T>& N, float eta)
{
	const T NI = Dot(N,I);
	const T k = 1.0 - eta*eta * (1.0 - NI*NI);
	if(k < 0) return Vector3<T>(0);
	return eta*I - (eta*NI + Sqrt(k))*N;
}

template<typename T> constexpr Vector3<T> FaceForward(const Vector3<T>& N, const Vector3<T>& I, const Vector3<T>& Nref)
{return N*Sign(-Dot(Nref, I));}

template<typename T> constexpr Vector3<T> Cross(const Vector3<T>& v1, const Vector3<T>& v2)
{
	return {
		v1.y*v2.z - v1.z*v2.y,
		v1.z*v2.x - v1.x*v2.z,
		v1.x*v2.y - v1.y*v2.x
	};
}

template<typename T> INTRA_MATH_CONSTEXPR Vector3<T> Floor(const Vector3<T>& v)
{return {Floor(v.x), Floor(v.y), Floor(v.z)};}

template<typename T> INTRA_MATH_CONSTEXPR Vector3<T> Ceil(const Vector3<T>& v)
{return {Ceil(v.x), Ceil(v.y), Ceil(v.z)};}

template<typename T> INTRA_MATH_CONSTEXPR Vector3<T> Fract(const Vector3<T>& v)
{return {Fract(v.x), Fract(v.y), Fract(v.z)};}


template<typename T> INTRA_MATH_CONSTEXPR Vector3<T> Round(const Vector3<T>& val)
{return {Round(val.x), Round(val.y), Round(val.z)};}


template<typename T> INTRA_MATH_CONSTEXPR Vector3<T> Exp(const Vector3<T>& v)
{return {T(Exp(v.x)), T(Exp(v.y)), T(Exp(v.z))};}

template<typename T> constexpr Vector3<T> Step(const Vector3<T>& edge, const Vector3<T>& value)
{
	return {
		T(value.x >= edge.x),
		T(value.y >= edge.y),
		T(value.z >= edge.z)
	};
}

template<typename T> INTRA_EXTENDED_CONSTEXPR Vector3<T> SmoothStep(T edge0, T edge1, const Vector3<T>& value)
{
	const Vector2<T> t = Clamp((value-Vector3<T>(edge0)) / (edge1-edge0), 0, 1);
	return t*t * T(3 - t*2);
}



template<typename T> constexpr Vector3<T> Sign(const Vector3<T>& v)
{return {Sign(v.x), Sign(v.y), Sign(v.z)};}


template<typename T> constexpr Vector3<T> Abs(const Vector3<T>& v)
{return {Abs(v.x), Abs(v.y), Abs(v.z)};}


namespace GLSL {
template<typename T> constexpr forceinline T dot(const Vector3<T>& l, const Vector3<T>& r) {return Dot(l, r);}
template<typename T> INTRA_MATH_CONSTEXPR forceinline T length(const Vector3<T>& v) {return Length(v);}
template<typename T> INTRA_MATH_CONSTEXPR forceinline T distance(const Vector3<T>& l, const Vector3<T>& r) {return Distance(l, r);}
template<typename T> INTRA_MATH_CONSTEXPR forceinline Vector3<T> normalize(const Vector3<T>& v) {return Normalize(v);}
template<typename T> constexpr forceinline Vector3<T> reflect(const Vector3<T>& incident, const Vector3<T>& normal) {return Reflect(incident, normal);}
template<typename T> INTRA_MATH_EXTENDED_CONSTEXPR forceinline Vector3<T> refract(const Vector3<T>& I, const Vector3<T>& N, float eta) {return Refract(I, N, eta);}
template<typename T> constexpr forceinline Vector3<T> faceforward(const Vector3<T>& N, const Vector3<T>& I, const Vector3<T>& Nref) {return FaceForward(N, I, Nref);}
template<typename T> constexpr forceinline Vector3<T> cross(const Vector3<T>& v1, const Vector3<T>& v2) {return Cross(v1, v2);}
}

typedef Vector3<float> Vec3;
typedef Vector3<double> DVec3;
typedef Vector3<int> IVec3;
typedef Vector3<uint> UVec3;
typedef Vector3<byte> UBVec3;
typedef Vector3<sbyte> SBVec3;
typedef Vector3<ushort> USVec3;
typedef Vector3<short> SVec3;
typedef Vector3<bool> BVec3;

namespace GLSL {
typedef Vector3<float> vec3;
typedef Vector3<double> dvec3;
typedef Vector3<int> ivec3;
typedef Vector3<uint> uvec3;
typedef Vector3<bool> bvec3;
}

}}

INTRA_WARNING_POP
