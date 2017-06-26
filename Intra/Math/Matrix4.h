#pragma once

#include "Cpp/Features.h"
#include "Cpp/Warnings.h"
#include "Cpp/Intrinsics.h"

#include "Meta/Type.h"

#include "Math/Math.h"
#include "Vector3.h"
#include "Vector4.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Math {

template<typename T> struct Matrix3;

template<typename T> struct Matrix4
{
	Matrix4() = default;
	
	explicit Matrix4(const T data[16]) {C::memcpy(Rows, data, sizeof(*this));}

	constexpr Matrix4(const Vector4<T>& right, const Vector4<T>& up,
		const Vector4<T>& forward, const Vector4<T>& origin) noexcept:
		Rows{right, up, forward, origin} {}

	constexpr Matrix4(T rX, T rY, T rZ, T rW, T uX, T uY, T uZ, T uW,
		T fX, T fY, T fZ, T fW, T oX, T oY, T oZ, T oW) noexcept:
		Rows{{rX, rY, rZ, rW}, {uX, uY, uZ, uW}, {fX, fY, fZ, fW}, {oX, oY, oZ, oW}} {}

	constexpr Matrix4(const Vector3<T>& right, const Vector3<T>& up,
		const Vector3<T>& forward, const Vector3<T>& origin = {0,0,0}) noexcept:
		Rows{{right, 0}, {up, 0}, {forward, 0}, {origin, 1}} {}

	static constexpr Matrix4 FromColumns(const Vector4<T>& col1, const Vector4<T>& col2,
		const Vector4<T>& col3, const Vector4<T>& col4) noexcept
	{
		return {
			{col1.x, col2.x, col3.x, col4.x},
			{col1.y, col2.y, col3.y, col4.y},
			{col1.z, col2.z, col3.z, col4.z},
			{col1.w, col2.w, col3.w, col4.w}
		};
	}

	constexpr explicit Matrix4(const Matrix3<T>& m, const Vector3<T>& translation={0,0,0}) noexcept:
		Rows{{m.Rows[0], 0}, {m.Rows[1], 0}, {m.Rows[2], 0}, {translation, 1}} {}

	constexpr bool operator==(const Matrix4& rhs) const noexcept
	{
		return Rows[0] == rhs.Rows[0] && Rows[1] == rhs.Rows[1] &&
			Rows[2] == rhs.Rows[2] && Rows[3] == rhs.Rows[3];
	}

	constexpr bool operator!=(const Matrix4& rhs) const noexcept {return !operator==(rhs);}

	constexpr Matrix4 operator*(const Matrix4& rhs) const noexcept
	{
		return {
			Rows[0].x*rhs.Rows[0] + Rows[0].y*rhs.Rows[1] + Rows[0].z*rhs.Rows[2] + Rows[0].w*rhs.Rows[3],
			Rows[1].x*rhs.Rows[0] + Rows[1].y*rhs.Rows[1] + Rows[1].z*rhs.Rows[2] + Rows[1].w*rhs.Rows[3],
			Rows[2].x*rhs.Rows[0] + Rows[2].y*rhs.Rows[1] + Rows[2].z*rhs.Rows[2] + Rows[2].w*rhs.Rows[3],
			Rows[3].x*rhs.Rows[0] + Rows[3].y*rhs.Rows[1] + Rows[3].z*rhs.Rows[2] + Rows[3].w*rhs.Rows[3]
		};
	}

	forceinline Matrix4& operator*=(const Matrix4& rhs) noexcept {return *this = *this*rhs;}

	constexpr Matrix4 operator*(T n) const noexcept {return {Rows[0]*n, Rows[1]*n, Rows[2]*n, Rows[3]*n};}
	constexpr Matrix4 operator/(T n) const {return {Rows[0]/n, Rows[1]/n, Rows[2]/n, Rows[3]/n};}
	
	Matrix4& operator*=(T n) noexcept {Rows[0] *= n; Rows[1] *= n; Rows[2] *= n; Rows[3] *= n; return *this;}
	Matrix4& operator/=(T n) {Rows[0] /= n; Rows[1] /= n; Rows[2] /= n; Rows[3] /= n; return *this;}
	
	constexpr Matrix4 operator+(const Matrix4& rhs) const noexcept
	{return {Rows[0] + rhs.Rows[0], Rows[1] + rhs.Rows[1], Rows[2] + rhs.Rows[2], Rows[3] + rhs.Rows[3]};}
	
	constexpr Matrix4 operator-(const Matrix4& rhs) const noexcept
	{return {Rows[0] - rhs.Rows[0], Rows[1] - rhs.Rows[1], Rows[2] - rhs.Rows[2], Rows[3] - rhs.Rows[3]};}
	
	Matrix4& operator+=(const Matrix4& rhs) noexcept
	{Rows[0] += rhs.Rows[0], Rows[1] += rhs.Rows[1], Rows[2] += rhs.Rows[2], Rows[3] += rhs.Rows[3]; return *this;}
	
	Matrix4& operator-=(const Matrix4& rhs) noexcept
	{Rows[0] -= rhs.Rows[0], Rows[1] -= rhs.Rows[1], Rows[2] -= rhs.Rows[2], Rows[3] -= rhs.Rows[3]; return *this;}

	//! Извлекает поворот из текущей матрицы.
	//! Предполагает, что строки матрицы ортогональны и что матрица представляет собой композицию поворота, масштабирования и переноса.
	//! Возвращает 4x4 матрицу, содержащую только поворот.
	INTRA_MATH_CONSTEXPR Matrix4 ExtractRotation3() const
	{
		return {
			{Normalize(Rows[0].xyz), 0},
			{Normalize(Rows[1].xyz), 0},
			{Normalize(Rows[2].xyz), 0},
			{0, 0, 0, 1}
		};
	}

	//! Извлекает поворот из текущей матрицы.
	//! Предполагает, что строки матрицы ортогональны и что матрица представляет собой композицию поворота, масштабирования и переноса.
	INTRA_MATH_CONSTEXPR Matrix4 ExtractRotationMat3() const
	{
		return {
			Normalize(Rows[0].xyz),
			Normalize(Rows[1].xyz),
			Normalize(Rows[2].xyz)
		};
	}

	constexpr const Vector3<T>& ExtractTranslationVector() const {return Rows[3].xyz;}
	INTRA_MATH_CONSTEXPR Vector3<T> ExtractScaleVector() const noexcept {return {Length(Rows[0].xyz), Length(Rows[1].xyz), Length(Rows[2].xyz)};}


	forceinline constexpr Vector4<T> Column(uint i) const {return {Rows[0][i], Rows[1][i], Rows[2][i], Rows[3][i]};}
	
	void SetColumn(size_t i, const Vector4<T>& value)
	{
		Rows[0][i] = value.x;
		Rows[1][i] = value.y;
		Rows[2][i] = value.z;
		Rows[3][i] = value.w;
	}

	static constexpr const Matrix4 I{1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};

	static constexpr Matrix4 CreateOrtho(T width, T height, T depth)
	{
		return {
			{2/width, 0, 0, 0},
			{0, 2/height, 0, 0},
			{0, 0, -2/depth, 0},
			{-1, -1,  -1,  1}
		};
	}

	static constexpr Matrix4 CreateOrtho(T left, T right, T bottom, T top, T zNear, T zFar)
	{
		return {
			{2/(right - left), 0, 0, 0},
			{0, 2/(top - bottom), 0, 0},
			{0, 0, -2/(zFar-zNear), 0},
			{-(right + left) / (right - left), -(top + bottom) / (top - bottom), -(zFar + zNear) / (zFar - zNear), 1}
		};
	}

	static INTRA_MATH_EXTENDED_CONSTEXPR Matrix4 CreatePerspective(T fovy, T znear, T zfar, T aspectRatio)
	{
		const T f = T(1) / T(Tan(fovy/360*PI));
		return {f/aspectRatio, 0,            0,                    0,
		        0,             f,            0,                    0,
		        0,             0, (zfar + znear) / (znear - zfar),-1,
		        0,             0, (2*zfar*znear) / (znear - zfar), 0};
	}

	INTRA_MATH_CONSTEXPR forceinline T ExtractPerspectiveFovY() const
	{return T(Atan(T(1) / Rows[1].y)*(360/PI));}

	constexpr forceinline T ExtractPerspectiveAspectRatio() const
	{return T(Rows[1].y / Rows[0].x);}

	constexpr forceinline T ExtractPerspectiveZNear() const
	{return T(Rows[3].z / (Rows[2].z - 1));}

	constexpr forceinline T ExtractPerspectiveZFar() const
	{return T(Rows[3].z / (Rows[2].z + 1));}

	static constexpr Matrix4 CreateTranslation(const Vector3<T>& translation) noexcept
	{
		return {
			{1, 0, 0, 0},
			{0, 1, 0, 0},
			{0, 0, 1, 0},
			{translation, 1}
		};
	}

	static INTRA_MATH_EXTENDED_CONSTEXPR Matrix4 LookAt(const Vector3<T>& eye, const Vector3<T>& center, const Vector3<T>& up)
	{
		const Vector3<T> f = Normalize(center - eye);
		Vector3<T> u = Normalize(up);
		const Vector3<T> s = Normalize(Cross(f, u));
		u = Cross(s, f);

		return {s.x,           u.x,        -f.x,     0,
		        s.y,           u.y,        -f.y,     0,
		        s.z,           u.z,        -f.z,     0,
		    -Dot(s, eye), -Dot(u, eye), Dot(f, eye), 1};
	}

	static INTRA_MATH_EXTENDED_CONSTEXPR Matrix4 CreateRotation(T angle, const Vector3<T>& axis)
	{return Matrix4(Matrix3<T>::Rotation3(angle, axis));}
	
	static INTRA_MATH_EXTENDED_CONSTEXPR Matrix4 CreateRotationEuler(T rotX, T rotY, T rotZ)
	{return Matrix4(Matrix3<T>::RotationEuler(rotX, rotY, rotZ));}

	static constexpr forceinline Matrix4 CreateScaling(const Vector3<T>& scale) noexcept
	{return {scale.x,0,0,0, 0,scale.y,0,0, 0,0,scale.z,0, 0,0,0,1};}

	static constexpr forceinline Matrix4 CreateScaling(T x, T y, T z) noexcept
	{return {x,0,0,0, 0,y,0,0, 0,0,z,0, 0,0,0,1};}

	static constexpr forceinline Matrix4 CreateScaling(T factor) noexcept
	{return {factor,0,0,0, 0,factor,0,0, 0,0,factor,0, 0,0,0,1};}

	Vector4<T> Rows[4];
};

