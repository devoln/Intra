#pragma once

#include "Math/Math.h"
#include "Math/Vector3.h"

namespace Intra { namespace Math {

template<typename T> struct Triangle;

//! ������������� ��������������, ����������� ����� ��� ������ ���� ��������� ������� ���������.
template<typename T> struct Aabb
{
	//! ��� ��������������� ����� ���������������.
	//! ���� ��� ������-���� i A[i] > B[i], �� ����� AABB �������� ���������������.
	Vector3<T> A, B;

	constexpr forceinline Aabb(const Vector3<T>& a, const Vector3<T>& b) noexcept: A(a), B(b) {}

	static constexpr Aabb FromCenterAndExtents(const Vector3<T>& center, const Vector3<T>& extents) noexcept
	{return {center - extents, center + extents};}
	
	static constexpr Aabb FromCenterAndSize(const Vector3<T>& center, const Vector3<T>& size) noexcept
	{return FromCenterAndExtents(center, size/2);}

	static constexpr Aabb FromExtents(const Vector3<T>& extents) noexcept
	{return {-extents, extents};}

	static constexpr Aabb FromSize(const Vector3<T>& size) noexcept
	{return FromExtents(size/2);}

	constexpr forceinline bool IsOriented() const noexcept
	{return A.x > B.x || A.y > B.y || A.z > B.z;}

	constexpr forceinline T OrientedWidth() const noexcept {return B.x - A.x;}
	constexpr forceinline T OrientedHeight() const noexcept {return B.y - A.y;}
	constexpr forceinline T OrientedDepth() const noexcept {return B.z - A.z;}
	constexpr forceinline Vector3<T> OrientedSize() const noexcept {return B - A;}
	constexpr forceinline Vector3<T> OrientedExtents() const noexcept {return OrientedSize()/T(2);}

	constexpr forceinline T Width() const noexcept {return Abs(OrientedWidth());}
	constexpr forceinline T Height() const noexcept {return Abs(OrientedHeight());}
	constexpr forceinline T Depth() const noexcept {return Abs(OrientedDepth());}
	constexpr forceinline Vector3<T> Size() const noexcept {return Abs(OrientedSize());}
	constexpr forceinline Vector3<T> Extents() const noexcept {return Abs(OrientedExtents());}

	constexpr Vector3<T> Center() const noexcept {return A + OrientedExtents();}
	constexpr T CenterX() const noexcept {return A.x + OrientedWidth();}
	constexpr T CenterY() const noexcept {return A.y + OrientedHeight();}
	constexpr T CenterZ() const noexcept {return A.z + OrientedDepth();}

	//! ����� ����� ���������� ��� ��������� ����� AABB, �������� ������� � pt.
	//! ��������������, ��� AABB �� ������������.
	constexpr Vector3<T> ClosestPointUnoriented(const Vector3<T>& pt) const {return Clamp(pt, A, B);}

	//! ����� ����� ���������� ��� ��������� ����� AABB, �������� ������� � pt.
	constexpr Vector3<T> ClosestPoint(const Vector3<T>& pt) const {return Clamp(pt, Min(), Max());}


	constexpr T DiagonalSqr() const noexcept {return LengthSqr(OrientedSize());}
	INTRA_MATH_CONSTEXPR T Diagonal() const noexcept {return Length(OrientedSize());}

	T SqrDistance(const Vector3<T>& pt, Vector3<T>* nearestPoint=null) const;

	T Distance(const Vec3& pt, Vector3<T>* nearestPoint=null) const
	{return T(Sqrt(SqrDistance(pt, nearestPoint)));}

	//! ������� ��������� �����.
	T MinSizeAxis(int* oAxis=null) const
	{
		const auto size = Size();
		T minSize = size.x;
		if(oAxis) *oAxis=0;
		if(size.y < minSize)
		{
			minSize = size.y;
			if(oAxis) *oAxis = 1;
		}
		if(size.z < minSize)
		{
			minSize = size.z;
			if(oAxis) *oAxis = 2;
		}
		return minSize;
	}

	//! ������ ��������� �����.
	forceinline T MinExtentAxis(int* oAxis=null)
	{return MinSizeAxis(oAxis)/T(2);}

	//! ������� ��������� �����.
	T MaxSizeAxis(size_t* oAxis=null) const
	{
		const auto size = Size();
		T maxSize = size.x;
		if(oAxis) *oAxis = 0;
		if(size.y > maxSize)
		{
			maxSize = size.y;
			if(oAxis) *oAxis = 1;
		}
		if(size.z > maxSize)
		{
			maxSize = size.z;
			if(oAxis) *oAxis = 2;
		}
		return maxSize;
	}

	//! ������ ��������� �����
	forceinline T MaxExtentAxis(int* oAxis=null)
	{return MaxSizeAxis(oAxis)/T(2);}

	//! ���������� ����������� AABB ���, ����� �� ������� � ���� ����� pt.
	//! ����� ������������, ��� AABB �� ������������. ����� ��������� ������������ ���������.
	constexpr Aabb UnorientedCombinedWith(const Vector3<T>& pt) const
	{return {Math::Min(A, pt), Math::Max(B, pt)};}

	//! ���������� ����������� AABB ���, ����� �� ������� � ���� ����� pt.
	//! ��������� � ����� ������ ����� �� ���������������.
	constexpr Aabb CombinedWith(const Vector3<T>& pt) const
	{return {Math::Min(Min(), pt), Math::Max(Max(), pt)};}

	constexpr Aabb CombinedWith(const Aabb& b) const
	{
		return CombinedWith(b.A)
			.UnorientedCombinedWith(b.B);
	}

	constexpr Aabb CombinedWith(const Triangle<T>& tri) const
	{
		return CombinedWith(tri.A)
			.UnorientedCombinedWith(tri.B)
			.UnorientedCombinedWith(tri.C);
	}

	constexpr Aabb UnorientedCombinedWith(const Triangle<T>& tri) const
	{
		return UnorientedCombinedWith(tri.A)
			.UnorientedCombinedWith(tri.B)
			.UnorientedCombinedWith(tri.C);
	}

	constexpr forceinline Vector3<T> Min() const noexcept {return Math::Min(A, B);}
	constexpr forceinline Vector3<T> Max() const noexcept {return Math::Max(A, B);}

	constexpr forceinline T MinX() const noexcept {return Math::Min(A.x, B.x);}
	constexpr forceinline T MaxX() const noexcept {return Math::Max(A.x, B.x);}
	constexpr forceinline T MinY() const noexcept {return Math::Min(A.y, B.y);}
	constexpr forceinline T MaxY() const noexcept {return Math::Max(A.y, B.y);}
	constexpr forceinline T MinZ() const noexcept {return Math::Min(A.z, B.z);}
	constexpr forceinline T MaxZ() const noexcept {return Math::Max(A.z, B.z);}

	constexpr Aabb Deoriented() const noexcept {return {Min(), Max()};}
	
	//! ���������� ��������������� ����� AABB.
	//! �������������� AABB ����� ������������� �����, � ������������� - �������������.
	constexpr forceinline T OrientedVolume() const {return OrientedWidth()*OrientedHeight()*OrientedDepth();}

	//! ���������� ����� AABB.
	constexpr forceinline T Volume() const {return Abs(OrientedVolume());}
};

//! ����������, �������� �� AABB �����.
//! ��������������, ��� AABB �� ���������������.
//! ���� ��� �� ���, ���������� false � ����� ������.
template<typename T> constexpr bool UnorientedContains(const Aabb<T>& aabb, const Vector3<T>& pt) const noexcept
{
	return pt.x >= aabb.A.x && pt.x <= aabb.B.x &&
		pt.y >= aabb.A.y && pt.y <= aabb.B.y &&
		pt.z >= aabb.A.z && pt.z <= aabb.B.z;
}

//! ����������, �������� �� AABB �����.
template<typename T> constexpr forceinline bool Contains(const Aabb<T>& aabb, const Vector3<T>& pt) const noexcept
{return UnorientedContains(aabb.Deoriented(), pt);}

//! ����������, �������� �� AABB � ���� ������ AABB.
//! ��������������, ��� ������� AABB �����������������.
//! ���� ��� �� ���, ���������� false � ����� ������.
template<typename T> constexpr forceinline bool UnorientedContains(const Aabb<T>& aabb) const
{return UnorientedContains(aabb.A) && UnorientedContains(aabb.B);}

//! ���������� true, ���� ������ AABB �������� � ���� ������ AABB.
template<typename T> constexpr forceinline bool Contains(const Aabb<T>& lhs, const Aabb<T>& rhs) const
{return Contains(lhs.Deoriented(), rhs);}

//! ��������� ����������� ���� AABB.
template<typename T> constexpr bool CheckIntersection(const Aabb<T>& a, const Aabb<T>& b)
{
	return b.MinX() <= a.MaxX() && b.MinY() <= a.MaxY() && b.MinZ() <= a.MaxZ() &&
		a.MinX() <= b.MaxX() && a.MinY() <= b.MaxY() && a.MinZ() <= b.MaxZ();
}

//! ��������� ����������� ���� AABB.
//! ��� ��� ������ ���� �� ����������������.
template<typename T> constexpr bool CheckItersectionBothUnoriented(const Aabb<T>& a, const Aabb<T>& b)
{
	return b.A.x <= a.B.x && b.A.y <= a.B.y && b.A.z <= a.B.z &&
		a.A.x <= b.B.x && a.A.y <= b.B.y && a.A.z <= b.B.z;
}


template<typename T> constexpr T DistanceSqrUnoriented(const Aabb<T>& aabb, const Vector3<T>& pt)
{return DistanceSqr(pt, aabb.ClosestPointUnoriented(pt));}

template<typename T> constexpr T DistanceSqr(const Aabb<T>& aabb, const Vector3<T>& pt)
{return DistanceSqr(pt, aabb.ClosestPoint(pt));}

template<typename T> T Distance(const Aabb<T>& aabb, const Vector3<T>& pt) {return T(Sqrt(DistanceSqr(aabb, pt)));}

}}
