#pragma once

#include "Math/Matrix4.h"
#include "Plane.h"
#include "Sphere.h"

namespace Intra { namespace Math {

template<typename T> struct Frustum
{
	struct PlaneIndex
	{
		enum: byte {Right, Left, Bottom, Top, Far, Near};
	};

	Frustum() = default;
	
	INTRA_MATH_CONSTEXPR Frustum(const Matrix4<T>& vp): mPlanes {
		Normalize(Plane<T>(vp[0][3]-vp[0][0], vp[1][3]-vp[1][0], vp[2][3]-vp[2][0], vp[3][3]-vp[3][0])), //Right
		Normalize(Plane<T>(vp[0][3]+vp[0][0], vp[1][3]+vp[1][0], vp[2][3]+vp[2][0], vp[3][3]+vp[3][0]), //Left
		Normalize(Plane<T>(vp[0][3]+vp[0][1], vp[1][3]+vp[1][1], vp[2][3]+vp[2][1], vp[3][3]+vp[3][1]), //Bottom
		Normalize(Plane<T>(vp[0][3]-vp[0][1], vp[1][3]-vp[1][1], vp[2][3]-vp[2][1], vp[3][3]-vp[3][1]), //Top
		Normalize(Plane<T>(vp[0][3]-vp[0][2], vp[1][3]-vp[1][2], vp[2][3]-vp[2][2], vp[3][3]-vp[3][2]), //Far
		Normalize(Plane<T>(vp[0][3]+vp[0][2], vp[1][3]+vp[1][2], vp[2][3]+vp[2][2], vp[3][3]+vp[3][2]) //Near
	} {}

	Frustum(const Matrix4<T>& view, const Matrix4<T>& proj): Frustum(proj*view) {}

	CollisionSide SphereTest(const Sphere<T>& refSphere) const
	{
		for(auto& p: mPlanes)
		{
			const T dist = Dot(p.Normal, refSphere.Center) + p.D;
			if(dist < -refSphere.Radius) return CollisionSide::Outside;
			if(Abs(dist) < refSphere.Radius) return CollisionSide::Intersect;
		}
		return CollisionSide::Inside;
	}

	bool Contains(const Vector3<T>& point) const
	{
		for(auto& p: mPlanes)
			if(Dot(point, p.Normal) + p.D <= 0) return false;
		return true;
	}

    bool Contains(const Sphere<T>& sphere) const
	{
		for(auto& p: mPlanes)
			if(Dot(sphere.Center, p.Normal) + p.D + sphere.Radius <= 0) return false;
		return true;
	}

	bool Contains(const Aabb<T>& box) const
	{
		for(auto& p: Planes)
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

	bool Contains(const Obb<T>& box) const
	{
		const auto localAabb = Aabb<T>({-0.5, -0.5, -0.5}, {0.5, 0.5, 0.5});
		Frustum<T> localFrust;
		auto boxTransform = box.GetFullTransform();
		for(ushort i=0; i<6; i++)
			localFrust.Planes[i] = Normalize(mPlanes[i].AsVec4 * boxTransform);
		return localFrust.Contains(localAabb);
	}

	constexpr forceinline const Plane<T>& operator[](size_t index) const {return mPlanes[index];}

	const Plane<T>(&Planes() const)[6] {return mPlanes;}



private:
	Plane<T> mPlanes[6];
};

}}