template<typename T> constexpr Vector4<T> operator*(const Vector4<T>& v, const Matrix4<T>& m) noexcept
{return m.Rows[0] * v.x + m.Rows[1] * v.y + m.Rows[2] * v.z + m.Rows[3] * v.w;}

template<typename T> forceinline Vector4<T>& operator*=(Vector4<T>& v, const Matrix4<T>& m) noexcept {return v = v*m;}

template<typename T> constexpr Vector4<T> operator*(const Matrix4<T>& m, const Vector4<T>& v) noexcept
{return {Dot(m.Rows[0], v), Dot(m.Rows[1], v), Dot(m.Rows[2], v), Dot(m.Rows[3], v)};}


template<typename T> constexpr Matrix4<T> Transpose(const Matrix4<T>& m) noexcept
{
	return {
		{m.Rows[0].x, m.Rows[1].x, m.Rows[2].x, m.Rows[3].x},
		{m.Rows[0].y, m.Rows[1].y, m.Rows[2].y, m.Rows[3].y},
		{m.Rows[0].z, m.Rows[1].z, m.Rows[2].z, m.Rows[3].z},
		{m.Rows[0].w, m.Rows[1].w, m.Rows[2].w, m.Rows[3].w}
	};
}

template<typename T> struct Plane;

template<typename T> INTRA_MATH_EXTENDED_CONSTEXPR Matrix4<T> Reflect(const Matrix4<T>& m, const Plane<T>& plane)
{
	const auto p = Normalize(plane);
	return {-2*p.A*p.A+1, -2*p.B*p.A, -2*p.C*p.A, 0,
	        -2*p.A*p.B, -2*p.B*p.B+1, -2*p.C*p.B, 0,
	        -2*p.A*p.C, -2*p.B*p.C, -2*p.C*p.C+1, 0,
	        -2*p.A*p.D, -2*p.B*p.D,   -2*p.C*p.D, 1};
}

