#pragma once

#include "Intra/Float.h"
#include "Intra/Math/Math.h"
#include "Vector2.h"

#ifdef _MSC_VER
#pragma warning(disable: 4201) //Do not complain about union { struct { ... }; ...};
#endif

namespace Intra { INTRA_BEGIN
template<typename T> struct Vector2;
template<typename T> struct Vector4;

template<typename T> struct Vector3
{
	Vector3() = default;
	constexpr explicit Vector3(T XYZ): x(XYZ), y(XYZ), z(XYZ) {}
	template<typename U, typename V, typename S> constexpr  INTRA_FORCEINLINE Vector3(U X, V Y, S Z): x(T(X)), y(T(Y)), z(T(Z)) {}
	template<typename U> explicit constexpr Vector3(const Vector2<U>& v, T Z=0): x(T(v.x)), y(T(v.y)), z(Z) {}
	template<typename U> constexpr Vector3(const Vector3<U>& v): x(T(v.x)), y(T(v.y)), z(T(v.z)) {}
	template<typename U> constexpr explicit Vector3(const Vector4<U>& v): x(T(v.x)), y(T(v.y)), z(T(v.z)) {}

	template<typename U> constexpr Vector3 operator+(const Vector3<U>& rhs) const
	{return {x+rhs.x, y+rhs.y, z+rhs.z};}

	template<typename U> INTRA_FORCEINLINE Vector3& operator+=(const Vector3<U>& rhs)
	{x += rhs.x, y += rhs.y, z += rhs.z; return *this;}

	constexpr Vector3 plus(const T& rhs) const
	{return {x+rhs, y+rhs, z+rhs};}

	constexpr Vector3 minus(const T& rhs) const
	{return {x-rhs, y-rhs, z-rhs};}

	constexpr Vector3 operator-() const {return {-x, -y, -z};}

	template<typename U> constexpr Vector3 operator-(const Vector3<U>& rhs) const
	{return Vector3(x-rhs.x, y-rhs.y, z-rhs.z);}

	template<typename U> INTRA_FORCEINLINE Vector3& operator-=(const Vector3<U>& rhs)
	{x-=rhs.x, y-=rhs.y, z-=rhs.z; return *this;}

	constexpr Vector3 operator*(T n) const {return {x*n, y*n, z*n};}

	INTRA_FORCEINLINE Vector3& operator*=(T n) noexcept {x*=n, y*=n, z*=n; return *this;}

	template<typename U> constexpr Vector3 operator*(const Vector3<U>& rhs) const
	{return Vector3(x*rhs.x, y*rhs.y, z*rhs.z);}

	template<typename U> INTRA_FORCEINLINE Vector3& operator*=(const Vector3<U>& rhs)
	{x*=rhs.x, y*=rhs.y, z*=rhs.z; return *this;}


	constexpr Vector3 operator/(T n) const {return {x/n, y/n, z/n};}

	INTRA_FORCEINLINE Vector3& operator/=(T n) {x /= n, y /= n, z /= n; return *this;}

	template<typename U> constexpr Vector3 operator/(const Vector3<U>& rhs) const {return {x/rhs.x, y/rhs.y, z/rhs.z};}

	template<typename U> INTRA_FORCEINLINE Vector3& operator/=(const Vector3<U>& rhs)
	{x /= rhs.x, y /= rhs.y, z /= rhs.z; return *this;}


	template<unsigned A, unsigned B> constexpr Vector2<T> swizzle() const noexcept
	{
		static_assert(A<3 && B<3, "Invalid swizzle arguments!");
		return {(&x)[A], (&x)[B]};
	}

	template<unsigned A, unsigned B, unsigned C> constexpr Vector3 swizzle() const noexcept
	{
		static_assert(A<3 && B<3 && C<3, "Invalid swizzle arguments!");
		return {(&x)[A], (&x)[B], (&x)[C]};
	}

	template<unsigned A, unsigned B, unsigned C, unsigned D> constexpr Vector4<T> swizzle() const noexcept
	{
		static_assert(A<3 && B<3 && C<3 && D<3, "Invalid swizzle arguments!");
		return {(&x)[A], (&x)[B], (&x)[C], (&x)[D]};
	}


	constexpr bool operator==(const Vector3& rhs) const noexcept
	{return (x==rhs.x && y==rhs.y && z==rhs.z);}

	constexpr bool operator!=(const Vector3& rhs) const noexcept {return !operator==(rhs);}

	INTRA_FORCEINLINE bool operator==(TNaN) const noexcept {return x + y + z == NaN;}
	INTRA_FORCEINLINE bool operator!=(TNaN) const noexcept {return !operator==(NaN);}
	friend INTRA_FORCEINLINE bool operator==(TNaN, const Vector3& rhs) noexcept {return rhs == NaN;}
	friend INTRA_FORCEINLINE bool operator!=(TNaN, const Vector3& rhs) noexcept {return rhs != NaN;}

	constexpr Vector3 operator<<(unsigned rhs) const
	{return {x << rhs, y << rhs, z << rhs};}

	constexpr Vector3 operator>>(unsigned rhs) const
	{return {x >> rhs, y >> rhs, z >> rhs};}

	constexpr Vector3 operator&(T rhs) const noexcept
	{return {(x & rhs), (y & rhs), (z & rhs)};}

	constexpr Vector3 operator|(T rhs) const noexcept
	{return {(x | rhs), (y | rhs), (z | rhs)};}

	constexpr Vector3 operator^(T rhs) const noexcept
	{return {(x ^ rhs), (y ^ rhs), (z ^ rhs)};}


	constexpr Vector3 operator&(const Vector3& rhs) const noexcept
	{return {(x & rhs.x), (y & rhs.y), (z & rhs.z)};}

	constexpr Vector3 operator|(const Vector3& rhs) const noexcept
	{return {(x | rhs.x), (y | rhs.y), (z | rhs.y)};}

	constexpr Vector3 operator^(const Vector3& rhs) const noexcept
	{return {(x ^ rhs.x), (y ^ rhs.y), (z ^ rhs.z)};}

	constexpr T* Data() noexcept {return &x;}
	constexpr const T* Data() const noexcept {return &x;}
	constexpr index_t Length() const noexcept {return 3;}

	INTRA_FORCEINLINE T& operator[](size_t index) {return (&x)[index];}
	constexpr T operator[](size_t index) const {return (&x)[index];}

	constexpr T MaxElement() const {return Max(xy.MaxElement(), z);}
	constexpr T MinElement() const {return Min(xy.MinElement(), z);}

	union
	{
INTRA_WARNING_PUSH
INTRA_IGNORE_WARN("pedantic")
		struct
		{
			T x;
			union
			{
				struct {T y, z;};
				Vector2<T> yz;
			};
		};
INTRA_WARNING_POP
		Vector2<T> xy;
	};
};

template<typename T> Vector3<T> ClosestPointOnLine(
	const Vector3<T>& pt, const Vector3<T>& lineA, const Vector3<T>& lineB)
{
	const Vector3<T> AB = Normalize(lineB - lineA);
	const T t = Dot(AB, (pt - lineA));
	if(t <= 0) return lineA;
	if(t*t >= DistanceSqr(lineA, lineB)) return lineB;
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

template<typename T> constexpr Vector3<T> operator*(T n, const Vector3<T>& v) {return v*n;}

template<typename T> constexpr T Dot(const Vector3<T>& l, const Vector3<T>& r) {return l.x*r.x + l.y*r.y + l.z*r.z;}

template<typename T> constexpr T LengthSqr(const Vector3<T>& v) {return Dot(v, v);}
template<typename T> T Length(const Vector3<T>& v) {return Sqrt(Dot(v, v));}

template<typename T> T Distance(const Vector3<T>& l, const Vector3<T>& r) {return Length(l - r);}

template<typename T> constexpr T DistanceSqr(const Vector3<T>& l, const Vector3<T>& r) noexcept {return LengthSqr(l-r);}



template<typename T> Vector3<T> Normalize(const Vector3<T>& v) {return v/Length(v);}

template<typename T> constexpr Vector3<T> Reflect(const Vector3<T>& incident, const Vector3<T>& normal) {return incident - 2*Dot(incident, normal)*normal;}

template<typename T> Vector3<T> Refract(const Vector3<T>& I, const Vector3<T>& N, float eta)
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

template<typename T> Vector3<T> Floor(const Vector3<T>& v)
{return {Floor(v.x), Floor(v.y), Floor(v.z)};}

template<typename T> Vector3<T> Ceil(const Vector3<T>& v)
{return {Ceil(v.x), Ceil(v.y), Ceil(v.z)};}

template<typename T> Vector3<T> Fract(const Vector3<T>& v)
{return {Fract(v.x), Fract(v.y), Fract(v.z)};}


template<typename T> Vector3<T> Round(const Vector3<T>& val)
{return {Round(val.x), Round(val.y), Round(val.z)};}


template<typename T> Vector3<T> Exp(const Vector3<T>& v)
{return {T(Exp(v.x)), T(Exp(v.y)), T(Exp(v.z))};}

template<typename T> constexpr Vector3<T> Step(const Vector3<T>& edge, const Vector3<T>& value)
{
	return {
		T(value.x >= edge.x),
		T(value.y >= edge.y),
		T(value.z >= edge.z)
	};
}

template<typename T> constexpr Vector3<T> SmoothStep(T edge0, T edge1, const Vector3<T>& value)
{
	const Vector2<T> t = Clamp((value-Vector3<T>(edge0)) / (edge1-edge0), 0, 1);
	return t*t * T(3 - t*2);
}



template<typename T> constexpr Vector3<T> Sign(const Vector3<T>& v)
{return {Sign(v.x), Sign(v.y), Sign(v.z)};}


template<typename T> constexpr Vector3<T> Abs(const Vector3<T>& v)
{return {Abs(v.x), Abs(v.y), Abs(v.z)};}

using Vec3 = Vector3<float>;
using DVec3 = Vector3<double>;
using IVec3 = Vector3<int32>;
using UVec3 = Vector3<uint32>;
using U8Vec3 = Vector3<byte>;
using I8Vec3 = Vector3<int8>;
using U16Vec3 = Vector3<uint16>;
using I16Vec3 = Vector3<short>;
using BVec3 = Vector3<bool>;
} INTRA_END
