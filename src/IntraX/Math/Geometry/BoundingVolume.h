#pragma once

#include "Aabb.h"
#include "Sphere.h"
#include "Obb.h"

namespace Intra { INTRA_BEGIN
namespace Math {

template<typename T> constexpr Aabb<T> BoundingAabb(const Sphere<T>& sphere) noexcept
{return {sphere.Center.minus(sphere.Radius), sphere.Center.plus(sphere.Radius)};}

template<typename T> Aabb<T> BoundingAabb(const Obb<T>& obb) const
{
	Aabb<T> result = {obb.Center, obb.Center};
	//Перебираем все точки параллелепипеда
	//TODO: должен быть способ лучше.
	for(size_t i = 0; i<8; i++)
		result = result.AddPoint(obb.GetPoint(bool(i & 1), bool((i & 2) >> 1), bool(i >> 2)));
	return result;
}

template<typename T> INTRA_MATH_CONSTEXPR Sphere<T> BoundingSphere(const Aabb<T>& aabb) const noexcept
{return {aabb.Center(), aabb.Diagonal()/2};}

template<typename T> INTRA_MATH_CONSTEXPR Sphere<T> BoundingSphere(const Obb<T>& obb) const noexcept
{return {obb.Center, obb.Diagonal()/2};}

}}