template<typename T> Matrix4<T> Inverse(const Matrix4<T>& m)
{
	T wtmp[4][8];
	T mn[4];
	for(int i=0; i<4; i++)
	{
		for(int j=0; j<4; j++) wtmp[i][j] = m[j][i];
		for(int j=4; j<8; j++) wtmp[i][j] = (j == i+4);
	}

	float* r[4] = {wtmp[0], wtmp[1], wtmp[2], wtmp[3]};
	for(int i=3; i>0; i--)
	{
		if(Abs(r[i][0]) <= Abs(r[i-1][0])) continue;
		Cpp::Swap(r[i], r[i-1]);
	}
	if(r[0][0]==0) return m;

	for(int i=1; i<=3; i++)
		mn[i] = r[i][0]/r[0][0];

	for(int j=1; j<8; j++)
		for(int i=1; i<=3; i++)
			r[i][j] -= mn[i]*r[0][j];

	if(Abs(r[3][1]) > Abs(r[2][1])) Cpp::Swap(r[3], r[2]);
	if(Abs(r[2][1]) > Abs(r[1][1])) Cpp::Swap(r[2], r[1]);

	if(r[1][1] == 0) return m;

	for(int j=2; j<4; j++)
	{
		mn[j] = r[j][1] / r[1][1];
		r[j][2] -= mn[j] * r[1][2];
		r[j][3] -= mn[j] * r[1][3];
	}
	for(int j=4; j<8; j++)
	{
		r[2][j] -= mn[2]*r[1][j];
		r[3][j] -= mn[3]*r[1][j];
	}
	if(Abs(r[3][2]) > Abs(r[2][2])) Cpp::Swap(r[3], r[2]);

	if(r[2][2] == 0) return m;

	mn[3] = r[3][2] / r[2][2];
	for(int i=3; i<=7; i++)
		r[3][i] -= mn[3]*r[2][i];
	if(r[3][3] == 0) return m;

	for(int i=4; i<=7; i++)
		r[3][i] /= r[3][3];

	for(int i=0; i<3; i++)
	{
		mn[2-i] = r[2-i][3-i];

		for(int j=4; j<8; j++)
			r[2-i][j] = ( r[2-i][j] - r[3-i][j]*mn[2-i] )/r[2-i][2-i];

		for(int j=1; j>=i; j--)
		{
			int ind = j-i;
			mn[ind] = r[ind][3-i];
			r[ind][4] -= r[3-i][4]*mn[ind];
			r[ind][5] -= r[3-i][5]*mn[ind];
			r[ind][6] -= r[3-i][6]*mn[ind];
			r[ind][7] -= r[3-i][7]*mn[ind];
		}
	}

	Matrix4<T> result{};
	for(uint i=0; i<4; i++)
		for(uint j=0; j<4; j++)
			result[i][j] = r[j][i+4];
	return result;
}



