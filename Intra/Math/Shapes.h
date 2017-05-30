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


}}
