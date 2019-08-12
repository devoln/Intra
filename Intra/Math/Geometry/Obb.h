#pragma once

#include "Math/Vector3.h"
#include "Math/Matrix3.h"
#include "Math/Matrix4.h"
#include "Aabb.h"

INTRA_BEGIN
namespace Math {

template<typename T> struct Obb
{
	Vector3<T> Center;

	//! Матрица, задающая вращение и масштаб OBB по трём осям.
	//! Угловые точки бокса до применения Transform расположены относительно Center
	//! в позициях (-0.5, -0.5, -0.5), ..., (0.5, 0.5, 0.5).
	Matrix3<T> Transform;

	constexpr Obb(const Vector3<T>& center={0,0,0}, const Vector3<T>& size={0,0,0}) noexcept:
		Center(center), Transform(Matrix3<T>::CreateScaling(size)) {}

	constexpr Obb(const Aabb<T>& aabb) noexcept:
		Center(aabb.Center()), Transform(Matrix3<T>::CreateScaling(aabb.OrientedSize())) {}

	constexpr Obb(const Aabb<T>& aabb, const Matrix3<T>& world) noexcept:
		Center(aabb.Center()), Transform(world * Matrix3<T>::CreateScaling(aabb.OrientedSize())) {}

	constexpr Vector3<T> GetPoint(bool positiveX, bool positiveY, bool positiveZ) const noexcept
	{return (GetFullTransform()*Vector4<T>(T(positiveX)-T(0.5), T(positiveY)-T(0.5), T(positiveZ)-T(0.5), 1)).xyz;}

	constexpr forceinline Matrix4<T> GetFullTransform() const noexcept {return {Transform, Center};}

	void AsRotatedAabb(Aabb<T>* oAabb, Matrix3<T>* oRotation)
	{
		if(oRotation) *oRotation = Transform.ExtractRotation();
		if(oAabb) *oAabb = Aabb<T>::FromCenterAndSize(Center, Transform.ExtractScalingVector());
	}

	constexpr T DiagonalSqr() const noexcept {return LengthSqr(Transform[0]+Transform[1]+Transform[2]);}
	INTRA_MATH_CONSTEXPR T Diagonal() const {return Sqrt(DiagonalSqr());}
};

template<typename T> constexpr Obb<T> operator*(const Matrix4<T>& m, const Obb<T>& obb)
{return {(m*Vector4<T>(obb.Center, 1)).xyz, Matrix3<T>(m)*obb.Transform};}

template<typename T> INTRA_MATH_CONSTEXPR T DistanceSqr(const Obb<T>& obb, const Vector3<T>& pt)
{
	return DistanceSqr(
		Aabb<T>::FromSize(obb.Transform.ExtractScaleVector()),
		(pt - obb.Center) * obb.Transform.ExtractRotation() //Умножение справа, потому что точка поворачивается вокруг aabb в противоположную повороту obb сторону
	);
}

template<typename T> INTRA_MATH_CONSTEXPR T Distance(const Obb<T>& obb, const Vector3<T>& pt) {return T(Sqrt(DistanceSqr(obb, pt)));}

}}
