#pragma once

#include "Math/Matrix4.h"
#include "Plane.h"
#include "Sphere.h"
#include "Aabb.h"
#include "Obb.h"

INTRA_BEGIN
namespace Math {

template<typename T> struct Frustum
{
	Frustum() = default;
	
	INTRA_MATH_CONSTEXPR Frustum(const Matrix4<T>& vp): mPlanes {
		Normalize(Plane<T>(vp[0][3]-vp[0][0], vp[1][3]-vp[1][0], vp[2][3]-vp[2][0], vp[3][3]-vp[3][0])), //Right
		Normalize(Plane<T>(vp[0][3]+vp[0][0], vp[1][3]+vp[1][0], vp[2][3]+vp[2][0], vp[3][3]+vp[3][0]), //Left
		Normalize(Plane<T>(vp[0][3]+vp[0][1], vp[1][3]+vp[1][1], vp[2][3]+vp[2][1], vp[3][3]+vp[3][1]), //Bottom
		Normalize(Plane<T>(vp[0][3]-vp[0][1], vp[1][3]-vp[1][1], vp[2][3]-vp[2][1], vp[3][3]-vp[3][1]), //Top
		Normalize(Plane<T>(vp[0][3]-vp[0][2], vp[1][3]-vp[1][2], vp[2][3]-vp[2][2], vp[3][3]-vp[3][2]), //Far
		Normalize(Plane<T>(vp[0][3]+vp[0][2], vp[1][3]+vp[1][2], vp[2][3]+vp[2][2], vp[3][3]+vp[3][2]) //Near
	} {}

	INTRA_MATH_CONSTEXPR Frustum(const Matrix4<T>& view, const Matrix4<T>& proj): Frustum(proj*view) {}

	constexpr forceinline const Plane<T>& Right() const {return mPlanes[0];}
	constexpr forceinline const Plane<T>& Left() const {return mPlanes[1];}
	constexpr forceinline const Plane<T>& Bottom() const {return mPlanes[2];}
	constexpr forceinline const Plane<T>& Top() const {return mPlanes[3];}
	constexpr forceinline const Plane<T>& Far() const {return mPlanes[4];}
	constexpr forceinline const Plane<T>& Near() const {return mPlanes[5];}

	constexpr forceinline const Plane<T>& operator[](size_t index) const {return mPlanes[index];}

	const Plane<T>(&Planes() const)[6] {return mPlanes;}



private:
	Plane<T> mPlanes[6];
};

}}
