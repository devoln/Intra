#pragma once

#include "Intra/Core.h"
#include "Intra/Math/Math.h"
#include "IntraX/Math/Vector3.h"

INTRA_BEGIN
namespace Math {

template<typename T> struct LineSegment
{
	Vector3<T> A, B;

	LineSegment() = default;
	constexpr LineSegment(const Vector3<T>& a, const Vector3<T>& b) noexcept: A(a), B(b) {}
	constexpr Vector3<T> Midpoint() const noexcept {return (A + B)/2;}
};

template<typename T> constexpr T LengthSqr(const LineSegment<T>& l) noexcept {return DistanceSqr(l.A, l.B);}
template<typename T> INTRA_MATH_CONSTEXPR INTRA_FORCEINLINE T Length(const LineSegment<T>& l) noexcept {return Distance(l.A, l.B);}

}}
