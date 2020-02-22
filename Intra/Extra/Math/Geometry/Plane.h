#pragma once

#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Sphere.h"

INTRA_BEGIN
namespace Math {

template<typename T> struct Plane
{
	Plane() = default;

	INTRA_MATH_CONSTEXPR Plane(const Vector3<T>& point1, const Vector3<T>& point2, const Vector3<T>& point3) noexcept:
		Normal( Normalize(Cross(point2-point1, point3-point1)) ),
		D( -Dot(point1, Normal) ) {}

	constexpr Plane(const Vector3<T>& planeNormal, const Vector3<T>& pointOnPlane) noexcept:
		Normal(planeNormal), D(-Dot(pointOnPlane, Normal)) {}

	constexpr forceinline Plane(const Vector3<T>& planeNormal, const T& distanceFromZero) noexcept:
		Normal(planeNormal), D(distanceFromZero) {}

	explicit constexpr forceinline Plane(const Vector4<T>& asVec4) noexcept: AsVec4(asVec4) {}

	constexpr forceinline Plane(const T& a, const T& b, const T& c, const T& d) noexcept: AsVec4(a, b, c, d) {}

	union
	{
		struct
		{
			Vector3<T> Normal;
			T D;
		};
		struct
		{
			T A, B, C;
		};
		Vector4<T> AsVec4;
	};
};

typedef Plane<float> PlaneF;

template<typename T> INTRA_MATH_CONSTEXPR Plane<T> Normalize(const Plane<T>& p)
{return Plane<T>(p.AsVec4 / Length(p.Normal);}

//Найти смещение центра сферы при столкновении с плоскостью
template<typename T> constexpr Vector3<T> GetCollisionOffset(const Plane<T>& plane, T Radius, T distanceOfSphereCenter)
{return plane.Normal*(Sign(distanceOfSphereCenter)*Radius - distanceOfSphereCenter);}

template<typename T> INTRA_MATH_CONSTEXPR Vector3<T> GetIntersectionPointNormalized(const Plane<T>& plane, const Vector3<T>& lineA, const Vector3<T>& lineB)
{
	const Vector3<T> dir = Normalize(lineB - lineA);
	const T denominator = Dot(plane.Normal, dir); //Косинус угла между прямой и плоскостью
	if(Abs(denominator) <= T(0.00001)) return lineA;  //Прямая лежит на плоскости
	return lineA + dir*((Distance(lineA, plane) + plane.D)/denominator);
}

template<typename T> INTRA_MATH_CONSTEXPR Vector3<T> GetIntersectionPoint(const Plane<T>& plane, const Vector3<T>& lineA, const Vector3<T>& lineB)
{return GetIntersectionPointNormalized(Normalize(plane), lineA, lineB);}

//! Определить, пересекается ли прямая с плоскостью
template<typename T> constexpr bool CheckIntersection(const Plane<T>& plane, Vector3<T> lineA, Vector3<T> lineB)
{return (Dot(plane.Normal, lineA) < -plane.D) == (Dot(plane.Normal, lineB) < -plane.D);}


template<typename T> constexpr forceinline SignedDistanceNormalized(const Plane<T>& plane, const Vector3<T>& pt)
{return -Dot(pt, plane.Normal);}

template<typename T> INTRA_MATH_CONSTEXPR forceinline SignedDistance(const Plane<T>& plane, const Vector3<T>& pt)
{return DistanceNormalized(pt, plane)/Length(plane.Normal);}

template<typename T> INTRA_MATH_CONSTEXPR forceinline Distance(const Plane<T>& plane, const Vector3<T>& pt)
{return Abs(SignedDistance(plane, pt));}

}}
