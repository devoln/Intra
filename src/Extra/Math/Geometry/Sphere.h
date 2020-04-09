#pragma once

#include "Intra/Math/Math.h"
#include "Extra/Math/Vector3.h"
#include "Extra/Math/Matrix4.h"
#include "Aabb.h"

INTRA_BEGIN
namespace Math {

template<typename T> struct Sphere
{
	Vector3<T> Center;
	T Radius;

	constexpr Sphere(const Vector3<T>& center, const T& radius) noexcept: Center(center), Radius(radius) {}

	void AddPoint(const Vector3<T>& p)
	{
		const T distSqr = DistanceSqr(p, Center);
		if(distSqr > Radius*Radius) Radius = T(Sqrt(distSqr));
	}

	void AddSphere(const Sphere<T>& s)
	{
		const T dist = Distance(s.Center, Center) + s.Radius;
		if(dist > Radius) Radius = dist;
	}
};

template<typename T> INTRA_MATH_CONSTEXPR Sphere<T> operator*(const Matrix4<T>& m, const Sphere<T>& s)
{return {(m*Vector4<T>(s.Center, 1)).xyz, m.ExtractScaleVector().MaxElement()*s.Radius};}

template<typename T> INTRA_MATH_CONSTEXPR T SignedDistanceFromZero(const Sphere<T>& sphere)
{return Length(sphere.Center) - sphere.Radius;}

template<typename T> INTRA_MATH_CONSTEXPR T SignedDistance(const Sphere<T>& sphere, const Vector3<T>& pt)
{return SignedDistanceFromZero({sphere.Center - pt, sphere.Radius});}

template<typename T> INTRA_MATH_CONSTEXPR T Distance(const Sphere<T>& sphere, const Vector3<T>& pt)
{return Max(SignedDistance(sphere.Center, pt), T(0));}

typedef Sphere<float> SphereF;

}}
