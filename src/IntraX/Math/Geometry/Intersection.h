#pragma once

#include "IntraX/Math/Vector3.h"
#include "Ray.h"
#include "Aabb.h"
#include "Sphere.h"
#include "Frustum.h"
#include "BoundingVolume.h"

namespace Intra { INTRA_BEGIN
namespace Math {

enum class IntersectionType: byte {Outside, Intersect, Inside};

/// Test intersection between ray and sphere.
/// @param ray must be normalized.
/// @return true, if there is an intersection.
template<typename T> constexpr IntersectionType CheckIntersection(const Ray<T>& ray, const Sphere<T>& sphere)
{
	const Vector3<T> l = sphere.Center - ray.Origin;
	const T d = Dot(l, ray.Direction);
	const T squaredLen = LengthSqr(l);
	const T squaredRadius = sphere.Radius*sphere.Radius;
	if(d < 0 && squaredLen > squaredRadius) return IntersectionType::Outside;
	return (squaredLen - d*d <= squaredRadius)? IntersectionType::Intersect: IntersectionType::Outside;
}

/// Test intersection between ray and sphere and find the intersection point.
/// @param ray must be normalized.
/// @return Parameter t for intersection point according to formula ray.origin + t*ray.dir. If there was no intersection, returns NaN for floating point types or 0 for integer
template<typename T> constexpr T GetRayIntersectionPoint(const Sphere<T>& sphere, const Ray<T>& ray)
{
	const Vector3<T> l = sphere.Center - ray.Origin;
	const T d = Dot(l, ray.Direction);
	const T D2 = sphere.Radius*sphere.Radius - LengthSqr(l) + d*d;
	if(D2<0) return NaN;
	const T D = Sqrt(D2);
	const T t0 = d - D;
	if(t0>0) return t0;
	const T t1 = d + D;
	if(t1>0) return t1;
	return NaN;
}


template<typename T> constexpr bool Contains(const Aabb<T>& aabb, const Sphere<T>& sph) noexcept
{return Contains(aabb, BoundingAabb(sph));}


template<typename T> bool CheckInvRayIntersection(const Aabb<T>& aabb,
	const Vector3<T>& rayOrigin, const Vector3<T>& invDirection, T rayLength=T(Infinity), T* oNear=nullptr, T* oFar=nullptr)
{
	const Vector3<T> t1 = (aabb.A - rayOrigin)*invDirection;
	const Vector3<T> t2 = (aabb.B - rayOrigin)*invDirection;

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

template<typename T> bool CheckRayIntersection(const Aabb<T>& aabb,
	const Ray<T>& ray, T rayLength=T(Infinity), T* oNear=nullptr, T* oFar=nullptr)
{return CheckInvRayIntersection({ray.Origin, Vector3<T>(1)/ray.Direction}, rayLength, oNear, oFar);}


template<typename T> bool CheckRayIntersection(const Obb<T>& obb, const Ray<T>& ray)
{return CheckRayIntersection(Aabb<T>(Vector3<T>(-0.5), Vector3<T>(0.5)), Inverse(obb.GetFullTransform())*ray);}


// Test intersection between sph and tri edges.
template<typename T> bool EdgeSphereCollision(const Triangle<T>& tri, const Sphere<T>& sph)
{
	return DistanceSqr(sph.Center, ClosestPointOnLine(sph.Center, tri.A, tri.B)) < Sqr(sph.Radius) ||
		DistanceSqr(sph.Center, ClosestPointOnLine(sph.Center, tri.B, tri.C)) < Sqr(sph.Radius) ||
		DistanceSqr(sph.Center, ClosestPointOnLine(sph.Center, tri.C, tri.A)) < Sqr(sph.Radius);
}

/// Test if point belongs to triangle.
template<typename T> INTRA_MATH_CONSTEXPR bool Contains(const Triangle<T>& tri, const Vector3<T>& v)
{
	const Vector3<T> v0 = Normalize(v - tri.A);
	const Vector3<T> v1 = Normalize(v - tri.B);
	const Vector3<T> v2 = Normalize(v - tri.C);
	return Acos(Dot(v0, v1)) + Acos(Dot(v1, v2)) + Acos(Dot(v2, v0)) >= 2*PI;
}

template<typename T> Vector3<T> CheckCollisionWithSphere(const Triangle<T>& tri, Sphere<T> sphere)
{
	const Plane<T> plane = tri.GetPlane();

	// If the plane doesn't intersect the sphere, then the sphere cannot intersect the triangle
	if(plane.ClassifySphere(sphere) != 0) return {0,0,0};

	const T distance = Dot(sphere.Center, plane.Normal)+plane.D;
	if(Contains(tri, sphere.Center - plane.Normal*distance) ||
		EdgeSphereCollision(tri, Sphere<T>(sphere.Center, sphere.Radius*0.3f)))
		return GetCollisionOffset(plane, sphere.Radius, distance);
	return {0,0,0};
}

/// Test intersection between line defined with two points and triangle.
template<typename T> bool IsIntersectedByLine(const Triangle<T>& tri, Vector3<T> line[2])
{
	const Plane<T> plane = GetPlane();
	return (IsIntersectedByLine(plane, line) &&
		Contains(tri, GetIntersectionWithLine(plane, line)));
}

/// Test ray triangle intersection
// \param[out] oPlaneIntersectionPoint Точка пересечения с треугольником, если такое пересечение есть. Иначе точка пересечения с плоскостью треугольника. Если пересечения с плоскостью нет, то {0,0,0}
template<typename T> bool CheckRayIntersection(const Triangle<T>& tri, const Ray<T>& ray, Vector3<T>* oPlaneIntersectionPoint=nullptr)
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


template<typename T> Vector3<T> CheckBoxSphereCollision(const Obb<T>& box, const Sphere<T>& sph)
{
	/*auto boxTransform = box.GetFullTransform();
	const auto boxSizes = box.Transform.ExtractScaleVector();
	boxTransform = (Matrix4<T>::Scaling(Vector3<T>(1) / boxSizes)*boxTransform);
	//if(Distance(box.position, sph.Center) >= sph.Radius+Sqrt(T(3))) return {0,0,0};

	const Vector4<T> localCenter = Inverse(boxTransform) * Vector4<T>(sph.Center, 1);
	auto dist = -Sqr(sph.Radius);
	for(uint16 i=0; i<3; i++)
	{
		if(localCenter[i] < -boxSizes[i]/2) dist += Sqr(localCenter[i] + boxSizes[i]/2);
		else if(localCenter[i] > boxSizes[i]/2) dist += Sqr(localCenter[i] - boxSizes[i]/2);
	}
	if(dist >= 0) return {0,0,0};

	Vec3 result = {0,0,0};
	for(uint16 i=0; i<3; i++)
	{
		if(localCenter[i] < -boxSizes[i]/2) result[i] = dist;
		else if(localCenter[i] > boxSizes[i]/2) result[i] = -dist;
	}
    return Matrix3<T>(boxTransform)*result;*/
	return CheckBoxSphereCollision(box.Transform.ExtractScaleVector(), box.GetFullTransform(), sph);
}

template<typename T> Vector3<T> CheckBoxSphereCollision(const Vector3<T>& boxSizes, const Matrix4<T>& transform, const Sphere<T>& sph)
{
	const T halfDiagSqr = LengthSqr(boxSizes)/4;
	const T radiusSqr = Sqr(sph.Radius);
	if(DistanceSqr(transform.ExtractTranslationVector(), sph.Center) >= radiusSqr + 2*sph.Radius*Sqrt(halfDiagSqr) + halfDiagSqr)
		return {0,0,0};

	const Vector4<T> localCenter = Inverse(transform)*Vector4<T>(sph.Center, 1);
	T dist = -radiusSqr;
	for(uint16 i=0; i<3; i++)
	{
		if(localCenter[i]<boxSizes[i]/-2) dist += Sqr(localCenter[i] + boxSizes[i]/2);
		else if(localCenter[i]>boxSizes[i]/2) dist += Sqr(localCenter[i] - boxSizes[i]/2);
	}

	if(dist>=0) return {0,0,0};

	Vector3<T> d = Abs(localCenter.xyz)-boxSizes/2;
	auto ld = Length(Max(d, T(0)));
	T dist1 = Clamp(Max(d.y, d.z), d.x, T(0)) + ld;
	dist1 -= sph.Radius;

	Vector3<T> result = {0,0,0};
	int c = 0;
	for(uint16 i=0; i<3; i++)
	{
		if(localCenter[i] < -boxSizes[i]/2)
			result[i] = -sph.Radius + d[i], c++;
		else if(localCenter[i]>boxSizes[i]/2)
			result[i] = sph.Radius - d[i], c++;
	}
	if(c>1)
	{
		result = {0,0,0};
		for(uint16 i=0; i<3; i++)
		{
			if(localCenter[i] < -boxSizes[i]/2)
				result[i] = dist;
			else if(localCenter[i] > boxSizes[i]/2)
				result[i] = -dist;
		}
	}

    return Matrix3<T>(transform)*result;
}

template<typename T> Vector3<T> CheckPlaneSphereCollision(const Vector2<T>& planeSizes,
	const Matrix4<T>& planeTransform, const Sphere<T>& sph)
{
	const Vector3<T> localCenter = Vector3<T>(Inverse(planeTransform)*Vector4<T>(sph.Center, 1));

	static const Vector3<T> points[4] = {
		{-planeSizes.x/2, -planeSizes.y/2, 0},
		{planeSizes.x/2, -planeSizes.y/2, 0},
		{planeSizes.x/2, planeSizes.y/2, 0},
		{-planeSizes.x/2, planeSizes.y/2, 0}
	};
	const Plane<T> planePlane = {points[0], points[1], points[2]};
	const Sphere<T> localSphere = {localCenter, sph.Radius};

    //Если плоскость не пересекает сферу, то сфера не может пересекать треугольник
	if(planePlane.ClassifySphere(localSphere)!=0) return {0,0,0};


    const T distance = Dot(localCenter, planePlane.Normal)+planePlane.D;
    const Vector3<T> pointOnPlane = localSphere.Center-planePlane.Normal*distance;

    /*if(pointOnPlane.x<-planeSizes.x/2)
    	if(relCenter.DistanceTo(relCenter.ClosestPointOnLine(points[3], points[0])) < sphere.Radius) return Vec3(-1, 0, -1);
    	else return {0};
    if(pointOnPlane.x>size.x/2)
        	if(relCenter.DistanceTo(relCenter.ClosestPointOnLine(points[1], points[2])) < sphere.Radius) return Vec3(1, 0, -1);
        	else return {0};
    if(pointOnPlane.y<-size.y/2)
        	if(relCenter.DistanceTo(relCenter.ClosestPointOnLine(points[0], points[1])) < sphere.Radius) return Vec3(0, -1, -1);
        	else return {0};
    if(pointOnPlane.y>size.y/2)
        	if(relCenter.DistanceTo(relCenter.ClosestPointOnLine(points[3], points[2])) < sphere.Radius) return Vec3(0, 1, -1);
        	else return {0};*/

    if(pointOnPlane.x < -planeSizes.x/2 - localSphere.Radius/2 ||
		pointOnPlane.x > planeSizes.x/2 + localSphere.Radius/2 ||
    	pointOnPlane.y < -planeSizes.y/2 - localSphere.Radius/2 ||
		pointOnPlane.y > planeSizes.y/2 + localSphere.Radius/2)
			return {0,0,0};

    const Vector3<T> result = GetCollisionOffset(planePlane, sph.Radius, distance);

    return Matrix3<T>(planeTransform)*result;
}

template<typename T, typename U> INTRA_FORCEINLINE bool TestIntersection(const T& lhs, const U& rhs)
{return TestIntersection(rhs, lhs);}

template<typename T> bool TestIntersection(const Triangle<T>& tri, const Sphere<T>& sph)
{
	const Plane<T> plane = tri.GetPlane();

	//Если плоскость не пересекает сферу, то сфера не может пересекать треугольник
	if(plane.ClassifySphere(sph)!=0) return false;

	const float distance = Dot(sph.Center, plane.Normal)+plane.D;
	return Vec3(sph.Center-plane.Normal*distance).IsInsideTriangle(tri) ||
		tri.EdgeSphereCollision(Sphere<T>(sph.Center, sph.Radius*0.3f));
}

template<typename T> bool TestIntersection(const Triangle<T>& tri1, const Triangle<T>& tri2);
/*{
	(void)tri1; (void)tri2;
	INTRA_DEBUG_ASSERT(!"Not implemented!");
	return false;
}*/

template<typename T> bool TestIntersection(const Aabb<T>& aabb, const Sphere<T>& sph)
{return DistanceSqr(aabb, sph.Center) <= Sqr(sph.Radius);}


template<typename T> constexpr CollisionSide CheckIntersection(const Frustum<T>& frust, const Sphere<T>& refSphere) noexcept
{
	for(auto& p: frust.Planes())
	{
		const T dist = Dot(p.Normal, refSphere.Center) + p.D;
		if(dist < -refSphere.Radius) return CollisionSide::Outside;
		if(Abs(dist) < refSphere.Radius) return CollisionSide::Intersect;
	}
	return CollisionSide::Inside;
}

template<typename T> constexpr bool Contains(const Frustum<T>& frust, const Vector3<T>& point) noexcept
{
	return Dot(point, frust.Right().Normal) + frust.Right().D > 0 &&
		Dot(point, frust.Left().Normal) + frust.Left().D > 0 &&
		Dot(point, frust.Bottom().Normal) + frust.Bottom().D > 0 &&
		Dot(point, frust.Top().Normal) + frust.Top().D > 0 &&
		Dot(point, frust.Far().Normal) + frust.Far().D > 0 &&
		Dot(point, frust.Near().Normal) + frust.Near().D > 0;
}

template<typename T> constexpr bool Contains(const Frustum<T>& frust, const Sphere<T>& sphere) noexcept
{
	return Dot(sphere.Center, frust.Right().Normal) + frust.Right().D + sphere.Radius > 0 &&
		Dot(sphere.Center, frust.Left().Normal) + frust.Left().D + sphere.Radius > 0 &&
		Dot(sphere.Center, frust.Bottom().Normal) + frust.Bottom().D + sphere.Radius > 0 &&
		Dot(sphere.Center, frust.Top().Normal) + frust.Top().D + sphere.Radius > 0 &&
		Dot(sphere.Center, frust.Far().Normal) + frust.Far().D + sphere.Radius > 0 &&
		Dot(sphere.Center, frust.Near().Normal) + frust.Near().D + sphere.Radius > 0;
}

template<typename T> constexpr bool Contains(const Frustum<T>& frust, const Aabb<T>& box) noexcept
{
	for(auto& p: frust.Planes())
	{
		const float d = -p.D;
		const auto& n = p.Normal;
		if(Dot(n, box.A) >= d) continue;
		if(n.x*box.B.x + n.y*box.A.y + n.z*box.A.z >= d) continue;
		if(n.x*box.A.x + n.y*box.B.y + n.z*box.A.z >= d) continue;
		if(n.x*box.B.x + n.y*box.B.y + n.z*box.A.z >= d) continue;
		if(n.x*box.A.x + n.y*box.A.y + n.z*box.B.z >= d) continue;
		if(n.x*box.B.x + n.y*box.A.y + n.z*box.B.z >= d) continue;
		if(n.x*box.A.x + n.y*box.B.y + n.z*box.B.z >= d) continue;
		if(Dot(n, box.B) >= d) continue;
		return false;
	}
	return true;
}

template<typename T> bool Contains(const Frustum<T>& frust, const Obb<T>& box) noexcept
{
	const auto localAabb = Aabb<T>({-0.5, -0.5, -0.5}, {0.5, 0.5, 0.5});
	const auto boxTransform = box.GetFullTransform();
	Frustum<T> localFrust;
	for(uint16 i = 0; i<6; i++)
		localFrust.Planes[i] = Normalize(mPlanes[i].AsVec4 * boxTransform);
	return Contains(localFrust, localAabb);
}

}}