typedef Matrix4<float> Mat4;
typedef Matrix4<double> DMat4;
typedef Matrix4<uint> UMat4;
typedef Matrix4<int> IMat4;
typedef Matrix4<ushort> USMat4;
typedef Matrix4<short> SMat4;
typedef Matrix4<byte> UBMat4;
typedef Matrix4<sbyte> SBMat4;
typedef Matrix4<bool> BMat4;

namespace GLSL {
typedef Matrix4<float> mat4;
typedef Matrix4<double> dmat4;
typedef Matrix4<uint> umat4;
typedef Matrix4<int> imat4;
typedef Matrix4<bool> bmat4;

typedef Matrix4<float> mat4x4;
typedef Matrix4<double> dmat4x4;
typedef Matrix4<uint> umat4x4;
typedef Matrix4<int> imat4x4;
typedef Matrix4<bool> bmat4x4;

template<typename T> constexpr Matrix4<T> transpose(const Matrix4<T>& m) {return Transpose(m);}
template<typename T> Matrix4<T> inverse(const Matrix4<T>& m) {return Inverse(m);}
}

namespace HLSL {
typedef Matrix4<float> float4x4;
typedef Matrix4<double> double4x4;
typedef Matrix4<uint> uint4x4;
typedef Matrix4<int> int4x4;
typedef Matrix4<bool> bool4x4;

using GLSL::transpose;
using GLSL::inverse;
}

INTRA_WARNING_POP

}}
