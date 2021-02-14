#pragma once

#include <Intra/Math.h>

namespace Intra { INTRA_BEGIN
#ifdef _MSC_VER
#pragma warning(disable: 4201) //Do not complain about union { struct { ... }; ...};
#endif
template<typename T> struct Vector3;
template<typename T> struct Vector4;

template<typename T> struct Vector2
{
	Vector2() = default;
	constexpr explicit Vector2(T XY): x(XY), y(XY) {}
	template<typename U, typename V> constexpr  INTRA_FORCEINLINE Vector2(U X, V Y): x(T(X)), y(T(Y)) {}
	template<typename U> constexpr Vector2(const Vector2<U>& v): x(T(v.x)), y(T(v.y)) {}
	template<typename U> constexpr explicit INTRA_FORCEINLINE Vector2(const Vector3<U>& v): x(T(v.x)), y(T(v.y)) {}
	template<typename U> constexpr explicit INTRA_FORCEINLINE Vector2(const Vector4<U>& v): x(T(v.x)), y(T(v.y)) {}

	template<typename U> constexpr  Vector2 operator+(const Vector2<U>& rhs) const {return Vector2(x+rhs.x, y+rhs.y);}
	template<typename U> INTRA_FORCEINLINE Vector2& operator+=(const Vector2<U>& rhs) {x += rhs.x, y += rhs.y; return *this;}

	constexpr Vector2 operator-() const {return {-x, -y};}
	template<typename U> constexpr Vector2 operator-(const Vector2<U>& rhs) const {return Vector2(x-rhs.x, y-rhs.y);}
	template<typename U> INTRA_FORCEINLINE Vector2& operator-=(const Vector2<U>& rhs) {x -= rhs.x, y -= rhs.y; return *this;}

	constexpr Vector2 operator*(T n) const {return {x*n, y*n};}
	INTRA_FORCEINLINE Vector2& operator*=(T n) {x*=n, y*=n; return *this;}

	template<typename U> constexpr Vector2 operator*(const Vector2<U>& rhs) const {return Vector2(x*rhs.x, y*rhs.y);}
	template<typename U> INTRA_FORCEINLINE Vector2& operator*=(const Vector2<U>& rhs) {x *= rhs.x, y *= rhs.y; return *this;}

	constexpr Vector2 operator/(T n) const {return {x/n, y/n};}
	INTRA_FORCEINLINE Vector2& operator/=(T n) {x /= n, y /= n; return *this;}
	template<typename U> constexpr Vector2 operator/(const Vector2<U>& rhs) const {return Vector2(x/rhs.x, y/rhs.y);}
	template<typename U> INTRA_FORCEINLINE Vector2& operator/=(const Vector2<U>& rhs) {x /= rhs.x, y /= rhs.y; return *this;}

	template<unsigned A, unsigned B> INTRA_FORCEINLINE Vector2 swizzle() const noexcept
	{
		static_assert(A<2 && B<2, "Invalid swizzle arguments!");
		return {(&x)[A], (&x)[B]};
	}

	template<unsigned A, unsigned B, unsigned C> Vector3<T> INTRA_FORCEINLINE swizzle() const
	{
		static_assert(A<2 && B<2 && C<2, "Invalid swizzle arguments!");
		return {(&x)[A], (&x)[B], (&x)[C]};
	}

	template<unsigned A, unsigned B, unsigned C, unsigned D> INTRA_FORCEINLINE Vector4<T> swizzle() const noexcept
	{
		static_assert(A<2 && B<2 && C<2 && D<2, "Invalid swizzle arguments!");
		return {(&x)[A], (&x)[B], (&x)[C], (&x)[D]};
	}

	constexpr bool operator==(const Vector2& rhs) const noexcept {return (x==rhs.x && y==rhs.y);}
	constexpr bool operator!=(const Vector2& rhs) const noexcept {return !operator==(rhs);}
	INTRA_FORCEINLINE bool operator==(TNaN) const noexcept {return x + y == NaN;}
	INTRA_FORCEINLINE bool operator!=(TNaN) const noexcept {return !operator==(NaN);}

	constexpr Vector2 operator<<(unsigned rhs) const {return {x << rhs, y << rhs};}
	constexpr Vector2 operator>>(unsigned rhs) const {return {x >> rhs, y >> rhs};}
	constexpr Vector2 operator&(T rhs) const noexcept {return {(x & rhs), (y & rhs)};}
	constexpr Vector2 operator|(T rhs) const noexcept {return {(x | rhs), (y | rhs)};}
	constexpr Vector2 operator^(T rhs) const noexcept {return {(x ^ rhs), (y ^ rhs)};}

	constexpr Vector2 operator&(const Vector2& rhs) const noexcept {return {(x & rhs.x), (y & rhs.y)};}
	constexpr Vector2 operator|(const Vector2& rhs) const noexcept {return {(x | rhs.x), (y | rhs.y)};}
	constexpr Vector2 operator^(const Vector2& rhs) const noexcept {return {(x ^ rhs.x), (y ^ rhs.y)};}


	Vector2 ClosestPointOnLine(const Vector2& lineA, const Vector2& lineB) const
	{
		const auto AB = Distance(lineB, lineA);
		const auto t = Dot(AB, (*this-lineA));
		if(t <= 0) return lineA;
		if(t >= Distance(lineA, lineB)) return lineB;
		return lineA+AB*t;
	}

	constexpr T MaxElement() const {return x<y? y: x;}
	constexpr T MinElement() const {return x<y? y: x;}

	INTRA_FORCEINLINE T* Data() noexcept {return &x;}
	constexpr const T* Data() const noexcept {return &x;}
	constexpr index_t Length() const noexcept {return 2;}

	INTRA_FORCEINLINE T& operator[](size_t index) {return (&x)[index];}
	INTRA_FORCEINLINE const T& operator[](size_t index) const {return (&x)[index];}

	T x, y;
};

template<typename T> constexpr Vector2<T> Min(const Vector2<T>& v1, T v2)
{return {Min(v1.x, v2), Min(v1.y, v2)};}

template<typename T> constexpr Vector2<T> Min(const Vector2<T>& v1, const Vector2<T>& v2)
{return {Min(v1.x, v2.x), Min(v1.y, v2.y)};}

template<typename T> constexpr Vector2<T> Max(const Vector2<T>& v1, T v2)
{return {Max(v1.x, v2), Max(v1.y, v2)};}

template<typename T> constexpr Vector2<T> Max(const Vector2<T>& v1, const Vector2<T>& v2)
{return {Max(v1.x, v2.x), Max(v1.y, v2.y)};}

template<typename T> constexpr Vector2<T> operator*(T n, const Vector2<T>& v) {return v*n;}


template<typename T> constexpr T Dot(const Vector2<T>& l, const Vector2<T>& r) {return l.x*r.x + l.y*r.y;}

template<typename T> constexpr T LengthSqr(const Vector2<T>& v) {return Dot(v,v);}

template<typename T> T Length(const Vector2<T>& v) {return Sqrt(Dot(v,v));}

template<typename T> T Distance(const Vector2<T>& l, const Vector2<T>& r) {return Length(l-r);}

template<typename T> constexpr T DistanceSqr(const Vector2<T>& l, const Vector2<T>& r) {return LengthSqr(l-r);}



template<typename T> Vector2<T> Normalize(const Vector2<T>& v) {return v/Length(v);}

template<typename T> constexpr Vector2<T> Reflect(const Vector2<T>& incident, const Vector2<T>& normal) {return incident - 2*Dot(incident, normal)*normal;}

template<typename T> Vector2<T> Refract(const Vector2<T>& I, const Vector2<T>& N, float eta)
{
	const T NI = Dot(N,I);
	const T k = 1.0 - eta*eta * (1.0 - NI*NI);
	if(k < 0) return Vector2<T>(0);
	return eta*I - (eta*NI + Sqrt(k))*N;
}

template<typename T> constexpr Vector2<T> FaceForward(const Vector2<T>& N, const Vector2<T>& I, const Vector2<T>& Nref)
{return N*Sign(-Dot(Nref, I));}

template<typename T> Vector2<T> Floor(const Vector2<T>& v)
{return {Floor(v.x), Floor(v.y)};}

template<typename T> Vector2<T> Fract(const Vector2<T>& v)
{return {Fract(v.x), Fract(v.y)};}


template<typename T> Vector2<T> Round(const Vector2<T>& val)
{return {Round(val.x), Round(val.y)};}


template<typename T> Vector2<T> Exp(const Vector2<T>& v)
{return {T(Exp(v.x)), T(Exp(v.y))};}

#if INTRA_DISABLED
template<typename T> T random(const Vector2<T>& seed)
{
	return T(Fract(Sin(Dot(seed, Vector2<T>(T(12.9898), T(78.233))))*T(43758.5453)));
}

//Получить случайный вектор с компонентами не больше x
template<typename T> Vector2<T> random2v(T x, const Vector2<T>& seed)
{
	return Vector2<T>(
		random(seed),
		random(seed*7.3242+Vector2<T>(4.4322, 8.556543))
	)*x;
}

template<typename T> Vector2<T> frandom2v(T x) {return Vector2<T>(frandom(), frandom())*x;}
#endif

template<typename T> constexpr Vector2<T> Step(const Vector2<T>& edge, const Vector2<T>& value)
{return {T(value.x >= edge.x), T(value.y >= edge.y)};}
	
template<typename T> Vector2<T> SmoothStep(T edge0, T edge1, const Vector2<T>& value)
{
	const Vector2<T> t = Clamp((value-Vector2<T>(edge0))/(edge1-edge0), 0, 1);
	return t*t * T(3 - t*2);
}



template<typename T> constexpr Vector2<T> Sign(const Vector2<T>& v)
{return {Sign(v.x), Sign(v.y)};}


template<typename T> Vector2<T> Abs(const Vector2<T>& v)
{return Vector2<T>(Abs(v.x), Abs(v.y));}


using Vec2 = Vector2<float>;
using DVec2 = Vector2<double>;
using IVec2 = Vector2<int32>;
using UVec2 = Vector2<uint32>;
using U8Vec2 = Vector2<byte>;
using I8Vec2 = Vector2<int8>;
using U16Vec2 = Vector2<uint16>;
using I16Vec2 = Vector2<int16>;
using BVec2 = Vector2<bool>;
} INTRA_END
