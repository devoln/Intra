#pragma once

INTRA_BEGIN
namespace Math {

template<typename T> struct Ray
{
	Vector3<T> Origin, Direction;

	Ray() = default;
	constexpr forceinline Ray(const Vector3<T>& origin, const Vector3<T>& direction) noexcept:
		Origin(origin), Direction(direction) {}

	template<typename T> constexpr Ray<T> Transform(const Matrix3<T>& mat, const Vector3<T>& offset)
	{return Ray<T>((mat*Vector4<T>(Origin, 1)).xyz, mat*Direction);}

	template<typename T> constexpr Ray<T> Transform(const Matrix4<T>& mat)
	{return Ray<T>((mat*Vector4<T>(Origin, 1)).xyz, Matrix3<T>(mat)*Direction);}

};

typedef Ray<float> RayF;


}}
