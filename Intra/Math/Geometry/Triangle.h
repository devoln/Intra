#pragma once

#include "Core/Core.h"
#include "Math/Math.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Math/Matrix4.h"

INTRA_BEGIN
namespace Math {

template<typename T> struct Triangle
{
	constexpr forceinline Triangle(const Vector3<T>& a, const Vector3<T>& b, const Vector3<T>& c):
		A(a), B(b), C(c) {}

	INTRA_MATH_CONSTEXPR Plane<T> GetPlane() const {return Plane<T>(A, B, C);}

	INTRA_MATH_CONSTEXPR2 T Area() const
	{
		const T a = LengthAB();
		const T b = LengthBC();
		const T c = LengthAC();
		const T p = (a + b + c) / 2;

		return Sqrt(p*(p-a)*(p-b)*(p-c));
	}
	
	INTRA_MATH_CONSTEXPR T LengthAB() const noexcept {return Distance(A, B);}
	INTRA_MATH_CONSTEXPR T LengthBC() const noexcept {return Distance(B, C);}
	INTRA_MATH_CONSTEXPR T LengthAC() const noexcept {return Distance(A, C);}

	constexpr Vector3<T> VectorAB() const noexcept {return B - A;}
	constexpr Vector3<T> VectorBC() const noexcept {return C - B;}
	constexpr Vector3<T> VectorAC() const noexcept {return C - A;}

	union
	{
		Vector3<T> Vertices[3];
		struct
		{
			Vector3<T> A, B, C;
		};
	};
};

template<typename T> constexpr Triangle<T> operator*(const Triangle<T>& lhs, const Matrix4<T>& m)
{return {(Vector4<T>(lhs.A, 1)*m).xyz, (Vector4<T>(lhs.B, 1)*m).xyz, (Vector4<T>(lhs.C, 1)*m).xyz};}

template<typename T> constexpr Triangle<T> operator*(const Matrix4<T>& m, const Triangle<T>& rhs)
{return {(m*Vector4<T>(rhs.A, 1)).xyz, (m*Vector4<T>(rhs.B, 1)).xyz, (m*Vector4<T>(rhs.C, 1)).xyz};}

}}

