#pragma once

namespace Intra { namespace Math {

template<typename T> struct Ray
{
	Vector3<T> Origin, Direction;

	Ray() = default;
	constexpr forceinline Ray(const Vector3<T>& origin, const Vector3<T>& direction) noexcept:
		Origin(origin), Direction(direction) {}
};

typedef Ray<float> RayF;

template<typename T> constexpr Ray<T> operator*(const Matrix4<T>& mat, const Ray<T>& r)
{return Ray<T>(Vector3<T>(mat*Vector4<T>(r.Origin, 1)), Matrix3<T>(mat)*r.Direction);}

}}
