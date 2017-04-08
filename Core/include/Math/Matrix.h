#pragma once

#include "Platform/CppFeatures.h"
#include "Platform/CppWarnings.h"
#include "Math/Math.h"
#include "Math/Vector.h"

//#define INTRA_SWAP_VM_MULTIPLY_ORDER
#define INTRA_SWAP_MATRIX_MULTIPLY_ORDER


namespace Intra { namespace Math {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS\

template<typename T> struct Matrix3;
template<typename T> struct Matrix4;

template<typename T> Matrix3<T> Transpose(const Matrix3<T>& m);
template<typename T> Matrix3<T> Inverse(const Matrix3<T>& m);
template<typename T> Matrix4<T> Transpose(const Matrix4<T>& m);
template<typename T> Matrix4<T> Inverse(const Matrix4<T>& m);

template<typename T> struct Matrix3
{
private:
	static Matrix3 multiply(const Matrix3& m1, const Matrix3& m2)
	{
		Matrix3 result;

		result.rows[0].x = m1.rows[0].x*m2.rows[0].x + m1.rows[0].y*m2.rows[1].x + m1.rows[0].z*m2.rows[2].x;
		result.rows[0].y = m1.rows[0].x*m2.rows[0].y + m1.rows[0].y*m2.rows[1].y + m1.rows[0].z*m2.rows[2].y;
		result.rows[0].z = m1.rows[0].x*m2.rows[0].z + m1.rows[0].y*m2.rows[1].z + m1.rows[0].z*m2.rows[2].z;

		result.rows[1].x = m1.rows[1].x*m2.rows[0].x + m1.rows[1].y*m2.rows[1].x + m1.rows[1].z*m2.rows[2].x;
		result.rows[1].y = m1.rows[1].x*m2.rows[0].y + m1.rows[1].y*m2.rows[1].y + m1.rows[1].z*m2.rows[2].y;
		result.rows[1].z = m1.rows[1].x*m2.rows[0].z + m1.rows[1].y*m2.rows[1].z + m1.rows[1].z*m2.rows[2].z;

		result.rows[2].x = m1.rows[2].x*m2.rows[0].x + m1.rows[2].y*m2.rows[1].x + m1.rows[2].z*m2.rows[2].x;
		result.rows[2].y = m1.rows[2].x*m2.rows[0].y + m1.rows[2].y*m2.rows[1].y + m1.rows[2].z*m2.rows[2].y;
		result.rows[2].z = m1.rows[2].x*m2.rows[0].z + m1.rows[2].y*m2.rows[1].z + m1.rows[2].z*m2.rows[2].z;

		return result;
	}

	static Matrix3 transpose_multiply(const Matrix3& m1, const Matrix3& m2)
	{
		Matrix3 result;

		result.rows[0].x = m1.rows[0].x*m2.rows[0].x + m1.rows[1].x*m2.rows[1].x + m1.rows[2].x*m2.rows[2].x;
		result.rows[0].y = m1.rows[0].x*m2.rows[0].y + m1.rows[1].x*m2.rows[1].y + m1.rows[2].x*m2.rows[2].y;
		result.rows[0].z = m1.rows[0].x*m2.rows[0].z + m1.rows[1].x*m2.rows[1].z + m1.rows[2].x*m2.rows[2].z;

		result.rows[1].x = m1.rows[0].y*m2.rows[0].x + m1.rows[1].y*m2.rows[1].x + m1.rows[2].y*m2.rows[2].x;
		result.rows[1].y = m1.rows[0].y*m2.rows[0].y + m1.rows[1].y*m2.rows[1].y + m1.rows[2].y*m2.rows[2].y;
		result.rows[1].z = m1.rows[0].y*m2.rows[0].z + m1.rows[1].y*m2.rows[1].z + m1.rows[2].y*m2.rows[2].z;

		result.rows[2].x = m1.rows[0].z*m2.rows[0].x + m1.rows[1].z*m2.rows[1].x + m1.rows[2].z*m2.rows[2].x;
		result.rows[2].y = m1.rows[0].z*m2.rows[0].y + m1.rows[1].z*m2.rows[1].y + m1.rows[2].z*m2.rows[2].y;
		result.rows[2].z = m1.rows[0].z*m2.rows[0].z + m1.rows[1].z*m2.rows[1].z + m1.rows[2].z*m2.rows[2].z;

		return result;
	}

	static Matrix3 multiply_transpose(const Matrix3& m1, const Matrix3& m2)
	{
		Matrix3 result;

		result.rows[0].x = m1.rows[0].x*m2.rows[0].x + m1.rows[0].y*m2.rows[0].y + m1.rows[0].z*m2.rows[0].z;
		result.rows[0].y = m1.rows[0].x*m2.rows[1].x + m1.rows[0].y*m2.rows[1].y + m1.rows[0].z*m2.rows[1].z;
		result.rows[0].z = m1.rows[0].x*m2.rows[2].x + m1.rows[0].y*m2.rows[2].y + m1.rows[0].z*m2.rows[2].z;

		result.rows[1].x = m1.rows[1].x*m2.rows[0].x + m1.rows[1].y*m2.rows[0].y + m1.rows[1].z*m2.rows[0].z;
		result.rows[1].y = m1.rows[1].x*m2.rows[1].x + m1.rows[1].y*m2.rows[1].y + m1.rows[1].z*m2.rows[1].z;
		result.rows[1].z = m1.rows[1].x*m2.rows[2].x + m1.rows[1].y*m2.rows[2].y + m1.rows[1].z*m2.rows[2].z;

		result.rows[2].x = m1.rows[2].x*m2.rows[0].x + m1.rows[2].y*m2.rows[0].y + m1.rows[2].z*m2.rows[0].z;
		result.rows[2].y = m1.rows[2].x*m2.rows[1].x + m1.rows[2].y*m2.rows[1].y + m1.rows[2].z*m2.rows[1].z;
		result.rows[2].z = m1.rows[2].x*m2.rows[2].x + m1.rows[2].y*m2.rows[2].y + m1.rows[2].z*m2.rows[2].z;

		return result;
	}
public:
	Matrix3() = default;
	explicit Matrix3(T v): Matrix3(v,v,v,v,v,v,v,v,v) {}
	Matrix3(T rX, T rY, T rZ, T uX, T uY, T uZ, T oX, T oY, T oZ)
	{
		rows[0].x=rX; rows[0].y=rY; rows[0].z=rZ;
		rows[1].x=uX; rows[1].y=uY; rows[1].z=uZ;
		rows[2].x=oX; rows[2].y=oY; rows[2].z=oZ;
	}
	explicit Matrix3(const T data[9]) {memcpy(rows, data, sizeof(*this));}
	Matrix3(const Vector2<T>& right, const Vector2<T>& up, const Vector2<T>& origin)
	{
		rows[0].xy = right;  rows[0].z = 0;
		rows[1].xy = up;     rows[1].z = 0;
		rows[2].xy = origin; rows[2].z = 1;
	}

	Matrix3(const Vector3<T>& right, const Vector3<T>& up, const Vector3<T>& forward) {rows[0]=right, rows[1]=up, rows[2]=forward;}
	explicit Matrix3(const Matrix4<T>& m) {rows[0] = m[0].xyz, rows[1] = m[1].xyz, rows[2] = m[2].xyz;}

	bool operator==(const Matrix3& m) const {return memcmp(rows, m, sizeof(*this))==0;}
	bool operator!=(const Matrix3& m) const {return !operator==(m);}

	Matrix3 operator*(const Matrix3& rhs) const
	{
#ifndef INTRA_SWAP_MATRIX_MULTIPLY_ORDER
		return multiply(*this, rhs);
#else
		return multiply(rhs, *this);
#endif
	}
	Matrix3 transposeMul(const Matrix3& rhs) const
	{
#ifndef INTRA_SWAP_MATRIX_MULTIPLY_ORDER
		return transpose_multiply(*this, rhs);
#else
		return multiply_transpose(rhs, *this);
#endif
	}

	Matrix3 mulTranspose(const Matrix3& rhs) const
	{
#ifndef INTRA_SWAP_MATRIX_MULTIPLY_ORDER
		return multiply_transpose(*this, rhs);
#else
		return transpose_multiply(rhs, *this);
#endif
	}

	Matrix3& operator*=(const Matrix3& m) {return *this = *this*m;}

	Vector3<T> operator*(const Vector3<T>& v) const;
	Matrix3 operator*(T n) const {return Matrix3(rows[0]*n, rows[1]*n, rows[2]*n);}
	Matrix3 operator/(T n) const {return Matrix3(rows[0]/n, rows[1]/n, rows[2]/n);}
	Matrix3& operator*=(T n) {rows[0]*=n, rows[1]*=n, rows[2]*=n; return *this;}
	Matrix3& operator/=(T n) {rows[0]/=n, rows[1]/=n, rows[2]/=n; return *this;}
	Matrix3 operator+(const Matrix3& m) const {return Matrix3(rows[0]+m[0], rows[1]+m[1], rows[2]+m[2]);}
	Matrix3 operator-(const Matrix3& m) const {return Matrix3(rows[0]-m[0], rows[1]-m[1], rows[2]-m[2]);}
	Matrix3& operator+=(const Matrix3& m) {rows[0]+=m[0], rows[1]+=m[1], rows[2]+=m[2]; return *this;}
	Matrix3& operator-=(const Matrix3& m) {rows[0]-=m[0], rows[1]-=m[1], rows[2]-=m[2]; return *this;}

	Vector3<T> GetScaling3() const {return {Length(rows[0]), Length(rows[1]), Length(rows[2])};}
	Matrix3 GetRotation3() const {Matrix3 result; DecomposeTransform3(&result, null); return result;}

	void DecomposeTransform3(Matrix3* rotation, Vector3<T>* scaling) const
	{
		const Vector3<T> s = GetScaling3();
		if(scaling!=null) *scaling = s;
		if(rotation!=null)
		{
			rotation->rows[0] = rows[0]/s.x;
			rotation->rows[1] = rows[1]/s.y;
			rotation->rows[2] = rows[2]/s.z;
		}
	}

	Vector2<T> GetScaling2() const {return {Length(rows[0].xy), Length(rows[1].xy)};}
	Vector2<T> GetTranslation2() const {return rows[2].xy;}
	/*Matrix2 GetRotation2() const {Matrix2 result; DecomposeTransform2(&result, null); return result;}

	void DecomposeTransform2(Matrix2* rotation, Vector2<T>* scaling, Vector2<T>* translation) const
	{
		const Vector2<T> s=GetScaling2();
		if(scaling!=null) *scaling=s;
		if(rotation!=null) *rotation=Matrix2(Vector2<T>(rows[0])/s[0], Vector2<T>(rows[1])/s[1]);
		if(translation!=null) *translation=GetTranslation2();
	}*/

	Vector3<T> Row(ushort i) const {return rows[i];}
	Vector3<T> Column(ushort i) const {return {rows[0][i], rows[1][i], rows[2][i]};}
	void SetRow(ushort i, Vector3<T> value) {rows[i] = value;}
	void SetColumn(ushort i, Vector3<T> value) {rows[0][i] = value[0]; rows[1][i] = value[1]; rows[2][i] = value[2];}

	operator Vector3<T>*() {return rows;}
	operator const Vector3<T>*() const {return rows;}

	static const Matrix3 I;

	static Matrix3 Translation2(const Vector2<T>& translation)
	{
		const Matrix3 result = I;
		result[2].xy = translation;
		return result;
	}

	static Matrix3 Rotation3(T angle, Vector3<T> axis)
	{
		const T len = Length(axis);
		axis /= len;

		const T s = T(Sin(angle));
		const T c = T(Cos(angle));

		const T oc = 1-c;

		const T oxx = oc*axis.x*axis.x;
		const T oyy = oc*axis.y*axis.y;
		const T ozz = oc*axis.z*axis.z;
		const T oxy = oc*axis.z*axis.y;
		const T oyz = oc*axis.y*axis.z;
		const T ozx = oc*axis.z*axis.x;

		const T xs = axis.x*s;
		const T ys = axis.y*s;
		const T zs = axis.z*s;

		return {oxx+c, oxy+zs, ozx-ys, oxy-zs, oyy+c, oyz+xs, ozx+ys, oyz-xs, ozz+c};
	}

	static Matrix3 Scaling3(const Vector3<T>& scale)
	{
		return {scale.x,0,0, 0,scale.y,0, 0,0,scale.z};
	}

	static Matrix3 RotationEuler(T rotX, T rotY, T rotZ)
	{
		const float cos_rx = Cos(rotX), cos_ry = Cos(rotY), cos_rz = Cos(rotZ);
		const float sin_rx = -Sin(rotX), sin_ry = -Sin(rotY), sin_rz = -Sin(rotZ);
		return {cos_ry*cos_rz, -sin_rz*cos_rx+cos_rz*sin_ry*sin_rx, sin_rx*sin_rz+cos_rz*sin_ry*cos_rx,
			cos_ry*sin_rz, cos_rz*cos_rx+sin_ry*sin_rz*sin_rx, -sin_rx*cos_rz+sin_rx*sin_rz*cos_rx,
			-sin_ry, cos_ry*sin_rx, cos_ry*cos_rx};
	}

	static Matrix3<T> LookAt(const Vector3<T>& eye, const Vector3<T>& center, const Vector3<T>& up)
	{
		Vector3<T> f = Normalize(center-eye);
		Vector3<T> u = Normalize(up);
		Vector3<T> s = Normalize(Cross(f, u));
		u = Cross(s, f);

		return {s.x, u.x, -f.x,
			    s.y, u.y, -f.y,
			    s.z, u.z, -f.z};
	}

	Matrix3 Orthonormalize() const
	{
		Matrix3 r{};
		r.rows[0] = Normalize(rows[0]);
		r.rows[1] = Normalize(rows[1] - r[0]*Dot(r[0], rows[1]));
		r.rows[2] = Normalize(rows[2] - (r[0]*Dot(r[0], rows[2]) + r[1]*Dot(r[1], rows[2])));
		return r;
	}

	Vector3<T> rows[3];
};



template<typename T> struct Matrix4
{
	Matrix4() = default;
	Matrix4(T val) {*this = I*val;}
	Matrix4(const T data[16]) {memcpy(rows, data, sizeof(*this));}

	Matrix4(const Vector4<T>& right, const Vector4<T>& up, const Vector4<T>& forward, const Vector4<T>& origin)
	{
		rows[0] = right;
		rows[1] = up;
		rows[2] = forward;
		rows[3] = origin;
	}

	Matrix4(T rX, T rY, T rZ, T rW,
		    T uX, T uY, T uZ, T uW,
		    T fX, T fY, T fZ, T fW,
		    T oX, T oY, T oZ, T oW)
	{
		rows[0] = {rX, rY, rZ, rW};
		rows[1] = {uX, uY, uZ, uW};
		rows[2] = {fX, fY, fZ, fW};
		rows[3] = {oX, oY, oZ, oW};
	}

	Matrix4(const Vector3<T>& right, const Vector3<T>& up, const Vector3<T>& forward, const Vector3<T>& origin={0,0,0})
	{
		rows[0].xyz = right;   rows[0].w = 0;
		rows[1].xyz = up;      rows[1].w = 0;
		rows[2].xyz = forward; rows[2].w = 0;
		rows[3].xyz = origin;  rows[3].w = 1;
	}

	static Matrix4 FromRows(const Vector4<T>& right, const Vector4<T>& up, const Vector4<T>& forward, const Vector4<T>& origin)
	{
		Matrix4 m;
		m.rows[0] = right;
		m.rows[1] = up;
		m.rows[2] = forward;
		m.rows[3] = origin;
		return m;
	}

	static Matrix4 FromColumns(const Vector4<T>& col1, const Vector4<T>& col2, const Vector4<T>& col3, const Vector4<T>& col4)
	{
		return Transpose(FromRows(col1, col2, col3, col4));
	}

	explicit Matrix4(const Matrix3<T>& m, const Vector3<T>& translation={0,0,0})
	{
		rows[0] = Vector4<T>(m[0], 0);
		rows[1] = Vector4<T>(m[1], 0);
		rows[2] = Vector4<T>(m[2], 0);
		rows[3] = Vector4<T>(translation, 1);
	}

	bool operator==(const Matrix4& m) const {return memcmp(rows, m, sizeof(*this))==0;}
	bool operator!=(const Matrix4& m) const {return !operator==(m);}

	static Matrix4 mul(const Matrix4& m1, const Matrix4& m2)
	{
		Matrix4 result;
		result.rows[0] = m1.rows[0].x*m2.rows[0] + m1.rows[0].y*m2.rows[1] + m1.rows[0].z*m2.rows[2] + m1.rows[0].w*m2.rows[3];
		result.rows[1] = m1.rows[1].x*m2.rows[0] + m1.rows[1].y*m2.rows[1] + m1.rows[1].z*m2.rows[2] + m1.rows[1].w*m2.rows[3];
		result.rows[2] = m1.rows[2].x*m2.rows[0] + m1.rows[2].y*m2.rows[1] + m1.rows[2].z*m2.rows[2] + m1.rows[2].w*m2.rows[3];
		result.rows[3] = m1.rows[3].x*m2.rows[0] + m1.rows[3].y*m2.rows[1] + m1.rows[3].z*m2.rows[2] + m1.rows[3].w*m2.rows[3];
		return result;
	}

	Matrix4 operator*(const Matrix4& rhs) const
	{
#ifndef INTRA_SWAP_MATRIX_MULTIPLY_ORDER
		return mul(*this, rhs);
#else
		return mul(rhs, *this);
#endif
	}

	Matrix4& operator*=(const Matrix4& rhs) {return *this = *this*rhs;}

	Vector4<T> operator*(const Vector4<T>& v) const;
	Matrix4 operator*(T n) const {return Matrix4(rows[0]*n, rows[1]*n, rows[2]*n, rows[3]*n);}
	Matrix4 operator/(T n) const {return Matrix4(rows[0]/n, rows[1]/n, rows[2]/n, rows[3]/n);}
	Matrix4& operator*=(T n) {rows[0]*=n; rows[1]*=n; rows[2]*=n; rows[3]*=n; return *this;}
	Matrix4& operator/=(T n) {rows[0]/=n; rows[1]/=n; rows[2]/=n; rows[3]/=n; return *this;}
	Matrix4 operator+(const Matrix4& rhs) const {return Matrix4(rows[0]+rhs[0], rows[1]+rhs[1], rows[2]+rhs[2], rows[3]+rhs[3]);}
	Matrix4 operator-(const Matrix4& rhs) const {return Matrix4(rows[0]-rhs[0], rows[1]-rhs[1], rows[2]-rhs[2], rows[3]-rhs[3]);}
	Matrix4& operator+=(const Matrix4& rhs) {rows[0]+=rhs[0], rows[1]+=rhs[1], rows[2]+=rhs[2], rows[3]+=rhs[3]; return *this;}
	Matrix4& operator-=(const Matrix4& rhs) {rows[0]-=rhs[0], rows[1]-=rhs[1], rows[2]-=rhs[2], rows[3]-=rhs[3]; return *this;}

	Matrix4 GetRotation3() const {return Matrix4(Matrix3<T>(*this).GetRotation3());}
	Vector3<T> GetTranslation3() const {return Vector3<T>(Row(3));}
	Vector3<T> GetScaling() const {return Vector3<T>(Length(Vector3<T>(rows[0])), Length(Vector3<T>(rows[1])), Length(Vector3<T>(rows[2])));}

	void DecomposeTransform(Matrix3<T>* rotation, Vector3<T>* scaling, Vector3<T>* translation) const
	{
		Matrix3<T>(*this).DecomposeTransform3(rotation, scaling);
		if(translation!=null) *translation = GetTranslation3();
	}

	Matrix4 Reflect(const Plane<T>& plane) const;

	Vector4<T> Row(uint i) const {return rows[i];}
	Vector4<T> Column(uint i) const {return {rows[0][i], rows[1][i], rows[2][i], rows[3][i]};}
	void SetRow(uint i, const Vec4& value) {rows[i]=value;}
	
	void SetColumn(uint i, const Vec4& value)
	{
		rows[0][i] = value[0];
		rows[1][i] = value[1];
		rows[2][i] = value[2];
		rows[3][i] = value[3];
	}

	operator Vector4<T>*() {return rows;}
	operator const Vector4<T>*() const {return rows;}

	static const Matrix4 I;

	static Matrix4 Ortho(T width, T height, T depth)
	{
		Matrix4 result = I;
		result[0][0] = 2/width;
		result[1][1] = 2/height;
		result[2][2] = -2/depth;
		result[3][0] = result[3][1] = result[3][2] = -1;
		return result;
	}

	static Matrix4 Ortho(T left, T right, T bottom, T top, T zNear, T zFar)
	{
		Matrix4 result = I;
		result[0][0] = 2/(right-left);
		result[1][1] = 2/(top-bottom);
		result[2][2] = -2/(zFar-zNear);
		result[3][0] = -(right+left)/(right-left);
		result[3][1] = -(top+bottom)/(top-bottom);
		result[3][2] = -(zFar+zNear)/(zFar-zNear);
		return result;
	}

	static Matrix4 Perspective(T fovy, T znear, T zfar, T aspectRatio)
	{
		const T f = T(1)/T(Tan(fovy/360*PI));
		return {f/aspectRatio, 0,            0,                0,
		        0,             f,            0,                0,
				0,             0, (zfar+znear)/(znear-zfar),  -1,
			    0,             0, (2*zfar*znear)/(znear-zfar), 0};
	}

	static void ExtractPerspectiveParameters(const Matrix4& m, T* fovy, T* znear, T* zfar, T* aspectRatio)
	{
		T f = m[1][1];
		if(fovy!=null) *fovy = T(Atan(1/f)*360/PI);
		if(aspectRatio!=null) *aspectRatio = f/m[0][0];
		if(zfar!=null) *zfar = m[3][2]/(m[2][2]+1);
		if(znear!=null) *znear = m[3][2]/(m[2][2]-1);
	}

	static Matrix4 Translation(const Vector3<T>& translation)
	{
		Matrix4 result = I;
		result[3].xyz = translation;
		return result;
	}

	static Matrix4<T> LookAt(const Vector3<T>& eye, const Vector3<T>& center, const Vector3<T>& up)
	{
		Vector3<T> f = Normalize(center-eye);
		Vector3<T> u = Normalize(up);
		Vector3<T> s = Normalize(Cross(f, u));
		u = Cross(s, f);

		return {s.x, u.x, -f.x, 0,
		        s.y, u.y, -f.y, 0,
		        s.z, u.z, -f.z, 0,
		-Dot(s,eye), -Dot(u, eye), Dot(f, eye), 1};
	}

	static Matrix4 Rotation(T angle, const Vector3<T>& axis) {return Matrix4(Matrix3<T>::Rotation3(angle, axis));}
	static Matrix4 RotationEuler(T rotX, T rotY, T rotZ) {return Matrix4(Matrix3<T>::RotationEuler(rotX, rotY, rotZ));}

	Vector4<T> rows[4];
};

#ifdef INTRA_SWAP_VM_MULTIPLY_ORDER

template<typename T> Vector3<T> Vector3<T>::operator*(const Matrix3<T>& m) const
{
	return Vector3<T>(m[0][0]*x+m[1][0]*y+m[2][0]*z, m[0][1]*x+m[1][1]*y+m[2][1]*z, m[0][2]*x+m[1][2]*y+m[2][2]*z);
}

template<typename T> Vector4<T> Vector4<T>::operator*(const Matrix4<T>& m) const
{
	return Vector4<T>(m[0][0]*x+m[1][0]*y+m[2][0]*z+m[3][0]*w, m[0][1]*x+m[1][1]*y+m[2][1]*z+m[3][1]*w,
			m[0][2]*x+m[1][2]*y+m[2][2]*z+m[3][2]*w, m[0][3]*x+m[1][3]*y+m[2][3]*z+m[3][3]*w);
}

template<typename T> Vector3<T> Matrix3::operator*(const Vector3<T>& v) const {return Vector3<T>(Dot(v, rows[0]), Dot(v, rows[1]), Dot(v, rows[2]));}
template<typename T> Vector4<T> Matrix4::operator*(const Vector4<T>& v) const {return Vector4<T>(Dot(v, rows[0]), Dot(v, rows[1]), Dot(v, rows[2]), Dot(v, rows[3]));}

#else

template<typename T> Vector3<T> Vector3<T>::operator*(const Matrix3<T>& m) const
{
	return Vector3<T>(Dot(*this, m[0]), Dot(*this, m[1]), Dot(*this, m[2]));
}

template<typename T> Vector4<T> Vector4<T>::operator*(const Matrix4<T>& m) const
{
	return Vector4<T>(Dot(*this, m[0]), Dot(*this, m[1]), Dot(*this, m[2]), Dot(*this, m[3]));
}

template<typename T> Vector3<T> Matrix3<T>::operator*(const Vector3<T>& v) const
{
	return {rows[0][0]*v.x + rows[1][0]*v.y + rows[2][0]*v.z,
		    rows[0][1]*v.x + rows[1][1]*v.y + rows[2][1]*v.z,
		    rows[0][2]*v.x + rows[1][2]*v.y + rows[2][2]*v.z};
}

template<typename T> Vector4<T> Matrix4<T>::operator*(const Vector4<T>& v) const
{
	return {rows[0][0]*v.x + rows[1][0]*v.y + rows[2][0]*v.z + rows[3][0]*v.w,
		    rows[0][1]*v.x + rows[1][1]*v.y + rows[2][1]*v.z + rows[3][1]*v.w,
			rows[0][2]*v.x + rows[1][2]*v.y + rows[2][2]*v.z + rows[3][2]*v.w,
		    rows[0][3]*v.x + rows[1][3]*v.y + rows[2][3]*v.z + rows[3][3]*v.w};
}

#endif


template<typename T> Matrix4<T> Transpose(const Matrix4<T>& m)
{
	Matrix4<T> result{};
	for(uint i=0; i<4; i++)
		for(uint j=0; j<4; j++)
			result[i][j] = m[j][i];
	return result;
}

template<typename T> Matrix3<T> Transpose(const Matrix3<T>& m)
{
	Matrix3<T> r;
	r.rows[0].x = m.rows[0].x; r.rows[0].y = m.rows[1].x; r.rows[0].z = m.rows[2].x;
	r.rows[1].x = m.rows[0].y; r.rows[1].y = m.rows[1].y; r.rows[1].z = m.rows[2].y;
	r.rows[2].x = m.rows[0].z; r.rows[2].y = m.rows[1].z; r.rows[2].z = m.rows[2].z;
	return r;
}


template<typename T> Matrix4<T> Mat4Scaling(const Vector4<T>& scaleVec)
{
	return Matrix4<T>({scaleVec.x,0,0,0, 0,scaleVec.y,0,0, 0,0,scaleVec.z,0, 0,0,0,scaleVec.w});
}

template<typename T> Matrix4<T> Mat4Scaling(const Vector3<T>& scaleVec)
{
	return Matrix4<T>({scaleVec.x,0,0,0, 0,scaleVec.y,0,0, 0,0,scaleVec.z,0, 0,0,0,1});
}

template<typename T> Matrix3<T> Mat3Scaling(const Vector3<T>& scaleVec)
{
	return Matrix3<T>({scaleVec.x,0,0, 0,scaleVec.y,0, 0,0,scaleVec.z});
}

template<typename T> Matrix4<T> Matrix4<T>::Reflect(const Plane<T>& plane) const
{
	plane.normal = Normalize(plane.normal);
	T a = plane.normal.x;
	T b = plane.normal.y;
	T c = plane.normal.z;
	T d = plane.d;
	return {-2*a*a+1, -2*b*a, -2*c*a, 0,
		    -2*a*b, -2*b*b+1, -2*c*b, 0,
		    -2*a*c, -2*b*c, -2*c*c+1, 0,
		    -2*a*d, -2*b*d,   -2*c*d, 1};
}

template<typename T> Matrix4<T> Inverse(const Matrix4<T>& m)
{
	T wtmp[4][8];
	T mn[4];
	for(int i=0; i<4; i++)
	{
		for(int j=0; j<4; j++) wtmp[i][j]=m[j][i];
		for(int j=4; j<8; j++) wtmp[i][j]=(j==i+4);
	}

	float* r[4]={wtmp[0], wtmp[1], wtmp[2], wtmp[3]};
	for(int i=3; i>0; i--) if(Abs(r[i][0])>Abs(r[i-1][0])) Meta::Swap(r[i], r[i-1]);
	if(r[0][0]==0) return m;

	for(int i=1; i<=3; i++) mn[i]=r[i][0]/r[0][0];
	for(int j=1; j<8; j++) for(int i=1; i<=3; i++) r[i][j]-=mn[i]*r[0][j];
	if(Abs(r[3][1])>Abs(r[2][1])) Meta::Swap(r[3], r[2]);
	if(Abs(r[2][1])>Abs(r[1][1])) Meta::Swap(r[2], r[1]);
	if(r[1][1]==0) return m;
	for(int j=2; j<4; j++)
	{
		mn[j]=r[j][1]/r[1][1];
		r[j][2]-=mn[j]*r[1][2];
		r[j][3]-=mn[j]*r[1][3];
	}
	for(int j=4; j<8; j++)
	{
		r[2][j]-=mn[2]*r[1][j];
		r[3][j]-=mn[3]*r[1][j];
	}
	if(Abs(r[3][2])>Abs(r[2][2])) Meta::Swap(r[3], r[2]);

	if(r[2][2]==0) return m;

	mn[3] = r[3][2]/r[2][2];
	for(int i=3; i<=7; i++) r[3][i] -= mn[3]*r[2][i];
	if(r[3][3]==0) return m;

	for(int i=4; i<=7; i++) r[3][i] /= r[3][3];

	for(int i=0; i<3; i++)
	{
		mn[2-i]=r[2-i][3-i];
		for(int j=4; j<8; j++)
			r[2-i][j] = ( r[2-i][j] - r[3-i][j]*mn[2-i] )/r[2-i][2-i];
		for(int j=1; j>=i; j--)
		{
			int ind=j-i;
			mn[ind]=r[ind][3-i];
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

template<typename T> Matrix3<T> Inverse(const Matrix3<T>& m)
{
	const T det = m[0][0]*(m[1][1]*m[2][2]-m[2][1]*m[1][2]) -
		          m[0][1]*(m[1][0]*m[2][2]-m[1][2]*m[2][0]) +
		          m[0][2]*(m[1][0]*m[2][1]-m[1][1]*m[2][0]);

	return {(m[1][1]*m[2][2]-m[2][1]*m[1][2])/det, (m[0][2]*m[2][1]-m[0][1]*m[2][2])/det, (m[0][1]*m[1][2]-m[0][2]*m[1][1])/det,
		    (m[1][2]*m[2][0]-m[1][0]*m[2][2])/det, (m[0][0]*m[2][2]-m[0][2]*m[2][0])/det, (m[1][0]*m[0][2]-m[0][0]*m[1][2])/det,
		    (m[1][0]*m[2][1]-m[2][0]*m[1][1])/det, (m[2][0]*m[0][1]-m[0][0]*m[2][1])/det, (m[0][0]*m[1][1]-m[1][0]*m[0][1])/det};
}

template<typename T> const Matrix3<T> Matrix3<T>::I(1,0,0, 0,1,0, 0,0,1);
template<typename T> const Matrix4<T> Matrix4<T>::I(1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1);

typedef Matrix3<float> Mat3;
typedef Matrix3<double> DMat3;
typedef Matrix3<uint> UMat3;
typedef Matrix3<int> IMat3;
typedef Matrix3<ushort> USMat3;
typedef Matrix3<short> SMat3;
typedef Matrix3<byte> UBMat3;
typedef Matrix3<sbyte> SBMat3;
typedef Matrix3<bool> BMat3;



typedef Matrix4<float> Mat4;
typedef Matrix4<double> DMat4;
typedef Matrix4<uint> UMat4;
typedef Matrix4<int> IMat4;
typedef Matrix4<ushort> USMat4;
typedef Matrix4<short> SMat4;
typedef Matrix4<byte> UBMat4;
typedef Matrix4<sbyte> SBMat4;
typedef Matrix4<bool> BMat4;

namespace GLSL
{
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

	template<typename T> Matrix3<T> transpose(const Matrix3<T>& m) {return Transpose(m);}
	template<typename T> Matrix4<T> transpose(const Matrix4<T>& m) {return Transpose(m);}
	template<typename T> Matrix3<T> Inverse(const Matrix3<T>& m) {return Inverse(m);}
	template<typename T> Matrix4<T> Inverse(const Matrix4<T>& m) {return Inverse(m);}
}

namespace HLSL
{
	typedef Matrix3<float> float3x3;
	typedef Matrix3<double> double3x3;
	typedef Matrix3<uint> uint3x3;
	typedef Matrix3<int> int3x3;
	typedef Matrix3<bool> bool3x3;

	typedef Matrix4<float> float4x4;
	typedef Matrix4<double> double4x4;
	typedef Matrix4<uint> uint4x4;
	typedef Matrix4<int> int4x4;
	typedef Matrix4<bool> bool4x4;

	using GLSL::transpose;
	using GLSL::Inverse;
}

INTRA_WARNING_POP

}}
