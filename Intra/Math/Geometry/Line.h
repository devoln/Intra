#pragma once

#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Math.h"

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

}}
