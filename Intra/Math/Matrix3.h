#pragma once

#include "Cpp/Features.h"
#include "Cpp/Warnings.h"
#include "Cpp/Intrinsics.h"

#include "Math/Math.h"
#include "Vector2.h"
#include "Vector3.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Math {

template<typename T> struct Matrix4;

template<typename T> struct Matrix3
{
public:
	Matrix3() = default;
	
	constexpr forceinline Matrix3(T rX, T rY, T rZ, T uX, T uY, T uZ, T oX, T oY, T oZ) noexcept:
		Rows{{rX, rY, rZ}, {uX, uY, uZ}, {oX, oY, oZ}} {}
	
	explicit Matrix3(const T data[9]) {C::memcpy(Rows, data, sizeof(*this));}
	
	constexpr forceinline Matrix3(const Vector2<T>& right, const Vector2<T>& up, const Vector2<T>& origin) noexcept:
		Rows{{right.xy, 0}, {up.xy, 0}, {origin.xy, 1}} {}

	constexpr forceinline Matrix3(const Vector3<T>& right, const Vector3<T>& up, const Vector3<T>& forward) noexcept:
		Rows{right, up, forward} {}

	constexpr forceinline explicit Matrix3(const Matrix4<T>& m) noexcept:
		Rows{m.Rows[0].xyz, m.Rows[1].xyz, m.Rows[2].xyz} {}

	constexpr bool operator==(const Matrix3& rhs) const noexcept
	{return Rows[0] == rhs.Rows[0] && Rows[1] == rhs.Rows[1] && Rows[2] == rhs.Rows[2];}
	
	constexpr bool operator!=(const Matrix3& rhs) const noexcept {return !operator==(rhs);}

	constexpr forceinline Matrix3 operator*(const Matrix3& rhs) const noexcept
	{
		return {
			{
				Rows[0].x*rhs.Rows[0].x + Rows[0].y*rhs.Rows[1].x + Rows[0].z*rhs.Rows[2].x,
				Rows[0].x*rhs.Rows[0].y + Rows[0].y*rhs.Rows[1].y + Rows[0].z*rhs.Rows[2].y,
				Rows[0].x*rhs.Rows[0].z + Rows[0].y*rhs.Rows[1].z + Rows[0].z*rhs.Rows[2].z
			},
			{
				Rows[1].x*rhs.Rows[0].x + Rows[1].y*rhs.Rows[1].x + Rows[1].z*rhs.Rows[2].x,
				Rows[1].x*rhs.Rows[0].y + Rows[1].y*rhs.Rows[1].y + Rows[1].z*rhs.Rows[2].y,
				Rows[1].x*rhs.Rows[0].z + Rows[1].y*rhs.Rows[1].z + Rows[1].z*rhs.Rows[2].z
			},
			{
				Rows[2].x*rhs.Rows[0].x + Rows[2].y*rhs.Rows[1].x + Rows[2].z*rhs.Rows[2].x,
				Rows[2].x*rhs.Rows[0].y + Rows[2].y*rhs.Rows[1].y + Rows[2].z*rhs.Rows[2].y,
				Rows[2].x*rhs.Rows[0].z + Rows[2].y*rhs.Rows[1].z + Rows[2].z*rhs.Rows[2].z
			}
		};
	}

	constexpr forceinline Matrix3 TransposeMultiply(const Matrix3& rhs) const noexcept
	{
		return {
			{
				Rows[0].x*rhs.Rows[0].x + Rows[1].x*rhs.Rows[1].x + Rows[2].x*rhs.Rows[2].x,
				Rows[0].x*rhs.Rows[0].y + Rows[1].x*rhs.Rows[1].y + Rows[2].x*rhs.Rows[2].y,
				Rows[0].x*rhs.Rows[0].z + Rows[1].x*rhs.Rows[1].z + Rows[2].x*rhs.Rows[2].z
			},
			{
				Rows[0].y*rhs.Rows[0].x + Rows[1].y*rhs.Rows[1].x + Rows[2].y*rhs.Rows[2].x,
				Rows[0].y*rhs.Rows[0].y + Rows[1].y*rhs.Rows[1].y + Rows[2].y*rhs.Rows[2].y,
				Rows[0].y*rhs.Rows[0].z + Rows[1].y*rhs.Rows[1].z + Rows[2].y*rhs.Rows[2].z
			},
			{
				Rows[0].z*rhs.Rows[0].x + Rows[1].z*rhs.Rows[1].x + Rows[2].z*rhs.Rows[2].x,
				Rows[0].z*rhs.Rows[0].y + Rows[1].z*rhs.Rows[1].y + Rows[2].z*rhs.Rows[2].y,
				Rows[0].z*rhs.Rows[0].z + Rows[1].z*rhs.Rows[1].z + Rows[2].z*rhs.Rows[2].z
			}
		};
	}

	constexpr forceinline Matrix3 MultiplyTranspose(const Matrix3& rhs) const noexcept
	{
		return {
			{
				Rows[0].x*rhs.Rows[0].x + Rows[0].y*rhs.Rows[0].y + Rows[0].z*rhs.Rows[0].z,
				Rows[0].x*rhs.Rows[1].x + Rows[0].y*rhs.Rows[1].y + Rows[0].z*rhs.Rows[1].z,
				Rows[0].x*rhs.Rows[2].x + Rows[0].y*rhs.Rows[2].y + Rows[0].z*rhs.Rows[2].z
			},
			{
				Rows[1].x*rhs.Rows[0].x + Rows[1].y*rhs.Rows[0].y + Rows[1].z*rhs.Rows[0].z,
				Rows[1].x*rhs.Rows[1].x + Rows[1].y*rhs.Rows[1].y + Rows[1].z*rhs.Rows[1].z,
				Rows[1].x*rhs.Rows[2].x + Rows[1].y*rhs.Rows[2].y + Rows[1].z*rhs.Rows[2].z
			},
			{
				Rows[2].x*rhs.Rows[0].x + Rows[2].y*rhs.Rows[0].y + Rows[2].z*rhs.Rows[0].z,
				Rows[2].x*rhs.Rows[1].x + Rows[2].y*rhs.Rows[1].y + Rows[2].z*rhs.Rows[1].z,
				Rows[2].x*rhs.Rows[2].x + Rows[2].y*rhs.Rows[2].y + Rows[2].z*rhs.Rows[2].z
			}
		};
	}

	forceinline Matrix3& operator*=(const Matrix3& m) noexcept {return *this = *this*m;}

	constexpr Matrix3 operator*(T n) const noexcept {return {Rows[0]*n, Rows[1]*n, Rows[2]*n};}
	constexpr Matrix3 operator/(T n) const {return {Rows[0]/n, Rows[1]/n, Rows[2]/n};}
	
	Matrix3& operator*=(T n) noexcept {Rows[0] *= n, Rows[1] *= n, Rows[2] *= n; return *this;}
	Matrix3& operator/=(T n) {Rows[0] /= n, Rows[1] /= n, Rows[2] /= n; return *this;}
	
	constexpr Matrix3 operator+(const Matrix3& m) const noexcept {return Matrix3(Rows[0]+m[0], Rows[1]+m[1], Rows[2]+m[2]);}
	constexpr Matrix3 operator-(const Matrix3& m) const noexcept {return Matrix3(Rows[0]-m[0], Rows[1]-m[1], Rows[2]-m[2]);}
	
	Matrix3& operator+=(const Matrix3& m) noexcept {Rows[0] += m[0], Rows[1] += m[1], Rows[2] += m[2]; return *this;}
	Matrix3& operator-=(const Matrix3& m) noexcept {Rows[0] -= m[0], Rows[1] -= m[1], Rows[2] -= m[2]; return *this;}

	INTRA_MATH_CONSTEXPR Vector3<T> ExtractScaleVector() const noexcept
	{return {Length(Rows[0]), Length(Rows[1]), Length(Rows[2])};}
	
	//! Извлекает вращение из текущей матрицы, предполагая, что она ортогональна.
	INTRA_MATH_CONSTEXPR Matrix3 ExtractRotation() const {return {Normalize(Rows[0]), Normalize(Rows[1]), Normalize(Rows[2])};}

	INTRA_MATH_CONSTEXPR Vector2<T> ExtractScaleVector2() const noexcept {return {Length(Rows[0].xy), Length(Rows[1].xy)};}
	
	constexpr const Vector2<T>& ExtractTranslation2() const {return Rows[2].xy;}
	/*Matrix2 GetRotation2() const {Matrix2 result; DecomposeTransform2(&result, null); return result;}

	void DecomposeTransform2(Matrix2* rotation, Vector2<T>* scaling, Vector2<T>* translation) const
	{
		const Vector2<T> s = GetScaling2();
		if(scaling!=null) *scaling = s;
		if(rotation!=null) *rotation = Matrix2(Vector2<T>(Rows[0])/s[0], Vector2<T>(Rows[1])/s[1]);
		if(translation!=null) *translation = GetTranslation2();
	}*/

	constexpr forceinline Vector3<T> Column(size_t i) const noexcept
	{return {Rows[0][i], Rows[1][i], Rows[2][i]};}
	
	forceinline void SetColumn(size_t i, const Vector3<T>& value) noexcept
	{Rows[0][i] = value.x; Rows[1][i] = value.y; Rows[2][i] = value.z;}

	static constexpr const Matrix3 I{1,0,0, 0,1,0, 0,0,1};

	static constexpr forceinline Matrix3 CreateTranslation(const Vector2<T>& translation)
	{return {1,0,0, 0,1,0, translation.x, translation.y, 1};}

	//! Возвращает матрицу поворота вокруг оси normalizedAxis на угол angleRadians, измеряемый в радианах.
	//! Предполагает, что вектор normalizedAxis нормирован. Иначе получится некорректный результат.
	static INTRA_MATH_EXTENDED_CONSTEXPR Matrix3 CreateRotationNormalized(T angleInRadians, const Vector3<T>& normalizedAxis)
	{
		const T s = T(Sin(angleInRadians));
		const T c = T(Cos(angleInRadians));

		const T oc = T(1-c);

		const T oxx = oc*normalizedAxis.x*normalizedAxis.x;
		const T oyy = oc*normalizedAxis.y*normalizedAxis.y;
		const T ozz = oc*normalizedAxis.z*normalizedAxis.z;
		const T oxy = oc*normalizedAxis.z*normalizedAxis.y;
		const T oyz = oc*normalizedAxis.y*normalizedAxis.z;
		const T ozx = oc*normalizedAxis.z*normalizedAxis.x;

		const T xs = normalizedAxis.x*s;
		const T ys = normalizedAxis.y*s;
		const T zs = normalizedAxis.z*s;

		return {oxx+c, oxy+zs, ozx-ys, oxy-zs, oyy+c, oyz+xs, ozx+ys, oyz-xs, ozz+c};
	}

	//! Возвращает матрицу поворота вокруг оси axis на угол angleRadians, измеряемый в радианах.
	static INTRA_MATH_EXTENDED_CONSTEXPR Matrix3 CreateRotation(T angleInRadians, const Vector3<T>& axis)
	{return CreateRotationNormalized(angleInRadians, Normalize(axis));}

	static constexpr forceinline Matrix3 CreateScaling(const Vector3<T>& scale) noexcept
	{return {scale.x,0,0, 0,scale.y,0, 0,0,scale.z};}

	static constexpr forceinline Matrix3 CreateScaling(const Vector2<T>& scale) noexcept
	{return {scale.x,0,0, 0,scale.y,0, 0,0,1};}

	static constexpr forceinline Matrix3 CreateScaling(T x, T y, T z) noexcept
	{return {x,0,0, 0,y,0, 0,0,z};}

	static constexpr forceinline Matrix3 CreateScaling(T factor) noexcept
	{return {factor,0,0, 0,factor,0, 0,0,factor};}

	static INTRA_MATH_EXTENDED_CONSTEXPR Matrix3 CreateRotationEuler(T rotX, T rotY, T rotZ)
	{
		const T cos_rx = T(Cos(rotX));
		const T cos_ry = T(Cos(rotY));
		const T cos_rz = T(Cos(rotZ));
		const T sin_rx = T(-Sin(rotX));
		const T sin_ry = T(-Sin(rotY));
		const T sin_rz = T(-Sin(rotZ));
		return {
			{
				cos_ry*cos_rz,
				-sin_rz*cos_rx + cos_rz*sin_ry*sin_rx,
				sin_rx*sin_rz + cos_rz*sin_ry*cos_rx
			},
			{
				cos_ry*sin_rz,
				cos_rz*cos_rx + sin_ry*sin_rz*sin_rx,
				-sin_rx*cos_rz + sin_rx*sin_rz*cos_rx
			},
			{
				-sin_ry,
				cos_ry*sin_rx,
				cos_ry*cos_rx
			}
		};
	}

	static INTRA_MATH_EXTENDED_CONSTEXPR Matrix3 CreateLookAt(const Vector3<T>& eye, const Vector3<T>& center, const Vector3<T>& up)
	{
		const Vector3<T> f = Normalize(center - eye);
		Vector3<T> u = Normalize(up);
		const Vector3<T> s = Normalize(Cross(f, u));
		u = Cross(s, f);

		return {
			{s.x, u.x, -f.x},
			{s.y, u.y, -f.y},
			{s.z, u.z, -f.z}
		};
	}

	INTRA_MATH_EXTENDED_CONSTEXPR Matrix3 Orthonormalize() const
	{
		const auto r0 = Normalize(Rows[0]);
		const auto r1 = Normalize(Rows[1] - r0*Dot(r0, Rows[1]));
		return {
			r0, r1,
			Normalize(Rows[2] - (r0*Dot(r0, Rows[2]) + r1*Dot(r1, Rows[2])))
		};
	}

	Vector3<T> Rows[3];
};

