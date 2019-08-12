#pragma once

#include "Core/Core.h"
#include "Math/Math.h"

INTRA_BEGIN
#ifdef _MSC_VER
#pragma warning(disable: 4201) //Не ругаться на использование расширения компилятора: union { struct { ... }; ...};
#endif
inline namespace Math {

template<typename T> struct Vector4
{
public:
	Vector4() = default;

	constexpr forceinline explicit Vector4(T XYZW):
		x(XYZW), y(XYZW), z(XYZW), w(XYZW) {}

	template<typename U, typename V, typename S, typename R> constexpr forceinline
	Vector4(U X, V Y, S Z, R W=1):
		x(T(X)), y(T(Y)), z(T(Z)), w(T(W)) {}

	template<typename U> constexpr forceinline
	explicit Vector4(const Vector2<U>& v, T Z=0, T W=1):
		x(T(v.x)), y(T(v.y)), z(Z), w(W) {}

	template<typename U> constexpr forceinline
	explicit Vector4(const Vector2<U>& v1, const Vector2<U>& v2):
		x(T(v1.x)), y(T(v1.y)), z(T(v2.x)), w(T(v2.y)) {}

	template<typename U> constexpr forceinline
	explicit Vector4(const Vector3<U>& v, T W=1):
		x(T(v.x)), y(T(v.y)), z(T(v.z)), w(W) {}

	template<typename U> constexpr forceinline
	Vector4(const Vector4<U>& v):
		x(T(v.x)), y(T(v.y)), z(T(v.z)), w(T(v.w)) {}

	template<typename U> constexpr forceinline
	Vector4 operator+(const Vector4<U>& rhs) const
	{return Vector4(x+rhs.x, y+rhs.y, z+rhs.z, w+rhs.w);}

	template<typename U> forceinline Vector4& operator+=(const Vector4<U>& rhs)
	{x += rhs.x, y += rhs.y, z += rhs.z, w += rhs.w; return *this;}

	constexpr forceinline Vector4 operator-() const {return {-x, -y, -z, -w};}
	template<typename U> constexpr forceinline
	Vector4 operator-(const Vector4<U>& rhs) const
	{return Vector4(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.z);}

	template<typename U> forceinline Vector4& operator-=(const Vector4<U>& rhs)
	{x -= rhs.x, y -= rhs.y, z -= rhs.z, w -= rhs.w; return *this;}

	constexpr forceinline Vector4 operator*(T n) const {return {x*n, y*n, z*n, w*n};}

	forceinline Vector4& operator*=(T n) {x *= n, y *= n, z *= n, w *= n; return *this;}

	template<typename U> constexpr forceinline
	Vector4 operator*(const Vector4<U>& rhs) const
	{return Vector4(x*rhs.x, y*rhs.y, z*rhs.z, w*rhs.w);}

	template<typename U> forceinline Vector4& operator*=(const Vector4<U>& rhs)
	{x *= rhs.x, y *= rhs.y, z *= rhs.z, w *= rhs.w; return *this;}

	forceinline constexpr Vector4 operator/(T n) const {return {x/n, y/n, z/n, w/n};}
	forceinline Vector4& operator/=(T n) {x /= n, y /= n, z /= n, w /= n; return *this;}

	template<typename U> constexpr forceinline Vector4 operator/(const Vector4<U>& rhs) const
	{return Vector4(x/rhs.x, y/rhs.y, z/rhs.z, w/rhs.w);}

	template<typename U> forceinline Vector4& operator/=(const Vector4<U>& rhs)
	{x/=rhs.x, y/=rhs.y, z/=rhs.z, w/=rhs.w; return *this;}


	template<uint A, uint B> constexpr forceinline Vector2<T> swizzle() const noexcept
	{
		static_assert(A<4 && B<4, "Invalid swizzle arguments!");
		return {(&x)[A], (&x)[B]};
	}

	template<uint A, uint B, uint C> constexpr forceinline Vector3<T> swizzle() const noexcept
	{
		static_assert(A<4 && B<4 && C<4, "Invalid swizzle arguments!");
		return {(&x)[A], (&x)[B], (&x)[C]};
	}

	template<uint A, uint B, uint C, uint D> constexpr forceinline Vector4 swizzle() const noexcept
	{
		static_assert(A<4 && B<4 && C<4 && D<4, "Invalid swizzle arguments!");
		return {(&x)[A], (&x)[B], (&x)[C], (&x)[D]};
	}

	forceinline constexpr bool operator==(const Vector4& rhs) const noexcept
		{return (x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w);}

	forceinline constexpr bool operator!=(const Vector4& rhs) const noexcept {return !operator==(rhs);}

	forceinline bool operator==(TNaN) const noexcept {return x + y + z + w == NaN;}
	forceinline bool operator!=(TNaN) const noexcept {return !operator==(NaN);}


	forceinline constexpr Vector4 operator<<(uint rhs) const
	{return {x << rhs, y << rhs, z << rhs, w << rhs};}

	forceinline constexpr Vector4 operator>>(uint rhs) const
	{return {x >> rhs, y >> rhs, z >> rhs, w >> rhs};}

	forceinline constexpr Vector4 operator&(T rhs) const noexcept
	{return {(x & rhs), (y & rhs), (z & rhs), (w & rhs)};}

	forceinline constexpr Vector4 operator|(T rhs) const noexcept
	{return {(x | rhs), (y | rhs), (z | rhs), (w | rhs)};}

	forceinline constexpr Vector4 operator^(T rhs) const noexcept
	{return {(x ^ rhs), (y ^ rhs), (z ^ rhs), (w ^ rhs)};}

	forceinline constexpr Vector4 operator&(const Vector4& rhs) const noexcept
	{return {(x & rhs.x), (y & rhs.y), (z & rhs.z), (w & rhs.w)};}
	
	forceinline constexpr Vector4 operator|(const Vector4& rhs) const noexcept
	{return {(x | rhs.x), (y | rhs.y), (z | rhs.z), (w | rhs.w)};}
	
	forceinline constexpr Vector4 operator^(const Vector4& rhs) const noexcept
	{return {(x ^ rhs.x), (y ^ rhs.y), (z ^ rhs.z), (w ^ rhs.w)};}

	forceinline T* Data() noexcept {return &x;}
	constexpr forceinline const T* Data() const noexcept {return &x;}
	constexpr forceinline index_t Length() const noexcept {return 4;}

	template<typename Index> forceinline T& operator[](Index index) {return (&x)[index];}
	template<typename Index> constexpr forceinline const T& operator[](Index index) const {return (&x)[index];}

	constexpr forceinline T MaxElement() const {return Max(xy.MaxElement(), zw.MaxElement());}
	constexpr forceinline T MinElement() const {return Min(xy.MinElement(), zw.MinElement());}

	union
	{
INTRA_WARNING_PUSH
INTRA_WARNING_DISABLE_PEDANTIC
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
INTRA_WARNING_POP
		Vector3<T> xyz;
	};
};

template<typename T> forceinline bool operator==(TNaN, const Vector4<T>& rhs) noexcept {return rhs == NaN;}
template<typename T> forceinline bool operator!=(TNaN, const Vector4<T>& rhs) noexcept {return rhs != NaN;}

template<typename T> Vector4<T> ClosestPointOnLine(const Vector4<T> pt, const Vector4<T>& lineA, const Vector4<T>& lineB)
{
	const Vector4<T> AB = Normalize(lineB - lineA);
	const Vector4<T> t = Dot(AB, (pt - lineA));
	if(t <= 0) return lineA;
	if(t*t >= DistanceSqr(lineA, lineB)) return lineB;
	return lineA + AB*t;
}

template<typename T> constexpr Vector4<T> Min(const Vector4<T>& v1, T v2) noexcept
{return {Min(v1.x, v2), Min(v1.y, v2), Min(v1.z, v2), Min(v1.w, v2)};}

template<typename T> constexpr Vector4<T> Min(const Vector4<T>& v1, const Vector4<T>& v2) noexcept
{return {Min(v1.x, v2.x), Min(v1.y, v2.y), Min(v1.z, v2.z), Min(v1.w, v2.w)};}

template<typename T> constexpr Vector4<T> Max(const Vector4<T>& v1, T v2) noexcept
{return {Max(v1.x, v2), Max(v1.y, v2), Max(v1.z, v2), Max(v1.w, v2)};}

template<typename T> constexpr Vector4<T> Max(const Vector4<T>& v1, const Vector4<T>& v2) noexcept
{return {Max(v1.x, v2.x), Max(v1.y, v2.y), Max(v1.z, v2.z), Max(v1.w, v2.w)};}

template<typename T> constexpr forceinline Vector4<T> operator*(T n, const Vector4<T>& v) noexcept {return v*n;}

template<typename T> constexpr T Dot(const Vector4<T>& l, const Vector4<T>& r) noexcept {return l.x*r.x + l.y*r.y+l.z*r.z + l.w*r.w;}

template<typename T> constexpr forceinline T LengthSqr(const Vector4<T>& v) noexcept {return Dot(v, v);}

template<typename T> T Length(const Vector4<T>& v) {return Sqrt(Dot(v, v));}

template<typename T> T Distance(const Vector4<T>& l, const Vector4<T>& r) {return Length(l - r);}

template<typename T> constexpr T DistanceSqr(const Vector4<T>& l, const Vector4<T>& r) noexcept {return LengthSqr(l - r);}



template<typename T> Vector4<T> Normalize(const Vector4<T>& v) {return v / Length(v);}

template<typename T> constexpr Vector4<T> Reflect(const Vector4<T>& incident, const Vector4<T>& normal) noexcept {return incident - Dot(incident, normal)*2*normal;}

template<typename T> Vector4<T> Refract(const Vector4<T>& I, const Vector4<T>& N, float eta)
{
	const T NI = Dot(N, I);
	const T k = T(1) - eta*eta * (T(1) - NI*NI);
	if(k < 0) return Vector4<T>(0);
	return eta*I - (eta*NI + Sqrt(k))*N;
}

template<typename T> constexpr Vector4<T> FaceForward(const Vector4<T>& N, const Vector4<T>& I, const Vector4<T>& Nref) noexcept
{return N*Sign(-Dot(Nref, I));}

template<typename T> Vector4<T> Floor(const Vector4<T>& v) noexcept
{return {Floor(v.x), Floor(v.y), Floor(v.z), Floor(v.w)};}

template<typename T> Vector4<T> Fract(const Vector4<T>& v) noexcept
{return {Fract(v.x), Fract(v.y), Fract(v.z), Fract(v.w)};}

template<typename T> Vector4<T> Round(const Vector4<T>& val) noexcept
{return {Round(val.x), Round(val.y), Round(val.z), Round(val.w)};}

template<typename T> Vector4<T> Exp(const Vector4<T>& v) noexcept
{return {T(Exp(v.x)), T(Exp(v.y)), T(Exp(v.z)), T(Exp(v.w))};}


template<typename T> constexpr Vector4<T> Step(const Vector4<T>& edge, const Vector4<T>& value)
{
	return {
		T(value.x >= edge.x),
		T(value.y >= edge.y),
		T(value.z >= edge.z),
		T(value.w >= edge.w)
	};
}

template<typename T> Vector4<T> SmoothStep(T edge0, T edge1, const Vector4<T>& value)
{
	const auto t = Clamp((value - Vector4<T>(edge0)) / (edge1 - edge0), 0, 1);
	return t*t * T(3 - t*2);
}

template<typename T> constexpr Vector4<T> Sign(const Vector4<T>& v)
{return {Sign(v.x), Sign(v.y), Sign(v.z), Sign(v.w)};}

template<typename T> constexpr Vector4<T> Abs(const Vector4<T>& v)
{return {Abs(v.x), Abs(v.y), Abs(v.z), Abs(v.w)};}

typedef Vector4<float> Vec4;
typedef Vector4<double> DVec4;
typedef Vector4<int> IVec4;
typedef Vector4<uint> UVec4;
typedef Vector4<byte> UBVec4;
typedef Vector4<sbyte> SBVec4;
typedef Vector4<ushort> USVec4;
typedef Vector4<short> SVec4;
typedef Vector4<bool> BVec4;

}
INTRA_END
