#pragma once

#include "Math/Vector3.h"
#include "Ray.h"
#include "Aabb.h"
#include "Sphere.h"
#include "BoundingVolume.h"

namespace Intra { namespace Math {

enum class CollisionSide: byte {Outside, Inside, Intersect};

//! Проверка пересечения луча со сферой
//! @param ray нормированный луч
//! @return true, если произошло пересечение.
template<typename T> bool CheckIntersection(const Ray<T>& ray, const Sphere<T>& sphere) const
{
	const Vector3<T> l = sphere.Center - ray.Origin;
	const T d = Dot(l, ray.Direction);
	const T squaredLen = LengthSqr(l);
	const T squaredRadius = sphere.Radius*sphere.Radius;
	if(d < 0 && squaredLen > squaredRadius) return false;
	return squaredLen - d*d <= squaredRadius;
}

//! Проверка пересечения луча со сферой с нахождением точки пересечения
//! @param ray Нормированный луч.
//! @return Параметр t для нахождения точки пересечения ray.origin + t*ray.dir. Если пересечения не было, то NaN для floating point типов или 0 для integer типов
template<typename T> T GetRayIntersectionPoint(const Sphere<T>& sphere, const Ray<T>& ray) const
{
	Vector3<T> l = sphere.Center-ray.Origin;
	const T d = Dot(l, ray.Direction);
	const T D2 = sphere.Radius*sphere.Radius - LengthSqr(l) + d*d;
	if(D2<0) return NaN;
	const T D = Sqrt(D2);
	const T t0 = d-D;
	if(t0>0) return t0;
	const T t1 = d+D;
	if(t1>0) return t1;
	return NaN;
}

template<typename T> bool CheckRayIntersection(const Obb<T>& obb, const Ray<T>& ray) const
{return Aabb<T>(Vector3<T>(-0.5), Vector3<T>(0.5)).CheckRayIntersection(Inverse(obb.GetFullTransform())*ray);}


template<typename T> constexpr bool Contains(const Aabb<T>& aabb, const Sphere<T>& sph) const noexcept
{return Contains(aabb, BoundingAabb(sph));}


template<typename T> bool CheckInvRayIntersection(const Ray<T>& invray, T rayLength=T(Infinity), T* oNear=null, T* oFar=null) const
{
	const Vector3<T> t1 = (A - invray.Origin)*invray.Direction;
	const Vector3<T> t2 = (B - invray.Origin)*invray.Direction;

	T tmin = Min(t1.x, t2.x);
	T tmax = Max(t1.x, t2.x);
	tmin = Max(tmin, Min(t1.y, t2.y));
	tmax = Min(tmax, Max(t1.y, t2.y));
	tmin = Max(tmin, Min(t1.z, t2.z));
	tmax = Min(tmax, Max(t1.z, t2.z));

	if(oNear) *oNear = tmin;
	if(oFar) *oFar = tmax;

	return tmax >= Max(T(0), tmin) && tmin < rayLength;
}

template<typename T> bool CheckRayIntersection(const Ray<T>& ray, T rayLength=T(Infinity), T* oNear=null, T* oFar=null) const
{return CheckInvRayIntersection({ray.Origin, Vector3<T>(1)/ray.Direction}, rayLength, oNear, oFar);}



//Проверка пересечения сферы с рёбрами треугольника
template<typename T> bool EdgeSphereCollision(const Triangle<T>& tri, const Sphere<T>& sph) const
{
	return DistanceSqr(sph.Center, ClosestPointOnLine(sph.Center, tri.A, tri.B)) < Sqr(sph.Radius) ||
		DistanceSqr(sph.Center, ClosestPointOnLine(sph.Center, tri.B, tri.C)) < Sqr(sph.Radius) ||
		DistanceSqr(sph.Center, ClosestPointOnLine(sph.Center, tri.C, tri.A)) < Sqr(sph.Radius);
}

//! Проверяет, лежит ли точка внутри треугольника.
template<typename T> INTRA_MATH_EXTENDED_CONSTEXPR bool Contains(const Triangle<T>& tri, const Vector3<T>& v)
{
	const Vector3<T> v0 = Normalize(v - tri.A);
	const Vector3<T> v1 = Normalize(v - tri.B);
	const Vector3<T> v2 = Normalize(v - tri.C);
	return Acos(Dot(v0, v1))+Acos(Dot(v1, v2))+Acos(Dot(v2, v0)) >= 2*PI;
}

template<typename T> Vector3<T> CheckCollisionWithSphere(const Triangle<T>& tri, Sphere<T> sphere) const
{
	const Plane<T> plane = tri.GetPlane();

	//Если плоскость не пересекает сферу, то сфера не может пересекать треугольник
	if(plane.ClassifySphere(sphere) != 0) return {0,0,0};

	const T distance = Dot(sphere.Center, plane.Normal)+plane.D;
	if(Contains(tri, sphere.Center - plane.Normal*distance) ||
		EdgeSphereCollision(tri, Sphere<T>(sphere.Center, sphere.Radius*0.3f)))
		return GetCollisionOffset(plane, sphere.Radius, distance);
	return {0,0,0};
}

//! Проверка пересечения прямой с треугольником
template<typename T> bool IsIntersectedByLine(const Triangle<T>& tri, Vector3<T> line[2])
{
	const Plane<T> plane = GetPlane();
	return (IsIntersectedByLine(plane, line) &&
		Contains(tri, GetIntersectionWithLine(plane, line)));
}

//! Проверить треугольник на пересечение с лучом
// \param[out] oPlaneIntersectionPoint Точка пересечения с треугольником, если такое пересечение есть. Иначе точка пересечения с плоскостью треугольника. Если пересечения с плоскостью нет, то {0,0,0}
template<typename T> bool CheckRayIntersection(const Ray<T>& ray, Vector3<T>* oPlaneIntersectionPoint=null)
{
	const Vector3<T> u = Vertices[1]-Vertices[0];
	const Vector3<T> v = Vertices[2]-Vertices[0];
	const Vector3<T> n = Cross(u, v);

	Vector3<T> w0 = ray.Origin - Vertices[0];
	const T a = -Dot(n, w0), b = Dot(n, ray.Direction);

	if(a*b<0 || b==0) //Луч уходит от плоскости треугольника или параллелен ей
	{
		if(oPlaneIntersectionPoint) *oPlaneIntersectionPoint = Vector3<T>(0);
		return false;
	}

	const T r = a/b;
	Vector3<T> planeIntersectionPoint = ray.Origin + ray.Direction*r;
	if(oPlaneIntersectionPoint) *oPlaneIntersectionPoint = planeIntersectionPoint;

	const T uu = Dot(u, u), uv = Dot(u, v), vv = Dot(v, v);
	const Vector3<T> w = planeIntersectionPoint-Vertices[0];
	const T wu = Dot(w, u), wv = Dot(w, v);
	const T D = uv*uv - uu*vv;

	T s = (uv*wv - vv*wu)/D;
	T t = (uv*wu - uu*wv)/D;
	bool pointIsInTriangle = s>=0 && s<=D && t>=0 && s+t<=D;
	return pointIsInTriangle;
}




}}