template<typename T> constexpr Vector3<T> operator*(const Vector3<T>& v, const Matrix3<T>& m) noexcept
{return m.Rows[0] * v.x + m.Rows[1] * v.y + m.Rows[2] * v.z;}

template<typename T> forceinline Vector3<T>& operator*=(Vector3<T>& v, const Matrix3<T>& m) noexcept {return v = v*m;}

template<typename T> constexpr Vector3<T> operator*(const Matrix3<T>& m, const Vector3<T>& v) noexcept
{return {Dot(m.Rows[0], v), Dot(m.Rows[1], v), Dot(m.Rows[2], v)};}

template<typename T> constexpr Matrix3<T> Transpose(const Matrix3<T>& m) noexcept
{
	return {
		{m.Rows[0].x, m.Rows[1].x, m.Rows[2].x},
		{m.Rows[0].y, m.Rows[1].y, m.Rows[2].y},
		{m.Rows[0].z, m.Rows[1].z, m.Rows[2].z}
	};
}

template<typename T> Matrix3<T> Inverse(const Matrix3<T>& m)
{
	const T det = m.Rows[0].x*(m.Rows[1].y*m.Rows[2].z - m.Rows[2].y*m.Rows[1].z) -
	              m.Rows[0].y*(m.Rows[1].x*m.Rows[2].z - m.Rows[1].z*m.Rows[2].x) +
	              m.Rows[0].z*(m.Rows[1].x*m.Rows[2].y - m.Rows[1].y*m.Rows[2].x);

	return {
		{
			(m.Rows[1].y*m.Rows[2].z - m.Rows[2].y*m.Rows[1].z) / det,
			(m.Rows[0].z*m.Rows[2].y - m.Rows[0].y*m.Rows[2].z) / det,
			(m.Rows[0].y*m.Rows[1].z - m.Rows[0].z*m.Rows[1].y) / det
		},
		{
			(m.Rows[1].z*m.Rows[2].x - m.Rows[1].x*m.Rows[2].z) / det,
			(m.Rows[0].x*m.Rows[2].z - m.Rows[0].z*m.Rows[2].x) / det,
			(m.Rows[1].x*m.Rows[0].z - m.Rows[0].x*m.Rows[1].z) / det
		},
		{
			(m.Rows[1].x*m.Rows[2].y - m.Rows[2].x*m.Rows[1].y) / det,
			(m.Rows[2].x*m.Rows[0].y - m.Rows[0].x*m.Rows[2].y) / det,
			(m.Rows[0].x*m.Rows[1].y - m.Rows[1].x*m.Rows[0].y) / det
		}
	};
}

