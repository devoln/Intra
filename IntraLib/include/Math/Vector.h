#pragma once

#include "Core/Core.h"
#include "Math/MathEx.h"

namespace Intra { namespace Math {

template<typename T> struct Matrix2;
template<typename T> struct Matrix3;
template<typename T> struct Matrix4;

template<typename T> struct Vector2
{
	Vector2() = default;
	constexpr explicit Vector2(T XY): x(XY), y(XY) {}
	template<typename U, typename V> constexpr Vector2(U X, V Y): x(T(X)), y(T(Y)) {}
	template<typename U> constexpr Vector2(const Vector2<U>& v): x(T(v.x)), y(T(v.y)) {}
	template<typename U> constexpr explicit Vector2(const Vector3<U>& v): x(T(v.x)), y(T(v.y)) {}
	template<typename U> constexpr explicit Vector2(const Vector4<U>& v): x(T(v.x)), y(T(v.y)) {}

	template<typename U> constexpr Vector2 operator+(const Vector2<U>& rhs) const {return Vector2(x+rhs.x, y+rhs.y);}
	template<typename U> Vector2& operator+=(const Vector2<U>& rhs) {x+=rhs.x, y+=rhs.y; return *this;}

	constexpr Vector2 operator-() const {return {-x, -y};}
	template<typename U> constexpr Vector2 operator-(const Vector2<U>& rhs) const {return Vector2(x-rhs.x, y-rhs.y);}
	template<typename U> Vector2& operator-=(const Vector2<U>& rhs) {x-=rhs.x, y-=rhs.y; return *this;}

	constexpr Vector2 operator*(T n) const {return {x*n, y*n};}
	Vector2& operator*=(T n) {x*=n, y*=n; return *this;}

	template<typename U> constexpr Vector2 operator*(const Vector2<U>& rhs) const {return Vector2(x*rhs.x, y*rhs.y);}
	template<typename U> Vector2& operator*=(const Vector2<U>& rhs) {x*=rhs.x, y*=rhs.y; return *this;}

	Vector2& operator*=(const Matrix3<T>& m) {return *this=*this*m;}

	constexpr Vector2 operator/(T n) const {return {x/n, y/n};}
	Vector2& operator/=(T n) {x/=n, y/=n; return *this;}
	template<typename U> constexpr Vector2 operator/(const Vector2<U>& rhs) const {return Vector2(x/rhs.x, y/rhs.y);}
	template<typename U> Vector2& operator/=(const Vector2<U>& rhs) {x/=rhs.x, y/=rhs.y; return *this;}

	Vector2 swizzle(uint X, uint Y) const {INTRA_ASSERT((X|Y)<2); return {(*this)[X], (*this)[Y]};}
	Vector3<T> swizzle(uint X, uint Y, uint Z) const {INTRA_ASSERT((X|Y|Z)<2); return {(*this)[X], (*this)[Y], (*this)[Z]};}
	Vector4<T> swizzle(uint X, uint Y, uint Z, uint W) const {INTRA_ASSERT((X|Y|Z|W)<2); return {(*this)[X], (*this)[Y], (*this)[Z], (*this)[W]};}

	constexpr bool operator==(const Vector2& rhs) const {return (x==rhs.x && y==rhs.y);}
	constexpr bool operator!=(const Vector2& rhs) const {return !operator==(rhs);}
	bool operator==(NaNType) const {return x+y==NaN;}
	bool operator!=(NaNType) const {return !operator==(NaN);}

	constexpr Vector2 operator<<(uint rhs) const {return {x << rhs, y << rhs};}
	constexpr Vector2 operator>>(uint rhs) const {return {x >> rhs, y >> rhs};}
	constexpr Vector2 operator&(T rhs) const {return {(x & rhs), (y & rhs)};}
	constexpr Vector2 operator|(T rhs) const {return {(x | rhs), (y | rhs)};}
	constexpr Vector2 operator^(T rhs) const {return {(x ^ rhs), (y ^ rhs)};}

	constexpr Vector2 operator&(const Vector2& rhs) const {return {(x & rhs.x), (y & rhs.y)};}
	constexpr Vector2 operator|(const Vector2& rhs) const {return {(x | rhs.x), (y | rhs.y)};}
	constexpr Vector2 operator^(const Vector2& rhs) const {return {(x ^ rhs.x), (y ^ rhs.y)};}


	Vector2 ClosestPointOnLine(const Vector2& lineA, const Vector2& lineB) const
	{
		const auto AB = Distance(lineB, lineA);
		const auto t = Dot(AB, (*this-lineA));
		if(t <= 0) return lineA;
		if(t >= Distance(lineA, lineB)) return lineB;
		return lineA+AB*t;
	}

	operator T*() {return (T*)this;}
	operator const T*() const {return (T*)this;}

	T x, y;
};

template<typename T> bool operator==(NaNType, const Vector2<T>& rhs) {return rhs==NaN;}
template<typename T> bool operator!=(NaNType, const Vector2<T>& rhs) {return rhs!=NaN;}

template<typename T> struct Vector3
{
	Vector3() = default;
	constexpr explicit Vector3(T XYZ): x(XYZ), y(XYZ), z(XYZ) {}
	template<typename U, typename V, typename S> constexpr Vector3(U X, V Y, S Z): x((T)X), y((T)Y), z((T)Z) {}
	template<typename U> explicit constexpr Vector3(const Vector2<U>& v, T Z=0): x((T)v.x), y((T)v.y), z(Z) {}
	template<typename U> constexpr Vector3(const Vector3<U>& v): x((T)v.x), y((T)v.y), z((T)v.z) {}
	template<typename U> constexpr explicit Vector3(Vector4<U> v): x(T(v.x)), y(T(v.y)), z(T(v.z)) {}

	template<typename U> constexpr Vector3 operator+(const Vector3<U>& rhs) const {return Vector3(x+rhs.x, y+rhs.y, z+rhs.z);}
	template<typename U> Vector3& operator+=(const Vector3<U>& rhs) {x+=rhs.x, y+=rhs.y, z+=rhs.z; return *this;}

	constexpr Vector3 operator-() const {return {-x, -y, -z};}
	template<typename U> constexpr Vector3 operator-(const Vector3<U>& rhs) const {return Vector3(x-rhs.x, y-rhs.y, z-rhs.z);}
	template<typename U> Vector3& operator-=(const Vector3<U>& rhs) {x-=rhs.x, y-=rhs.y, z-=rhs.z; return *this;}

	constexpr Vector3 operator*(T n) const {return {x*n, y*n, z*n};}
	Vector3& operator*=(T n) {x*=n, y*=n, z*=n; return *this;}
	template<typename U> constexpr Vector3 operator*(const Vector3<U>& rhs) const {return Vector3(x*rhs.x, y*rhs.y, z*rhs.z);}
	template<typename U> Vector3& operator*=(const Vector3<U>& rhs) {x*=rhs.x, y*=rhs.y, z*=rhs.z; return *this;}

	Vector3 operator*(const Matrix3<T>& m) const;

	Vector3& operator*=(const Matrix3<T>& m) {*this=*this*m; return *this;}

	constexpr Vector3 operator/(T n) const {return {x/n, y/n, z/n};}
	Vector3& operator/=(T n) {x /= n, y /= n, z /= n; return *this;}
	template<typename U> constexpr Vector3 operator/(const Vector3<U>& rhs) const {return {x/rhs.x, y/rhs.y, z/rhs.z};}
	template<typename U> Vector3& operator/=(const Vector3<U>& rhs) {x /= rhs.x, y /= rhs.y, z /= rhs.z; return *this;}

	Vector2<T> swizzle(byte X, byte Y) const {INTRA_ASSERT((X|Y)<3); return {(*this)[X], (*this)[Y]};}
	Vector3 swizzle(byte X, byte Y, byte Z) const {INTRA_ASSERT((X|Y|Z)<3); return {(*this)[X], (*this)[Y], (*this)[Z]};}
	Vector4<T> swizzle(byte X, byte Y, byte Z, byte W) const {INTRA_ASSERT((X|Y|Z|W)<3); return {(*this)[X], (*this)[Y], (*this)[Z], (*this)[W]};}

	constexpr bool operator==(const Vector3& rhs) const {return (x==rhs.x && y==rhs.y && z==rhs.z);}
	constexpr bool operator!=(const Vector3& rhs) const {return !operator==(rhs);}
	bool operator==(NaNType) const {return x+y+z==NaN;}
	bool operator!=(NaNType) const {return !operator==(NaN);}


	constexpr T DistanceToPlane(const Plane<T>& plane) const {return -Dot(*this, plane.normal);}

	//Проверить, лежит ли точка внутри треугольника
	bool IsInsideTriangle(const Triangle<T>& triangle) const
	{
		const Vector3 v0 = Normalize(*this-triangle.vertices[0]);
		const Vector3 v1 = Normalize(*this-triangle.vertices[1]);
		const Vector3 v2 = Normalize(*this-triangle.vertices[2]);
		return Acos(Dot(v0, v1))+Acos(Dot(v1, v2))+Acos(Dot(v2, v0))>=2*PI;
	}

	Vector3 ClosestPointOnLine(const Vector3& lineA, const Vector3& lineB) const
	{
		const Vector3 AB = Normalize(lineB-lineA);
		T t = Dot(AB, (*this-lineA));
		if(t<=0) return lineA;
		if(t >= Distance(lineA, lineB)) return lineB;
		return lineA+AB*t;
	}

	constexpr Vector3 operator<<(uint rhs) const {return {x << rhs, y << rhs, z << rhs};}
	constexpr Vector3 operator>>(uint rhs) const {return {x >> rhs, y >> rhs, z >> rhs};}
	constexpr Vector3 operator&(T rhs) const {return {(x & rhs), (y & rhs), (z & rhs)};}
	constexpr Vector3 operator|(T rhs) const {return {(x | rhs), (y | rhs), (z | rhs)};}
	constexpr Vector3 operator^(T rhs) const {return {(x ^ rhs), (y ^ rhs), (z ^ rhs)};}

	constexpr Vector3 operator&(const Vector3& rhs) const {return {(x & rhs.x), (y & rhs.y), (z & rhs.z)};}
	constexpr Vector3 operator|(const Vector3& rhs) const {return {(x | rhs.x), (y | rhs.y), (z | rhs.y)};}
	constexpr Vector3 operator^(const Vector3& rhs) const {return {(x ^ rhs.x), (y ^ rhs.y), (z ^ rhs.z)};}

	operator T*() {return (T*)this;}
	operator const T*() const {return (T*)this;}

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

template<typename T> bool operator==(NaNType, const Vector3<T>& rhs) {return rhs==NaN;}
template<typename T> bool operator!=(NaNType, const Vector3<T>& rhs) {return rhs!=NaN;}

template<typename T> struct Vector4
{
public:
	Vector4() = default;

	constexpr forceinline explicit Vector4(T XYZW):
		x(XYZW), y(XYZW), z(XYZW), w(XYZW) {}

	template<typename U, typename V, typename S, typename R> forceinline
	Vector4(U X, V Y, S Z, R W=1):
		x((T)X), y((T)Y), z((T)Z), w((T)W) {}

	template<typename U> constexpr forceinline
	explicit Vector4(const Vector2<U>& v, T Z=0, T W=1):
		x((T)v.x), y((T)v.y), z(Z), w(W) {}

	template<typename U> constexpr forceinline
	explicit Vector4(const Vector2<U>& v1, const Vector2<U>& v2):
		x((T)v1.x), y((T)v1.y), z((T)v2.x), w((T)v2.y) {}

	template<typename U> constexpr forceinline
	explicit Vector4(const Vector3<U>& v, T W=1):
		x((T)v.x), y((T)v.y), z((T)v.z), w(W) {}

	template<typename U> constexpr forceinline
	Vector4(const Vector4<U>& v):
		x((T)v.x), y((T)v.y), z((T)v.z), w((T)v.w) {}

	template<typename U> constexpr forceinline
	Vector4 operator+(const Vector4<U>& rhs) const
		{return Vector4(x+rhs.x, y+rhs.y, z+rhs.z, w+rhs.w);}

	template<typename U> forceinline Vector4& operator+=(const Vector4<U>& rhs)
		{x+=rhs.x, y+=rhs.y, z+=rhs.z, w+=rhs.w; return *this;}

	constexpr forceinline Vector4 operator-() const {return {-x, -y, -z, -w};}
	template<typename U> constexpr forceinline
	Vector4 operator-(const Vector4<U>& rhs) const
		{return Vector4(x-rhs.x, y-rhs.y, z-rhs.z, w-rhs.z);}

	template<typename U> forceinline Vector4& operator-=(const Vector4<U>& rhs)
		{x-=rhs.x, y-=rhs.y, z-=rhs.z, w-=rhs.w; return *this;}

	constexpr forceinline Vector4 operator*(T n) const {return {x*n, y*n, z*n, w*n};}

	forceinline Vector4& operator*=(T n) {x*=n, y*=n, z*=n, w*=n; return *this;}

	template<typename U> constexpr forceinline
	Vector4 operator*(const Vector4<U>& rhs) const
		{return Vector4(x*rhs.x, y*rhs.y, z*rhs.z, w*rhs.w);}

	template<typename U> forceinline Vector4& operator*=(const Vector4<U>& rhs)
		{x*=rhs.x, y*=rhs.y, z*=rhs.z, w*=rhs.w; return *this;}

	Vector4 operator*(const Matrix4<T>& m) const;

	forceinline Vector4& operator*=(const Matrix4<T>& m) {return *this=*this*m;}

	forceinline constexpr Vector4 operator/(T n) const {return {x/n, y/n, z/n, w/n};}
	forceinline Vector4& operator/=(T n) {x/=n, y/=n, z/=n, w/=n; return *this;}

	template<typename U> constexpr forceinline Vector4 operator/(const Vector4<U>& rhs) const
		{return Vector4(x/rhs.x, y/rhs.y, z/rhs.z, w/rhs.w);}

	template<typename U> forceinline Vector4& operator/=(const Vector4<U>& rhs)
		{x/=rhs.x, y/=rhs.y, z/=rhs.z, w/=rhs.w; return *this;}

	Vector2<T> swizzle(uint X, uint Y) const {INTRA_ASSERT((X|Y)<4); return {(*this)[X], (*this)[Y]};}
	Vector3<T> swizzle(uint X, uint Y, uint Z) const {INTRA_ASSERT((X|Y|Z)<4); return {(*this)[X], (*this)[Y], (*this)[Z]};}
	Vector4 swizzle(uint X, uint Y, uint Z, uint W) const {INTRA_ASSERT((X|Y|Z|W)<4); return {(*this)[X], (*this)[Y], (*this)[Z], (*this)[W]};}

	constexpr bool operator==(const Vector4& rhs) const {return (x==rhs.x && y==rhs.y && z==rhs.z && w==rhs.w);}
	constexpr bool operator!=(const Vector4& rhs) const {return !operator==(rhs);}
	bool operator==(NaNType) const {return x+y+z+w==NaN;}
	bool operator!=(NaNType) const {return !operator==(NaN);}


	constexpr Vector4 operator<<(uint rhs) const {return {x << rhs, y << rhs, z << rhs, w << rhs};}
	constexpr Vector4 operator>>(uint rhs) const {return {x >> rhs, y >> rhs, z >> rhs, w >> rhs};}
	constexpr Vector4 operator&(T rhs) const {return {(x & rhs), (y & rhs), (z & rhs), (w & rhs)};}
	constexpr Vector4 operator|(T rhs) const {return {(x | rhs), (y | rhs), (z | rhs), (w | rhs)};}
	constexpr Vector4 operator^(T rhs) const {return {(x ^ rhs), (y ^ rhs), (z ^ rhs), (w ^ rhs)};}

	constexpr Vector4 operator&(const Vector4& rhs) const {return {(x & rhs.x), (y & rhs.y), (z & rhs.z), (w & rhs.w)};}
	constexpr Vector4 operator|(const Vector4& rhs) const {return {(x | rhs.x), (y | rhs.y), (z | rhs.z), (w | rhs.w)};}
	constexpr Vector4 operator^(const Vector4& rhs) const {return {(x ^ rhs.x), (y ^ rhs.y), (z ^ rhs.z), (w ^ rhs.w)};}


	Vector4 ClosestPointOnLine(const Vector4& lineA, const Vector4& lineB) const
	{
		const Vector4 AB = Normalize(lineB-lineA);
		const Vector4 t = Dot(AB, (*this-lineA));
		if(t <= 0) return lineA;
		if(t >= Distance(lineA, lineB)) return lineB;
		return lineA+AB*t;
	}

	forceinline operator T*() {return (T*)this;}
	forceinline operator const T*() const {return (T*)this;}

	union
	{
		struct
		{
			union
			{
				struct {T x, y;};
				Vector2<T> xy;
			};
			union
			{
				struct {T z, w;};
				Vector2<T> zw;
			};
		};
		Vector3<T> xyz;
	};
};

#define NOMINMAX
template<typename T> constexpr Vector2<T> Min(const Vector2<T>& v1, T v2) {return {Min(v1.x, v2), Min(v1.y, v2)};}
template<typename T> constexpr Vector3<T> Min(const Vector3<T>& v1, T v2) {return {Min(v1.x, v2), Min(v1.y, v2), Min(v1.z, v2)};}
template<typename T> constexpr Vector4<T> Min(const Vector4<T>& v1, T v2) {return {Min(v1.x, v2), Min(v1.y, v2), Min(v1.z, v2), Min(v1.w, v2)};}
template<typename T> constexpr Vector2<T> Min(const Vector2<T>& v1, const Vector2<T>& v2) {return {Min(v1.x, v2.x), Min(v1.y, v2.y)};}
template<typename T> constexpr Vector3<T> Min(const Vector3<T>& v1, const Vector3<T>& v2) {return {Min(v1.x, v2.x), Min(v1.y, v2.y), Min(v1.z, v2.z)};}
template<typename T> constexpr Vector4<T> Min(const Vector4<T>& v1, const Vector4<T>& v2) {return {Min(v1.x, v2.x), Min(v1.y, v2.y), Min(v1.z, v2.z), Min(v1.w, v2.w)};}
template<typename T> constexpr Vector2<T> Max(const Vector2<T>& v1, T v2) {return {Max(v1.x, v2), Max(v1.y, v2)};}
template<typename T> constexpr Vector3<T> Max(const Vector3<T>& v1, T v2) {return {Max(v1.x, v2), Max(v1.y, v2), Max(v1.z, v2)};}
template<typename T> constexpr Vector4<T> Max(const Vector4<T>& v1, T v2) {return {Max(v1.x, v2), Max(v1.y, v2), Max(v1.z, v2), Max(v1.w, v2)};}
template<typename T> constexpr Vector2<T> Max(const Vector2<T>& v1, const Vector2<T>& v2) {return {Max(v1.x, v2.x), Max(v1.y, v2.y)};}
template<typename T> constexpr Vector3<T> Max(const Vector3<T>& v1, const Vector3<T>& v2) {return {Max(v1.x, v2.x), Max(v1.y, v2.y), Max(v1.z, v2.z)};}
template<typename T> constexpr Vector4<T> Max(const Vector4<T>& v1, const Vector4<T>& v2) {return {Max(v1.x, v2.x), Max(v1.y, v2.y), Max(v1.z, v2.z), Max(v1.w, v2.w)};}

template<typename T> constexpr Vector2<T> operator*(T n, const Vector2<T>& v) {return v*n;}
template<typename T> constexpr Vector3<T> operator*(T n, const Vector3<T>& v) {return v*n;}
template<typename T> constexpr Vector4<T> operator*(T n, const Vector4<T>& v) {return v*n;}

template<typename T> bool operator==(NaNType, const Vector4<T>& rhs) {return rhs==NaN;}
template<typename T> bool operator!=(NaNType, const Vector4<T>& rhs) {return rhs!=NaN;}


template<typename T> constexpr T Dot(const Vector2<T>& l, const Vector2<T>& r) {return l.x*r.x+l.y*r.y;}
template<typename T> constexpr T Dot(const Vector3<T>& l, const Vector3<T>& r) {return l.x*r.x+l.y*r.y+l.z*r.z;}
template<typename T> constexpr T Dot(const Vector4<T>& l, const Vector4<T>& r) {return l.x*r.x+l.y*r.y+l.z*r.z+l.w*r.w;}

template<typename T> constexpr T LengthSqr(const Vector2<T>& v) {return Dot(v,v);}
template<typename T> constexpr T LengthSqr(const Vector3<T>& v) {return Dot(v,v);}
template<typename T> constexpr T LengthSqr(const Vector4<T>& v) {return Dot(v,v);}

template<typename T> T Length(const Vector2<T>& v) {return Sqrt(Dot(v,v));}
template<typename T> T Length(const Vector3<T>& v) {return Sqrt(Dot(v,v));}
template<typename T> T Length(const Vector4<T>& v) {return Sqrt(Dot(v,v));}

template<typename T> T Distance(const Vector2<T>& l, const Vector2<T>& r) {return Length(l-r);}
template<typename T> T Distance(const Vector3<T>& l, const Vector3<T>& r) {return Length(l-r);}
template<typename T> T Distance(const Vector4<T>& l, const Vector4<T>& r) {return Length(l-r);}

template<typename T> constexpr T DistanceSqr(const Vector2<T>& l, const Vector2<T>& r) {return LengthSqr(l-r);}
template<typename T> constexpr T DistanceSqr(const Vector3<T>& l, const Vector3<T>& r) {return LengthSqr(l-r);}
template<typename T> constexpr T DistanceSqr(const Vector4<T>& l, const Vector4<T>& r) {return LengthSqr(l-r);}




template<typename T> Vector2<T> Normalize(const Vector2<T>& v) {return v/Length(v);}
template<typename T> Vector3<T> Normalize(const Vector3<T>& v) {return v/Length(v);}
template<typename T> Vector4<T> Normalize(const Vector4<T>& v) {return v/Length(v);}

template<typename T> constexpr Vector2<T> Reflect(const Vector2<T>& incident, const Vector2<T>& normal) {return incident-2*Dot(incident, normal)*normal;}
template<typename T> constexpr Vector3<T> Reflect(const Vector3<T>& incident, const Vector3<T>& normal) {return incident-2*Dot(incident, normal)*normal;}
template<typename T> constexpr Vector4<T> Reflect(const Vector4<T>& incident, const Vector4<T>& normal) {return incident-2*Dot(incident, normal)*normal;}

template<typename T> Vector2<T> Refract(const Vector2<T>& I, const Vector2<T>& N, float eta)
{
	const float NI=Dot(N,I), k=1.0-eta*eta*(1.0-NI*NI);
	if(k<0) return Vector2<T>(0); else return eta*I-(eta*NI+Sqrt(k))*N;
}

template<typename T> Vector3<T> Refract(const Vector3<T>& I, const Vector3<T>& N, float eta)
{
	const float NI=Dot(N,I), k=1.0-eta*eta*(1.0-NI*NI);
	if(k<0) return Vector3<T>(0); else return eta*I-(eta*NI+Sqrt(k))*N;
}

template<typename T> Vector4<T> Refract(const Vector4<T>& I, const Vector4<T>& N, float eta)
{
	const float NI=Dot(N,I), k=1.0-eta*eta*(1.0-NI*NI);
	if(k<0) return Vector4<T>(0); else return eta*I-(eta*NI+Sqrt(k))*N;
}

template<typename T> constexpr Vector2<T> FaceForward(const Vector2<T>& N, const Vector2<T>& I, const Vector2<T>& Nref) {return N*Sign(-Dot(Nref, I));}
template<typename T> constexpr Vector3<T> FaceForward(const Vector3<T>& N, const Vector3<T>& I, const Vector3<T>& Nref) {return N*Sign(-Dot(Nref, I));}
template<typename T> constexpr Vector4<T> FaceForward(const Vector4<T>& N, const Vector4<T>& I, const Vector4<T>& Nref) {return N*Sign(-Dot(Nref, I));}

template<typename T> constexpr Vector3<T> Cross(const Vector3<T>& v1, const Vector3<T>& v2)
{
	return {v1.y*v2.z-v1.z*v2.y, v1.z*v2.x-v1.x*v2.z, v1.x*v2.y-v1.y*v2.x};
}

template<typename T> Vector2<T> Floor(const Vector2<T>& v) {return {Floor(v.x), Floor(v.y)};}
template<typename T> Vector3<T> Floor(const Vector3<T>& v) {return {Floor(v.x), Floor(v.y), Floor(v.z)};}
template<typename T> Vector4<T> Floor(const Vector4<T>& v) {return {Floor(v.x), Floor(v.y), Floor(v.z), Floor(v.w)};}

template<typename T> Vector2<T> Fract(const Vector2<T>& v) {return {Fract(v.x), Fract(v.y)};}
template<typename T> Vector3<T> Fract(const Vector3<T>& v) {return {Fract(v.x), Fract(v.y), Fract(v.z)};}
template<typename T> Vector4<T> Fract(const Vector4<T>& v) {return {Fract(v.x), Fract(v.y), Fract(v.z), Fract(v.w)};}

template<typename T> Vector2<T> Round(const Vector2<T>& val) {return {Round(val.x), Round(val.y)};}
template<typename T> Vector3<T> Round(const Vector3<T>& val) {return {Round(val.x), Round(val.y), Round(val.z)};}
template<typename T> Vector4<T> Round(const Vector4<T>& val) {return {Round(val.x), Round(val.y), Round(val.z), Round(val.w)};}


template<typename T> Vector2<T> Exp(const Vector2<T>& v) {return {(T)Exp(v.x), (T)Exp(v.y)};}
template<typename T> Vector3<T> Exp(const Vector3<T>& v) {return {(T)Exp(v.x), (T)Exp(v.y), (T)Exp(v.z)};}
template<typename T> Vector4<T> Exp(const Vector4<T>& v) {return {(T)Exp(v.x), (T)Exp(v.y), (T)Exp(v.z), (T)Exp(v.w)};}

#if INTRA_DISABLED
template<typename T> T random(const Vector2<T>& seed)
{
	return (T)(Fract(Sin(Dot(seed, Vector2<T>((T)12.9898, (T)78.233)))*(T)43758.5453));
}

//Получить случайный вектор с компонентами не больше x
template<typename T> Vector2<T> random2v(T x, const Vector2<T>& seed)
{
	return Vector2<T>(
		random(seed),
		random(seed*7.3242+Vector2<T>(4.4322, 8.556543))
	)*x;
}

template<typename T> Vector3<T> random3v(T x, const Vector2<T>& seed)
{
	return Vector3<T>(
		random(seed),
		random(seed*7.3242+Vector2<T>(4.4322, 8.556543)),
		random(seed*3.3242+Vector2<T>(5.44722, 6.526543))
	)*x;
}

template<typename T> Vector4<T> random4v(T x, const Vector2<T>& seed)
{
	return Vector4<T>(
		random(seed),
		random(seed*7.3242+Vector2<T>(4.4322, 8.556543)),
		random(seed*3.3242+Vector2<T>(5.44722, 6.526543)),
		random(seed*9.123442+Vector2<T>(3.44722, 5.526543))
	)*x;
}

template<typename T> Vector2<T> frandom2v(T x) {return Vector2<T>(frandom(), frandom())*x;}
template<typename T> Vector3<T> frandom3v(T x) {return Vector3<T>(frandom(), frandom(), frandom())*x;}
template<typename T> Vector4<T> frandom4v(T x) {return Vector4<T>(frandom(), frandom(), frandom(), frandom())*x;}
#endif

template<typename T> constexpr Vector2<T> Step(const Vector2<T>& edge, const Vector2<T>& value)
    {return {T(value.x>=edge.x), T(value.y>=edge.y)};}

template<typename T> constexpr Vector3<T> Step(const Vector3<T>& edge, const Vector3<T>& value)
    {return {T(value.x>=edge.x), T(value.y>=edge.y), T(value.z>=edge.z)};}

template<typename T> constexpr Vector4<T> Step(const Vector4<T>& edge, const Vector4<T>& value)
    {return {T(value.x>=edge.x), T(value.y>=edge.y), T(value.z>=edge.z), T(value.w>=edge.w)};}
	
template<typename T> Vector2<T> SmoothStep(T edge0, T edge1, const Vector2<T>& value)
	{const Vector2<T> t=Clamp((value-Vector2<T>(edge0))/(edge1-edge0), 0, 1); return t*t*((T)3-t*2);}

template<typename T> Vector3<T> SmoothStep(T edge0, T edge1, const Vector3<T>& value)
	{const Vector2<T> t=Clamp((value-Vector3<T>(edge0))/(edge1-edge0), 0, 1); return t*t*((T)3-t*2);}

template<typename T> Vector4<T> SmoothStep(T edge0, T edge1, const Vector4<T>& value)
	{const Vector2<T> t=Clamp((value-Vector4<T>(edge0))/(edge1-edge0), 0, 1); return t*t*((T)3-t*2);}

template<typename T> constexpr Vector2<T> Sign(const Vector2<T>& v) {return {Sign(v.x), Sign(v.y)};}
template<typename T> constexpr Vector3<T> Sign(const Vector3<T>& v) {return {Sign(v.x), Sign(v.y), Sign(v.z)};}
template<typename T> constexpr Vector4<T> Sign(const Vector4<T>& v) {return {Sign(v.x), Sign(v.y), Sign(v.z), Sign(v.w)};}

template<typename T> Vector2<T> Abs(const Vector2<T>& v) {return Vector2<T>(Abs(v.x), Abs(v.y));}
template<typename T> Vector3<T> Abs(const Vector3<T>& v) {return Vector3<T>(Abs(v.x), Abs(v.y), Abs(v.z));}
template<typename T> Vector4<T> Abs(const Vector4<T>& v) {return Vector4<T>(Abs(v.x), Abs(v.y), Abs(v.z), Abs(v.w));}


namespace GLSL
{
	
	template<typename T> constexpr forceinline T dot(const Vector2<T>& l, const Vector2<T>& r) {return Dot(l, r);}
	template<typename T> constexpr forceinline T dot(const Vector3<T>& l, const Vector3<T>& r) {return Dot(l, r);}
	template<typename T> constexpr forceinline T dot(const Vector4<T>& l, const Vector4<T>& r) {return Dot(l, r);}

	template<typename T> forceinline T length(const Vector2<T>& v) {return Length(v);}
	template<typename T> forceinline T length(const Vector3<T>& v) {return Length(v);}
	template<typename T> forceinline T length(const Vector4<T>& v) {return Length(v);}

	template<typename T> T distance(const Vector2<T>& l, const Vector2<T>& r) {return Distance(l, r);}
	template<typename T> T distance(const Vector3<T>& l, const Vector3<T>& r) {return Distance(l, r);}
	template<typename T> T distance(const Vector4<T>& l, const Vector4<T>& r) {return Distance(l, r);}

	template<typename T> Vector2<T> normalize(const Vector2<T>& v) {return Normalize(v);}
	template<typename T> Vector3<T> normalize(const Vector3<T>& v) {return Normalize(v);}
	template<typename T> Vector4<T> normalize(const Vector4<T>& v) {return Normalize(v);}

	template<typename T> constexpr Vector2<T> reflect(const Vector2<T>& incident, const Vector2<T>& normal) {return Reflect(incident, normal);}
	template<typename T> constexpr Vector3<T> reflect(const Vector3<T>& incident, const Vector3<T>& normal) {return Reflect(incident, normal);}
	template<typename T> constexpr Vector4<T> reflect(const Vector4<T>& incident, const Vector4<T>& normal) {return Reflect(incident, normal);}

	template<typename T> Vector2<T> refract(const Vector2<T>& I, const Vector2<T>& N, float eta) {return Refract(I, N, eta);}
	template<typename T> Vector3<T> refract(const Vector3<T>& I, const Vector3<T>& N, float eta) {return Refract(I, N, eta);}
	template<typename T> Vector4<T> refract(const Vector4<T>& I, const Vector4<T>& N, float eta) {return Refract(I, N, eta);}

	template<typename T> constexpr Vector2<T> faceforward(const Vector2<T>& N, const Vector2<T>& I, const Vector2<T>& Nref) {return FaceForward(N, I, Nref);}
	template<typename T> constexpr Vector3<T> faceforward(const Vector3<T>& N, const Vector3<T>& I, const Vector3<T>& Nref) {return FaceForward(N, I, Nref);}
	template<typename T> constexpr Vector4<T> faceforward(const Vector4<T>& N, const Vector4<T>& I, const Vector4<T>& Nref) {return FaceForward(N, I, Nref);}

	template<typename T> constexpr Vector3<T> cross(const Vector3<T>& v1, const Vector3<T>& v2) {return Cross(v1, v2);}

}


typedef Vector2<Half> HVec2;
typedef Vector3<Half> HVec3;
typedef Vector4<Half> HVec4;

typedef Vector2<float> Vec2;
typedef Vector3<float> Vec3;
typedef Vector4<float> Vec4;

typedef Vector2<double> DVec2;
typedef Vector3<double> DVec3;
typedef Vector4<double> DVec4;

typedef Vector2<int> IVec2;
typedef Vector3<int> IVec3;
typedef Vector4<int> IVec4;

typedef Vector2<uint> UVec2;
typedef Vector3<uint> UVec3;
typedef Vector4<uint> UVec4;

typedef Vector2<byte> UBVec2;
typedef Vector3<byte> UBVec3;
typedef Vector4<byte> UBVec4;

typedef Vector2<sbyte> SBVec2;
typedef Vector3<sbyte> SBVec3;
typedef Vector4<sbyte> SBVec4;

typedef Vector2<ushort> USVec2;
typedef Vector3<ushort> USVec3;
typedef Vector4<ushort> USVec4;

typedef Vector2<short> SVec2;
typedef Vector3<short> SVec3;
typedef Vector4<short> SVec4;

typedef Vector2<bool> BVec2;
typedef Vector3<bool> BVec3;
typedef Vector4<bool> BVec4;


namespace GLSL
{
	typedef Vector2<float> vec2;
	typedef Vector3<float> vec3;
	typedef Vector4<float> vec4;

	typedef Vector2<double> dvec2;
	typedef Vector3<double> dvec3;
	typedef Vector4<double> dvec4;

	typedef Vector2<int> ivec2;
	typedef Vector3<int> ivec3;
	typedef Vector4<int> ivec4;

	typedef Vector2<uint> uvec2;
	typedef Vector3<uint> uvec3;
	typedef Vector4<uint> uvec4;

	typedef Vector2<bool> bvec2;
	typedef Vector3<bool> bvec3;
	typedef Vector4<bool> bvec4;
}




}}
