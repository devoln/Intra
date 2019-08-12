#pragma once

#include "Core/Core.h"
#include "Math/Matrix3.h"

INTRA_BEGIN

#ifdef _MSC_VER
#pragma warning(disable: 4201) //Do not complain about compiler extension: union { struct { ... }; ...};
#endif

inline namespace Math {

template<typename T> struct Quaternion
{
	union
	{
		struct {
			T x, y, z, w;
		};
		Vector2<T> xy;
		Vector3<T> xyz;
		Vector4<T> xyzw;
	};

	forceinline Quaternion() = default;
	constexpr forceinline Quaternion(T X, T Y=0, T Z=0, T W=1): x(X), y(Y), z(Z), w(W) {}
	constexpr forceinline Quaternion(const Vector3<T>& v): xyz(v), w(1) {}

	Quaternion(const Matrix3<T>& mat);

	constexpr forceinline Quaternion operator+() const {return *this;}
	
	constexpr forceinline Quaternion operator-() const
	{return {-x, -y, -z, -w};}

	forceinline Quaternion operator*(T rhs) const
	{return {x*rhs, y*rhs, z*rhs, w*rhs};}

	forceinline Quaternion operator/(T rhs) const
	{return {x/rhs, y/rhs, z/rhs, w/rhs};}

	forceinline Quaternion operator+(const Quaternion& rhs) const
	{return {x+rhs.x, y+rhs.y, z+rhs.z, w+rhs.w};}

	forceinline Quaternion operator-(const Quaternion& rhs) const
	{return {x-rhs.x, y-rhs.y, z-rhs.z, w-rhs.w};}

	constexpr Quaternion operator*(const Quaternion& rhs) const
	{
		return {y*rhs.z - z*rhs.y + w*rhs.x + x*rhs.w,
		        z*rhs.x - x*rhs.z + w*rhs.y + y*rhs.w,
		        x*rhs.y - y*rhs.x + w*rhs.z + z*rhs.w,
		        w*rhs.w - x*rhs.x - y*rhs.y - z*rhs.z};
	}

	forceinline Quaternion& operator+=(const Quaternion& q) {xyzw += q.xyzw; return *this;}
	forceinline Quaternion& operator-=(const Quaternion& q) {xyzw -= q.xyzw; return *this;}
	forceinline Quaternion& operator*=(T rhs) {xyzw *= rhs; return *this;}
	forceinline Quaternion& operator/=(T rhs) {xyzw /= rhs; return *this;}
	forceinline Quaternion& operator*=(const Quaternion& q) {return *this = *this*q;}

	//! Поворот вектора v кватернионом.
	INTRA_CONSTEXPR2 Vector3<T> operator*(const Vector3<T>& v) const
	{
#if INTRA_DISABLED
		const Quaternion p = Conjugate(*this) * Quaternion<T>(v) * *this;
		return {p.x, p.y, p.z};
#endif
		const Vector3<T> t = T(2.0)*Cross(v, xyz);
		return v + w*t + Cross(t, xyz);
	}

	constexpr forceinline bool operator==(const Quaternion& rhs) const {return xyzw == rhs.xyzw;}
	constexpr forceinline bool operator!=(const Quaternion& rhs) const {return !operator==(rhs);}

	static INTRA_MATH_CONSTEXPR2 Quaternion RotationEulerRad(T yaw, T pitch, T roll)
	{
		const T cx = T(Cos(yaw/2));
		const T cy = T(Cos(pitch/2));
		const T cz = T(Cos(roll/2));

		const T sx = -T(Sin(yaw/2));
		const T sy = -T(Sin(pitch/2));
		const T sz = -T(Sin(roll/2));

		const T cc = cx*cz;
		const T cs = cx*sz;
		const T sc = sx*cz;
		const T ss = sx*sz;

		return {cy*sc-sy*cs, cy*ss+sy*cc, cy*cs-sy*sc, cy*cc+sy*ss};
	}

	static INTRA_MATH_CONSTEXPR Quaternion FromAngleAndAxis(T angle, const Vector3<T>& axis)
	{return {axis.x*Sin(angle/2), T(Cos(angle/2))};}

	static Quaternion RotationEulerDeg(T yaw, T pitch, T roll)
	{return RotationEulerRad(T(yaw*(PI/180)), T(pitch*(PI/180)), T(roll*(PI/180)));}

	//Ещё не протестировано
	static Quaternion RotationBetweenVectors(Vector3<T> start, Vector3<T> dest)
	{
		start = Normalize(start);
		dest = Normalize(dest);

		const T cosTheta = Dot(start, dest);
		if(cosTheta < T(-1 + 0.0001))
		{
			const Vector3<T> rotationAxis = {-start.y, start.x, 0}; //Cross(Vector3<T>(0, 0, 1), start);
			T l = LengthSqr(rotationAxis.xy);
			if(l<0.0001)
			{
				rotationAxis = {0, -start.z, start.y};// Cross(Vector3<T>(1, 0, 0), start);
				l = LengthSqr(rotationAxis.yz);
			}
			return FromAngleAndAxis(T(PI), rotationAxis/Sqrt(l));
		}

		const Vector3<T> rotationAxis = Cross(start, dest);

		const T s = T(Sqrt((1 + cosTheta)*2));
		return {s/2,
		        rotationAxis.x/s,
		        rotationAxis.y/s,
		        rotationAxis.z/s};
	}

	//Ещё не протестировано
	static Quaternion CreateLookAt(const Vector3<T>& eye, const Vector3<T>& center, const Vector3<T>& up)
	{
		auto delta = center - eye;
		auto rot1 = RotationBetweenVectors(Vector3<T>(0,0,1), delta);
		auto right = Cross(delta, up);
		auto desiredUp = Cross(right, delta);
		auto newUp = rot1*Vector3<T>(0,1,0);
		auto rot2 = RotationBetweenVectors(newUp, desiredUp);
		return rot2*rot1;
	}

	Vector3<T> GetRightVectorNorm() const
	{return {1-2*(y*y+z*z), 2*(x*y-w*z), 2*(x*z+w*y)};}

	Vector3<T> GetUpVectorNorm() const
	{return {2*(x*y+w*z), 1-2*(x*x+z*z), 2*(y*z-w*x)};}

	Vector3<T> GetForwardVectorNorm() const
	{return {2*(x*z-w*y), 2*(y*z+w*x), 1-2*(x*x+y*y)};}

	Vector3<T> GetRightVector() const
	{
		const T l = LengthSqr(xyzw);
		return {l-2*(y*y+z*z), 2*(x*y-w*z), 2*(x*z+w*y)};
	}

	Vector3<T> GetUpVector() const
	{
		const T l = LengthSqr(xyzw);
		return {2*(x*y+w*z), l-2*(x*x+z*z), 2*(y*z-w*x)};
	}

	Vector3<T> GetForwardVector() const
	{
		const T l = LengthSqr(xyzw);
		return {2*(x*z-w*y), 2*(y*z+w*x), l-2*(x*x+y*y)};
	}

	explicit operator Matrix3<T>() const
	{
		const T l = LengthSqr(xyzw);
		return Matrix3<T>(l-2*(y*y+z*z), 2*(x*y-w*z), 2*(x*z+w*y),
		                  2*(x*y+w*z), l-2*(x*x+z*z), 2*(y*z-w*x),
		                  2*(x*z-w*y), 2*(y*z+w*x), l-2*(x*x+y*y));
	}

	Matrix3<T> ToMatrix3Norm() const
	{
		return Matrix3<T>(1-2*(y*y+z*z), 2*(x*y-w*z), 2*(x*z+w*y),
		                  2*(x*y+w*z), 1-2*(x*x+z*z), 2*(y*z-w*x),
		                  2*(x*z-w*y), 2*(y*z+w*x), 1-2*(x*x+y*y));
	}

	explicit operator Matrix4<T>() const
	{
		const T l = LengthSqr(xyzw);
		return Matrix4<T>(l-2*(y*y+z*z), 2*(x*y-w*z), 2*(x*z+w*y), 0,
		                  2*(x*y+w*z), l-2*(x*x+z*z), 2*(y*z-w*x), 0,
		                  2*(x*z-w*y), 2*(y*z+w*x), l-2*(x*x+y*y), 0,
			                   0,           0,             0,      1);
	}

	Matrix4<T> ToMatrix4Norm() const
	{
		return Matrix4<T>(1-2*(y*y+z*z), 2*(x*y-w*z), 2*(x*z+w*y), 0,
		                  2*(x*y+w*z), 1-2*(x*x+z*z), 2*(y*z-w*x), 0,
		                  2*(x*z-w*y), 2*(y*z+w*x), 1-2*(x*x+y*y), 0,
		                       0,             0,          0,       1);
	}

	static const Quaternion<T> Identity;
};

template<typename T> const Quaternion<T> Quaternion<T>::Identity(0, 0, 0, 1);

template<typename T> Quaternion<T> operator*(T lhs, const Quaternion<T>& rhs) {return rhs*lhs;}

//Порядок аналогичен умножению вектора на матрицу
template<typename T> Vector3<T> operator*(const Vector3<T>& v, const Quaternion<T>& rhs)
{
#if INTRA_DISABLED
	Quaternion p = rhs * Quaternion<T>(v) * Conjugate(rhs);
	return {p.x, p.y, p.z};
#endif
	Vector3<T> t = T(2.0)*Cross(rhs.xyz, v);
	return v + rhs.w*t + Cross(rhs.xyz, t);
}

template<typename T> forceinline T Length(const Quaternion<T>& q) {return Length(q.xyzw);}
template<typename T> forceinline T LengthSqr(const Quaternion<T>& q) {return LengthSqr(q.xyzw);}
template<typename T> forceinline Quaternion<T> Normalize(const Quaternion<T>& q) {return Normalize(q.xyzw);}
template<typename T> forceinline Quaternion<T> Conjugate(const Quaternion<T>& q) {return Quaternion<T>(-q.x, -q.y, -q.z, q.w);}

template<typename T> Quaternion<T> Inverse(const Quaternion<T>& q)
{
	const T l = Length(q);
	return Quaternion<T>(q.x/-l, q.y/-l, q.z/-l, q.w/l);
}

template<typename T> Quaternion<T>::Quaternion(const Matrix3<T>& mat)
{
	const T trace = mat[0][0]+mat[1][1]+mat[2][2];
	if(trace>0)
	{
		T s = T(Sqrt(trace+1));
		w = s/2;
		s = T(0.5)/s;
		x = (mat[2][1] - mat[1][2])*s;
		y = (mat[0][2] - mat[2][0])*s;
		z = (mat[1][0] - mat[0][1])*s;
		return;
	}

	uint i = uint(mat[1][1] > mat[0][0]);
	if(mat[2][2]>mat[i][i]) i=2;

	uint j = (i+1)%3, k = (j+1)%3;
	T s = T(Sqrt(mat[i][i] - mat[j][j] - mat[k][k] + 1));
	T* q = &x;
	q[i] = s/2;
	if(s!=0) s = 0.5f/s;
	q[3] = (mat[k][j] - mat[j][k])*s;
	q[j] = (mat[j][i] + mat[i][j])*s;
	q[k] = (mat[k][i] + mat[i][k])*s;
}




template<typename T> Quaternion<T> slerp(const Quaternion<T>& q1, const Quaternion<T>& q2, T t)
{
	Quaternion<T> z = q2;
	T cosTheta = Dot(q1.xyzw, q2.xyzw);
	if(cosTheta<0) z = -q2, cosTheta = -cosTheta;
	if(cosTheta>0.99999f) return LinearMix(q1, q2, t);
	const T angle = T(Acos(cosTheta));
	return ( T(Sin((T(1)-t)*angle))*q1 + T(Sin(t*angle))*z ) / T(Sin(angle));
}


typedef Quaternion<float> Quat;
typedef Quaternion<double> DQuat;


struct QuatTransform
{
	Vec3 pos;
	Quat orient;

	//! Суперпозиция двух преобразований
	//TODO: Ещё не протестировано!
	QuatTransform operator*(const QuatTransform& rhs) const
	{
		return {orient*rhs.pos + pos, orient*rhs.orient};
	}

	//! Преобразовать вектор.
	Vec3 operator*(const Vec3& v) const {return orient*v+pos;}

	//! Обратное преобразование вектора
	Vec3 invTransform(const Vec3& v) const {return Inverse(orient)*(v-pos);}

	//! То же самое, но работает корректно только при отсутствии масштабирования в orient.
	Vec3 fastInvTransform(const Vec3& v) const {return Conjugate(orient)*(v-pos);}
};

inline QuatTransform Inverse(const QuatTransform& qt)
{return {-qt.pos, Inverse(qt.orient)};}

}

INTRA_END