typedef Matrix3<float> Mat3;
typedef Matrix3<double> DMat3;
typedef Matrix3<uint> UMat3;
typedef Matrix3<int> IMat3;
typedef Matrix3<ushort> USMat3;
typedef Matrix3<short> SMat3;
typedef Matrix3<byte> UBMat3;
typedef Matrix3<sbyte> SBMat3;
typedef Matrix3<bool> BMat3;

namespace GLSL {
typedef Matrix3<float> mat3;
typedef Matrix3<double> dmat3;
typedef Matrix3<uint> umat3;
typedef Matrix3<int> imat3;
typedef Matrix3<bool> bmat3;

typedef Matrix3<float> mat3x3;
typedef Matrix3<double> dmat3x3;
typedef Matrix3<uint> umat3x3;
typedef Matrix3<int> imat3x3;
typedef Matrix3<bool> bmat3x3;

template<typename T> constexpr Matrix3<T> transpose(const Matrix3<T>& m) {return Transpose(m);}
template<typename T> Matrix3<T> inverse(const Matrix3<T>& m) {return Inverse(m);}
}

namespace HLSL {
typedef Matrix3<float> float3x3;
typedef Matrix3<double> double3x3;
typedef Matrix3<uint> uint3x3;
typedef Matrix3<int> int3x3;
typedef Matrix3<bool> bool3x3;

using GLSL::transpose;
using GLSL::inverse;
}

}}

INTRA_WARNING_POP
