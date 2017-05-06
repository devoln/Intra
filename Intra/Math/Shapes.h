#pragma once

#include "Math/Math.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Matrix3.h"
#include "Matrix4.h"

namespace Intra { namespace Math {

template<typename T> struct Line
{
	union
	{
		struct
		{
			union
			{
				Vector2<T> Normal;
				struct {T A, B;};
			};
			T C;
		};
		Vector3<T> AsVec3;
	};

	Line() = default;
	constexpr forceinline Line(T a, T b, T c) noexcept: A(a), B(b), C(c) {}
	constexpr forceinline Line(const Vector2<T>& normal, T c) noexcept: Normal(normal), C(c) {}
	constexpr forceinline Line(const Vector3<T>& asVec3) noexcept: AsVec3(asVec3) {}

	static constexpr Line FromPoints(const Vector2<T>& p1, const Vector2<T>& p2) noexcept
	{
		return Normalize(Line(
			p2.y - p1.y,
			p1.x - p2.x,
			p1.x*(p1.y - p2.y) + p1.y*(p2.x - p1.x)
		));
	}

	//! Проверить, лежит ли точка pt на прямой.
	//! Функция предполагает, что Normal единична.
	constexpr bool Contains(const Vector2<T>& pt, T eps=T(0.0001)) noexcept
	{return Abs(Dot(Normal, pt) + C) <= eps;}
};

template<typename T> constexpr Line<T> Normalize(const Line<T>& line)
{return {line.AsVec3/Length(line.Normal)};}

template<typename T> struct LineSegment
{
	Vector3<T> A, B;

	LineSegment() = default;
	constexpr LineSegment(const Vector3<T>& a, const Vector3<T>& b) noexcept: A(a), B(b) {}
	constexpr forceinline Vector3<T> Midpoint() const noexcept {return (A + B)/2;}
};

template<typename T> constexpr forceinline T LengthSqr(const LineSegment<T>& l) noexcept {return DistanceSqr(l.A, l.B);}
template<typename T> INTRA_MATH_CONSTEXPR forceinline T Length(const LineSegment<T>& l) noexcept {return Distance(l.A, l.B);}








template<typename T> Vector3<T> CheckBoxSphereCollision(const Obb<T>& box, const Sphere<T>& sph)
{
	/*auto boxTransform = box.GetFullTransform();
	const auto boxSizes = box.Transform.ExtractScaleVector();
	boxTransform = (Matrix4<T>::Scaling(Vector3<T>(1) / boxSizes)*boxTransform);
	//if(Distance(box.position, sph.Center) >= sph.Radius+Sqrt(T(3))) return {0,0,0};

	const Vector4<T> localCenter = Inverse(boxTransform) * Vector4<T>(sph.Center, 1);
	auto dist = -Sqr(sph.Radius);
	for(ushort i=0; i<3; i++)
	{
		if(localCenter[i] < -boxSizes[i]/2) dist += Sqr(localCenter[i] + boxSizes[i]/2);
		else if(localCenter[i] > boxSizes[i]/2) dist += Sqr(localCenter[i] - boxSizes[i]/2);
	}
	if(dist >= 0) return {0,0,0};

	Vec3 result = {0,0,0};
	for(ushort i=0; i<3; i++)
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
	for(ushort i=0; i<3; i++)
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
	for(ushort i=0; i<3; i++)
	{
		if(localCenter[i] < -boxSizes[i]/2)
			result[i] = -sph.Radius + d[i], c++;
		else if(localCenter[i]>boxSizes[i]/2)
			result[i] = sph.Radius - d[i], c++;
	}
	if(c>1)
	{
		result = {0,0,0};
		for(ushort i=0; i<3; i++)
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

template<typename T, typename U> forceinline bool TestIntersection(const T& lhs, const U& rhs)
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


}}
