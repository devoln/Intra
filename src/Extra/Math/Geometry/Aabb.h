#pragma once

#include "Intra/Math/Math.h"
#include "Extra/Math/Vector3.h"

INTRA_BEGIN
namespace Math {

template<typename T> struct Triangle;

//! ������������� ��������������, ����������� ����� ��� ������ ���� ��������� ������� ���������.
template<typename T> struct Aabb
{
	//! ��� ��������������� ����� ���������������.
	//! ���� ��� ������-���� i A[i] > B[i], �� ����� AABB �������� ���������������.
	Vector3<T> A, B;

	constexpr Aabb(const Vector3<T>& a, const Vector3<T>& b) noexcept: A(a), B(b) {}

	static constexpr Aabb FromCenterAndExtents(const Vector3<T>& center, const Vector3<T>& extents) noexcept
	{return {center - extents, center + extents};}
	
	static constexpr Aabb FromCenterAndSize(const Vector3<T>& center, const Vector3<T>& size) noexcept
	{return FromCenterAndExtents(center, size/2);}

	static constexpr Aabb FromExtents(const Vector3<T>& extents) noexcept
	{return {-extents, extents};}

	static constexpr Aabb FromSize(const Vector3<T>& size) noexcept
	{return FromExtents(size/2);}

	constexpr bool IsOriented() const noexcept
	{return A.x > B.x || A.y > B.y || A.z > B.z;}

	constexpr T OrientedWidth() const noexcept {return B.x - A.x;}
	constexpr T OrientedHeight() const noexcept {return B.y - A.y;}
	constexpr T OrientedDepth() const noexcept {return B.z - A.z;}
	constexpr Vector3<T> OrientedSize() const noexcept {return B - A;}
	constexpr Vector3<T> OrientedExtents() const noexcept {return OrientedSize()/T(2);}

	constexpr T Width() const noexcept {return Abs(OrientedWidth());}
	constexpr T Height() const noexcept {return Abs(OrientedHeight());}
	constexpr T Depth() const noexcept {return Abs(OrientedDepth());}
	constexpr Vector3<T> Size() const noexcept {return Abs(OrientedSize());}
	constexpr Vector3<T> Extents() const noexcept {return Abs(OrientedExtents());}

	constexpr Vector3<T> Center() const noexcept {return A + OrientedExtents();}
	constexpr T CenterX() const noexcept {return A.x + OrientedWidth();}
	constexpr T CenterY() const noexcept {return A.y + OrientedHeight();}
	constexpr T CenterZ() const noexcept {return A.z + OrientedDepth();}

	//! ����� ����� ���������� ��� ��������� ����� AABB, �������� ������� � pt.
	//! ��������������, ��� AABB �� ������������.
	constexpr Vector3<T> ClosestPointUnoriented(const Vector3<T>& pt) const noexcept {return Clamp(pt, A, B);}

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
	INTRA_FORCEINLINE T MinExtentAxis(int* oAxis=null)
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
	INTRA_FORCEINLINE T MaxExtentAxis(int* oAxis=null)
	{return MaxSizeAxis(oAxis)/T(2);}

	//! ���������� ����������� AABB ���, ����� �� ������� � ���� ����� pt.
	//! ����� ������������, ��� AABB �� ������������. ����� ��������� ������������ ���������.
	constexpr Aabb UnorientedCombinedWith(const Vector3<T>& pt) const
	{return {Min(A, pt), Max(B, pt)};}

	//! ���������� ����������� AABB ���, ����� �� ������� � ���� ����� pt.
	//! ��������� � ����� ������ ����� �� ���������������.
	constexpr Aabb CombinedWith(const Vector3<T>& pt) const
	{return {Min(Min(), pt), Max(Max(), pt)};}

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

	constexpr Vector3<T> Min() const noexcept {return Min(A, B);}
	constexpr Vector3<T> Max() const noexcept {return Max(A, B);}

	constexpr T MinX() const noexcept {return Min(A.x, B.x);}
	constexpr T MaxX() const noexcept {return Max(A.x, B.x);}
	constexpr T MinY() const noexcept {return Min(A.y, B.y);}
	constexpr T MaxY() const noexcept {return Max(A.y, B.y);}
	constexpr T MinZ() const noexcept {return Min(A.z, B.z);}
	constexpr T MaxZ() const noexcept {return Max(A.z, B.z);}

	constexpr Aabb Deoriented() const noexcept {return {Min(), Max()};}
	
	//! ���������� ��������������� ����� AABB.
	//! �������������� AABB ����� ������������� �����, � ������������� - �������������.
	constexpr T OrientedVolume() const {return OrientedWidth()*OrientedHeight()*OrientedDepth();}

	//! ���������� ����� AABB.
	constexpr T Volume() const {return Abs(OrientedVolume());}
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
template<typename T> constexpr bool Contains(const Aabb<T>& aabb, const Vector3<T>& pt) const noexcept
{return UnorientedContains(aabb.Deoriented(), pt);}

//! ����������, �������� �� AABB � ���� ������ AABB.
//! ��������������, ��� ������� AABB �����������������.
//! ���� ��� �� ���, ���������� false � ����� ������.
template<typename T> constexpr bool UnorientedContains(const Aabb<T>& aabb) const
{return UnorientedContains(aabb.A) && UnorientedContains(aabb.B);}

//! ���������� true, ���� ������ AABB �������� � ���� ������ AABB.
template<typename T> constexpr bool Contains(const Aabb<T>& lhs, const Aabb<T>& rhs) const
{return Contains(lhs.Deoriented(), rhs);}

//! ��������� ����������� ���� AABB.
template<typename T> constexpr bool CheckIntersection(const Aabb<T>& a, const Aabb<T>& b)
{
	return b.MinX() <= a.MaxX() && b.MinY() <= a.MaxY() && b.MinZ() <= a.MaxZ() &&
		a.MinX() <= b.MaxX() && a.MinY() <= b.MaxY() && a.MinZ() <= b.MaxZ();
}

//! ��������� ����������� ���� AABB.
//! ��� ��� ������ ���� �� ����������������.
template<typename T> constexpr bool CheckItersectionBothUnoriented(const Aabb<T>& a, const Aabb<T>& b) noexcept
{
	return b.A.x <= a.B.x && b.A.y <= a.B.y && b.A.z <= a.B.z &&
		a.A.x <= b.B.x && a.A.y <= b.B.y && a.A.z <= b.B.z;
}


template<typename T> constexpr T DistanceSqrUnoriented(const Aabb<T>& aabb, const Vector3<T>& pt) noexcept
{return DistanceSqr(pt, aabb.ClosestPointUnoriented(pt));}

template<typename T> constexpr T DistanceSqr(const Aabb<T>& aabb, const Vector3<T>& pt) noexcept
{return DistanceSqr(pt, aabb.ClosestPoint(pt));}

template<typename T> INTRA_MATH_CONSTEXPR T BoxSignedDistanceFromZero(const Vector3<T>& center, const Vector3<T>& extents)
{
	const auto d = Abs(center) - extents;
	return Clamp(Max(d.y, d.z), d.x, T(0)) + Length(Max(d, T(0)));
}

template<typename T> INTRA_MATH_CONSTEXPR T BoxSignedDistance(
	const Vector3<T>& center, const Vector3<T>& extents, const Vector3<T>& pt)
{return BoxSignedDistanceFromZero(center - pt, extents);}

template<typename T> INTRA_MATH_CONSTEXPR T SignedDistance(const Aabb<T>& aabb, const Vector3<T>& pt)
{return BoxSignedDistance(aabb.Center(), aabb.Extents(), pt);}

template<typename T> INTRA_MATH_CONSTEXPR T Distance(const Aabb<T>& aabb, const Vector3<T>& pt)
{return T(Sqrt(DistanceSqr(aabb, pt)));}

}}
