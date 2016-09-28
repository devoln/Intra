#pragma once

#include "CompilerSpecific/InitializerList.h"
#include "Math/Vector.h"
#include "Math/Matrix.h"

namespace Intra { namespace Math {

template<typename T> struct Ray
{
	Ray() = default;
	Ray(const Vector3<T>& orig, const Vector3<T>& direction) {origin=orig; dir=direction;}

	Vector3<T> origin, dir;
};

typedef Ray<float> RayF;

template<typename T> Ray<T> operator*(const Mat4& mat, const Ray<T>& r)
{
	return Ray<T>(Vec3(mat*Vec4(r.origin, 1.0f)), Mat3(mat)*r.dir);
}

template<typename T> struct Line
{
	Vector2<T> Normal;
	T C;

	Line(Vector2<T> p1, Vector2<T> p2)
	{
		Normal.x = (p2.y-p1.y);
		Normal.y = (p1.x-p2.x);
		C = p1.x*(p1.y-p2.y)+p1.y*(p2.x-p1.x);
		auto nl = Length(Normal);
		Normal /= nl;
		C /= nl;
	}

	//Функция предполагает, что уравнение нормировано
	bool Contains(Vector2<T> pt)
	{
		return Abs(Dot(Normal, pt)+C)<=0.0001;
	}
};

template<typename T> struct LineSegment
{
	Vector3<T> A, B;

	Vector3<T> Midpoint() const {return (A+B)/2;}
};

template<typename T> T Length(const LineSegment<T>& l) {return Length(l.B-l.A);}

template<typename T> struct Sphere;

template<typename T> struct AABB // "бокс", лежащий вдоль осей координат
{
	Vector3<T> min, max;

	AABB() = default;
	AABB(null_t) {min=max=Vector3<T>(T(0));}
	AABB(Vector3<T> _min, Vector3<T> _max): min(_min), max(_max) {}

	static AABB<T> FromCenterAndExtents(const Vector3<T>& center, const Vector3<T>& extents) {return {center-extents, center+extents};}
	static AABB<T> FromCenterAndSize(const Vector3<T>& center, const Vector3<T>& size) {return FromCenterAndExtents(center, size/2);}

	Vector3<T> Size() const {return max-min;}
	Vector3<T> Extents() const {return Size()/T(2);}
	Vector3<T> Center() const {return min+Extents();}
	T Diagonal() const {return Length(Size());}

	T SqrDistance(const Vector3<T>& pt, Vector3<T>* nearestPoint=null) const;
	T Distance(const Vec3& pt, Vector3<T>* nearestPoint=null) const {return T(Sqrt(SqrDistance(pt, nearestPoint)));}

	//Диаметр вписанной сферы
	T MinSizeAxis(int* axis=null) const
	{
		auto size = Size();
		T minSize = size.x;
		if(axis!=null) *axis=0;
		if(size.y<minSize)
		{
			minSize = size.y;
			if(axis!=null) *axis=1;
		}
		if(size.z<minSize)
		{
			minSize = size.z;
			if(axis!=null) *axis=2;
		}
		return minSize;
	}

	//Радиус вписанной сферы
	T MinExtentAxis(int* axis=null)
	{
		return MinSizeAxis(axis)/T(2);
	}

	//Диаметр описанной сферы
	T MaxSizeAxis(size_t* oAxis=null) const
	{
		auto size = Size();
		T maxSize = size.x;
		if(oAxis!=null) *oAxis = 0;
		if(size.y>maxSize)
		{
			maxSize=size.y;
			if(oAxis!=null) *oAxis=1;
		}
		if(size.z>maxSize)
		{
			maxSize=size.z;
			if(oAxis!=null) *oAxis=2;
		}
		return maxSize;
	}

	//Радиус описанной сферы
	T MaxExtentAxis(int* oAxis=null)
	{
		return MaxSizeAxis(oAxis)/T(2);
	}

	void AddTriangle(const Triangle<T>& tri) {for(int i=0; i<3; i++) AddPoint(tri.vertices[i]);}

	void AddPoint(Vector3<T> pt)
	{
		min = Math::Min(min, pt);
		max = Math::Max(max, pt);
	}

	void AddAABB(const AABB<T>& b)
	{
		AddPoint(b.min);
		AddPoint(b.max);
	}

	bool Contains(const Vector3<T>& pt) const
	{
		return pt.x>=min.x && pt.x<=max.x &&
			pt.y>=min.y && pt.y<=max.y &&
			pt.z>=min.z && pt.z<=max.z;
	}

	bool Contains(const AABB<T>& aabb) const {return Contains(aabb.min) && Contains(aabb.max);}
	bool Contains(const Sphere<T>& sph) const;

	bool OverlapsLineSegment(const LineSegment<T>& ls) const;

	bool CheckInvRayIntersection(const Ray<T>& invray, T rayLength=T(Infinity), T* near=null, T* far=null) const
	{
		using Math::Max;
		using Math::Min;

		const Vector3<T> t1 = (this->min - invray.origin)*invray.dir;
		const Vector3<T> t2 = (this->max - invray.origin)*invray.dir;

		T tmin = Min(t1.x, t2.x);
		T tmax = Max(t1.x, t2.x);
		tmin = Max(tmin, Min(t1.y, t2.y));
		tmax = Min(tmax, Max(t1.y, t2.y));
		tmin = Max(tmin, Min(t1.z, t2.z));
		tmax = Min(tmax, Max(t1.z, t2.z));

		if(near!=null) *near = tmin;
		if(far!=null) *far = tmax;

		return tmax>=Max(T(0), tmin) && tmin<rayLength;
	}

	//Работает только для плавающей запятой!
	bool CheckRayIntersection(const Ray<T>& ray, T rayLength=T(Infinity), T* near=null, T* far=null) const
	{
		return CheckInvRayIntersection({ray.origin, Vector3<T>(T(1))/ray.dir}, rayLength, near, far);
	}

	

	T Volume() const
	{
		auto sz=Size();
		return sz.x*sz.y*sz.z;
	}

	bool operator>(const AABB& rhs) const //Сравнить размеры двух "боксов"
	{
		return Volume()>rhs.Volume();
	}
};



template<typename T> struct OBB
{
	Vector3<T> position;
	Matrix3<T> transform;

	OBB(const Vector3<T>& pos={0,0,0}, const Vector3<T>& size={0,0,0}):
		position(pos), transform(Mat3Scaling(size)) {}

	OBB(const AABB<T>& aabb):
		position(aabb.Center()), transform(Mat3Scaling(aabb.Size())) {}

	OBB(const AABB<T>& aabb, const Mat3& world):
		position(aabb.Center()), transform(world*Mat3Scaling(aabb.Size())) {}

	Vector3<T> GetPoint(bool positiveX, bool positiveY, bool positiveZ) const
	{
		const Vector3<T> localUnitPoint = {T(positiveX)-T(0.5), T(positiveY)-T(0.5), T(positiveZ)-T(0.5)};
		return Vector3<T>(GetFullTransform()*Vector4<T>(localUnitPoint, 1));
	}

	Matrix4<T> GetFullTransform() const {return Matrix4<T>(transform, position);}

	AABB<T> BoundingAABB() const
	{
		AABB<T> result;
		result.max=result.min=position;
		for(bool b1: {false, true})
			for(bool b2: {false, true})
				for(bool b3: {false, true}) //Перебираем все точки параллелепипеда
					result.AddPoint(GetPoint(b1, b2, b3));
		return result;
	}

	void AsRotatedAABB(AABB<T>* aabb, Matrix3<T>* rotation)
	{
		Matrix3<T> rot = transform.GetRotation3();
		if(rotation!=null) *rotation = rot;
		if(aabb!=null) *aabb = AABB<T>::FromCenterAndSize(position, transform.GetScaling3());
	}

	Sphere<T> BoundingSphere() const;

	bool CheckRayIntersection(const Ray<T>& ray) const
	{
		const AABB<T> localAABB = {Vector3<T>(-0.5), Vector3<T>(0.5)};
		const Ray<T> localRay = Inverse(GetFullTransform())*ray;
		return localAABB.CheckRayIntersection(localRay);
	}

	bool OverlapsAABB(const AABB<T>& aabb) const;
	bool OverlapsOBB(const OBB<T>& obb) const;
	bool OverlapsSphere(const Sphere<T> &sphere) const;
	bool OverlapsLineSegment(const LineSegment<T>& ls) const;
	bool OverlapsTriangle(const Triangle<T>& tri) const;
};

template<typename T> OBB<T> operator*(const Mat4& m, const OBB<T>& obb)
{
	OBB<T> result;
	result.position = Vector3<T>(m*Vector4<T>(obb.position, 1));
	result.transform = Matrix3<T>(m)*obb.transform;
	return result;
}




template<typename T> struct Sphere
{
	Vector3<T> Center;
	T Radius;

	Sphere() = default;
	Sphere(null_t): Center(0,0,0), Radius(0) {}
	Sphere(const Vector3<T>& c, T r): Center(c), Radius(r) {}

	void AddPoint(const Vector3<T>& p)
	{
		const T distSqr = LengthSqr(p-Center);
		if(distSqr>Radius*Radius) Radius = T(Sqrt(distSqr));
	}

	void AddSphere(const Sphere<T>& s)
	{
		const T dist = Distance(s.Center, Center)+s.Radius;
		if(dist>Radius) Radius = dist;
	}

	//! Проверка пересечения луча со сферой
	//! \param ray нормированный луч
	//! \returns Произошло ли пересечение (true или false)
	bool CheckRayIntersection(const Ray<T>& ray) const
	{
		const Vector3<T> l = Center-ray.origin;
		const T d = Dot(l, ray.dir);
		const T squaredLen = LengthSqr(l);
		const T squaredRadius = Radius*Radius;
		if(d<0 && squaredLen>squaredRadius) return false;
		return squaredLen-d*d <= squaredRadius;
	}

	//! Проверка пересечения луча со сферой с нахождением точки пересечения
	//! \param ray нормированный луч
	//! \returns Параметр t для нахождения точки пересечения ray.origin + t*ray.dir. Если пересечения не было, то NaN для floating point типов или 0 для integer типов
	T GetRayIntersectionPoint(const Ray<T>& ray) const
	{
		Vector3<T> l = Center-ray.origin;
		const T d = Dot(l, ray.dir);
		const T D2 = Radius*Radius - LengthSqr(l) + d*d;
		if(D2<0) return NaN;
		const T D = Sqrt(D2);
		const T t0 = d-D;
		if(t0>0) return t0;
		const T t1 = d+D;
		if(t1>0) return t1;
		return NaN;
	}

	AABB<T> GetAABB() const {return {Center-Vector3<T>(Radius), Center+Vector3<T>(Radius)};}
};

template<typename T> Sphere<T> operator*(const Matrix4<T>& m, const Sphere<T>& s)
{
	Sphere<T> result;
	result.Center = Vector3<T>(m*Vector4<T>(s.Center, 1));
	auto scale = m.GetScaling();
	result.Radius = Max(Max(scale.x, scale.y), scale.z)*s.Radius;
	return result;
}

typedef Sphere<float> SphereF;

template<typename T> Sphere<T> OBB<T>::BoundingSphere() const
{
	T maxdiagSqr = 0;
	for(bool b1: {false, true}) for(bool b2: {false, true})
		maxdiagSqr = Max(maxdiagSqr, Distance(GetPoint(b1, b2, false), GetPoint(!b1, !b2, true)));
	return Sphere<T>(position, Sqrt(maxdiagSqr)/2);
}


template<typename T> bool AABB<T>::Contains(const Sphere<T>& sph) const {return Contains(sph.GetAABB());}

template<typename T> struct Plane
{
	Plane() = default;

	Plane(const Vector3<T>& point1, const Vector3<T>& point2, const Vector3<T>& point3):
		Normal( Normalize(Cross(point2-point1, point3-point1)) ),
		D( -Dot(point1, Normal) ) {}

	Plane(const Vector3<T>& planeNormal, const Vector3<T>& pointOnPlane):
		Normal(planeNormal), D(-Dot(pointOnPlane, Normal)) {}

	Plane(const Vector3<T>& planeNormal, T distanceFromZero):
		Normal(planeNormal), D(distanceFromZero) {}

	Plane(const Vector4<T>& asVec4):
		Normal(asVec4.xyz), D(asVec4.w) {}

	Plane(T a, T b, T c, T d):
		Normal(a, b, c), D(d) {}

	short ClassifySphere(Sphere<T> sphere) const
	{
		const T dist = Dot(sphere.Center, Normal)+D;
		if(Abs(dist)<sphere.Radius) return 0;
		if(dist>=sphere.Radius) return 1;
		return -1;
	}

	Vector3<T> GetIntersectionWithLine(const Vector3<T>& lineA, const Vector3<T>& lineB) const
	{
		const Vector3<T> dir = Normalize(lineB-lineA);
		const T denominator = Dot(Normal, dir); //Косинус угла между прямой и плоскостью
		if(Abs(denominator)<=T(0.00001)) return lineA;  //Прямая лежит на плоскости
		return lineA+dir*((lineA.DistanceToPlane(*this)+D)/denominator);
	}

	bool IsIntersectedByLine(Vector3<T> lineA, Vector3<T> lineB) const //Определить, пересекается ли прямая с плоскостью
	{
		return (Dot(Normal, lineA)<-D) == (Dot(Normal, lineB)<-D);
	}

	Vector4<T>& AsVec4() {return *reinterpret_cast<Vector4<T>*>(this);}
	const Vector4<T>& AsVec4() const {return *reinterpret_cast<const Vector4<T>*>(this);}

	Vector3<T> Normal;
	T D;
};

typedef Plane<float> PlaneF;

template<typename T> Plane<T> Normalize(const Plane<T>& p)
{
	auto l = Length(p.Normal);
	return Plane<T>(p.Normal/l, p.D/l);
}

//Найти смещение центра сферы при столкновении с плоскостью
template<typename T> Vector3<T> GetCollisionOffset(const Plane<T>& plane, T Radius, T distanceOfSphereCenter)
{
	return plane.Normal*(Sign(distanceOfSphereCenter)*Radius-distanceOfSphereCenter);
}


template<typename T> struct Triangle
{
	Plane<T> GetPlane() const {return Plane<T>(vertices[0], vertices[1], vertices[2]);}

	T Area() const
	{
		T a = Length(vertices[0]-vertices[1]);
		T b = Length(vertices[0]-vertices[2]);
		T c = Length(vertices[2]-vertices[1]);
		T p = (a+b+c)/2;

		return Sqrt(p*(p-a)*(p-b)*(p-c));
	}

	//Проверка пересечения сферы с рёбрами треугольника
	bool EdgeSphereCollision(Sphere<T> sph) const
	{
		return (Distance(sph.Center, sph.Center.ClosestPointOnLine(vertices[0], vertices[1]))<sph.Radius ||
			    Distance(sph.Center, sph.Center.ClosestPointOnLine(vertices[1], vertices[2]))<sph.Radius ||
			    Distance(sph.Center, sph.Center.ClosestPointOnLine(vertices[2], vertices[0]))<sph.Radius);
	}

	Vector3<T> CheckCollisionWithSphere(Sphere<T> sphere) const
	{
		const Plane<T> plane = GetPlane();

		//Если плоскость не пересекает сферу, то сфера не может пересекать треугольник
		if(plane.ClassifySphere(sphere)!=0) return Vector3<T>(0);

		const T distance = Dot(sphere.Center, plane.Normal)+plane.D;
		if((sphere.Center-plane.Normal*distance).IsInsideTriangle(*this) ||
			EdgeSphereCollision(Sphere<T>(sphere.Center, sphere.Radius*0.3f)))
				return GetCollisionOffset(plane, sphere.Radius, distance);
		return Vector3<T>(0);
	}

	//! Проверка пересечения прямой с треугольником
	bool IsIntersectedByLine(Vector3<T> line[2]) const
	{
		const Plane<T> plane = GetPlane();
		return (plane.IsIntersectedByLine(line) &&
			plane.GetIntersectionWithLine(line).IsInsideTriangle(*this));
	}

	//! Проверить треугольник на пересечение с лучом
	// \param[out] oPlaneIntersectionPoint Точка пересечения с треугольником, если такое пересечение есть. Иначе точка пересечения с плоскостью треугольника. Если пересечения с плоскостью нет, то {0,0,0}
	bool CheckRayIntersection(const Ray<T>& ray, Vector3<T>* oPlaneIntersectionPoint=null)
	{
		const Vector3<T> u = vertices[1]-vertices[0], v = vertices[2]-vertices[0];
		const Vector3<T> n = Cross(u, v);

		Vector3<T> w0 = ray.origin - vertices[0];
		const T a = -Dot(n, w0), b = Dot(n, ray.dir);

		if(a*b<0 || b==0) //Луч уходит от плоскости треугольника или параллелен ей
		{
			if(oPlaneIntersectionPoint) *oPlaneIntersectionPoint = Vector3<T>(0);
			return false;
		}

		const T r = a/b;
		Vector3<T> planeIntersectionPoint = ray.origin + ray.dir*r;
		if(oPlaneIntersectionPoint) *oPlaneIntersectionPoint = planeIntersectionPoint;

		const T uu = Dot(u, u), uv = Dot(u, v), vv = Dot(v, v);
		const Vector3<T> w = planeIntersectionPoint-vertices[0];
		const T wu = Dot(w, u), wv = Dot(w, v);
		const T D = uv*uv - uu*vv;

		T s = (uv*wv - vv*wu)/D;
		T t = (uv*wu - uu*wv)/D;
		bool pointIsInTriangle = s>=0 && s<=D && t>=0 && s+t<=D;
		return pointIsInTriangle;
	}

	Vector3<T> vertices[3];
};

template<typename T> Triangle<T> operator*(const Triangle<T>& lhs, const Matrix4<T>& m)
{
	Triangle<T> result;
	for(int i=0; i<3; i++)
		result.vertices[i] = (Vector4<T>(lhs.vertices[i], 1)*m).xyz;
	return result;
}

template<typename T> Triangle<T> operator*(const Matrix4<T>& m, const Triangle<T>& rhs)
{
	Triangle<T> result;
	for(int i=0; i<3; i++)
		result.vertices[i] = (m*Vector4<T>(rhs.vertices[i], 1)).xyz;
	return result;
}

enum class CollisionSide: byte {Outside, Inside, Intersect};

template<typename T> struct Frustum
{
	enum Plane: byte {Right, Left, Bottom, Top, Far, Near};

	Frustum() = default;
	Frustum(const Matrix4<T>& viewProjection);
	Frustum(const Matrix4<T>& view, const Matrix4<T>& proj): Frustum(proj*view) {}

	CollisionSide SphereTest(const Sphere<T>& refSphere) const
	{
		for(int i=0; i<6; i++)
		{
			T dist = Dot(planes[i].Normal, refSphere.Center)+planes[i].D;
			if(dist<-refSphere.Radius) return CollisionSide::Outside;
			if(Abs(dist)<refSphere.Radius) return CollisionSide::Intersect;
		}
		return CollisionSide::Inside;
	}

    bool Contains(const Sphere<T>& sphere) const
	{
		for(byte p=0; p<6; p++)
			if(Dot(sphere.Center, planes[p].Normal) + planes[p].D + sphere.Radius <= 0) return false;
		return true;
		//return SphereTest(sphere)!=CollisionSide::Outside;
	}

	bool Contains(const Vector3<T>& point) const {return Contains(Sphere<T>(point, 0));}

	bool Contains(const AABB<T>& box) const
	{
		for(int i=0; i<6; i++)
		{
			if(Dot(planes[i].Normal, box.min) >= -planes[i].D) continue;
			if(Dot(planes[i].Normal, Vector3<T>(box.max.x, box.min.y, box.min.z)) >= -planes[i].D) continue;
			if(Dot(planes[i].Normal, Vector3<T>(box.min.x, box.max.y, box.min.z)) >= -planes[i].D) continue;
			if(Dot(planes[i].Normal, Vector3<T>(box.max.x, box.max.y, box.min.z)) >= -planes[i].D) continue;
			if(Dot(planes[i].Normal, Vector3<T>(box.min.x, box.min.y, box.max.z)) >= -planes[i].D) continue;
			if(Dot(planes[i].Normal, Vector3<T>(box.max.x, box.min.y, box.max.z)) >= -planes[i].D) continue;
			if(Dot(planes[i].Normal, Vector3<T>(box.min.x, box.max.y, box.max.z)) >= -planes[i].D) continue;
			if(Dot(planes[i].Normal, box.max) >= -planes[i].D) continue;
			return false;
		}
		return true;
	}

	bool Contains(const OBB<T>& box) const
	{
		const auto localAabb = AABB<T>({-0.5, -0.5, -0.5}, {0.5, 0.5, 0.5});
		Frustum<T> localFrust;
		auto boxTransform = box.GetFullTransform();
		for(ushort i=0; i<6; i++)
			localFrust.planes[i] = Normalize(planes[i].AsVec4()*boxTransform);
		return localFrust.Contains(localAabb);
	}

	const Math::Plane<T>& operator[](byte index) const {return planes[index];}

	Math::Plane<T> planes[6];
};


template<typename T> struct Ellipse
{
	Vector2<T> Center;
	T a, b;


};

template<typename T> Frustum<T>::Frustum(const Matrix4<T>& vp)
{
	//right
	planes[0].Normal = {vp[0][3]-vp[0][0], vp[1][3]-vp[1][0], vp[2][3]-vp[2][0]};
	planes[0].D = vp[3][3]-vp[3][0];

	//left
	planes[1].Normal = {vp[0][3]+vp[0][0], vp[1][3]+vp[1][0], vp[2][3]+vp[2][0]};
	planes[1].D = vp[3][3]+vp[3][0];

	//bottom
	planes[2].Normal = {vp[0][3]+vp[0][1], vp[1][3]+vp[1][1], vp[2][3]+vp[2][1]};
	planes[2].D = vp[3][3]+vp[3][1];

	//top
	planes[3].Normal = {vp[0][3]-vp[0][1], vp[1][3]-vp[1][1], vp[2][3]-vp[2][1]};
	planes[3].D = vp[3][3]-vp[3][1];

	//far
	planes[4].Normal = {vp[0][3]-vp[0][2], vp[1][3]-vp[1][2], vp[2][3]-vp[2][2]};
	planes[4].D = vp[3][3]-vp[3][2];

	//near
	planes[5].Normal = {vp[0][3]+vp[0][2], vp[1][3]+vp[1][2], vp[2][3]+vp[2][2]};
	planes[5].D = vp[3][3]+vp[3][2];

	for(auto& p: planes) p = Normalize(p);
}


template<typename T> Vector3<T> CheckBoxSphereCollision(const OBB<T>& box, const Sphere<T>& sph)
{
	/*auto boxTransform=box.GetFullTransform();
	const auto boxSizes=box.transform.GetScaling3();
	boxTransform=(Mat4::Scaling(Vec3(1)/boxSizes)*boxTransform);
	//if(Distance(box.position, sph.Center)>=sph.Radius+Sqrt(T(3))) return {0};

	const Vector4<T> localCenter=Inverse(boxTransform)*Vector4<T>(sph.Center, 1.0);
	auto dist=-Sqr(sph.Radius);
	for(ushort i=0; i<3; i++)
	{
		if(localCenter[i]<-boxSizes[i]*0.5f) dist+=Sqr(localCenter[i]+boxSizes[i]*0.5f);
		else if(localCenter[i]>boxSizes[i]*0.5f) dist+=Sqr(localCenter[i]-boxSizes[i]*0.5f);
	}
	if(dist>=0) return {0};

	Vec3 result={0};
	for(ushort i=0; i<3; i++)
	{
		if(localCenter[i]<-boxSizes[i]*0.5f) result[i]=dist;
		else if(localCenter[i]>boxSizes[i]*0.5f) result[i]=-dist;
	}
    return Mat3(boxTransform)*result;*/
	return CheckBoxSphereCollision(box.transform.GetScaling3(), box.GetFullTransform(), sph);
}

template<typename T> Vector3<T> CheckBoxSphereCollision(const Vector3<T>& boxSizes, const Matrix4<T>& transform, const Sphere<T>& sph)
{
	T halfDiagSqr = LengthSqr(boxSizes)/4;
	T radiusSqr = sph.Radius*sph.Radius;
	if(DistanceSqr(Vec3(transform.Row(3)), sph.Center) >= radiusSqr+2*sph.Radius*Sqrt(halfDiagSqr)+halfDiagSqr)
		return {0,0,0};

	const Vector4<T> localCenter = Inverse(transform)*Vector4<T>(sph.Center, 1.0);
	auto dist = -radiusSqr;
	for(ushort i=0; i<3; i++)
	{
		if(localCenter[i]<boxSizes[i]/-2) dist += Sqr(localCenter[i]+boxSizes[i]/2);
		else if(localCenter[i]>boxSizes[i]/2) dist += Sqr(localCenter[i]-boxSizes[i]/2);
	}

	if(dist>=0) return {0,0,0};

	Vector3<T> d = Abs(localCenter.xyz)-boxSizes/2;
	auto ld = Length(Max(d, T(0)));
	T dist1 = Clamp(Max(d.y, d.z), d.x, T(0))+ld;
	dist1 -= sph.Radius;

	Vector3<T> result={0,0,0};
	int c=0;
	for(ushort i=0; i<3; i++)
	{
		if(localCenter[i]<-boxSizes[i]/2)
			result[i] = -sph.Radius+d[i], c++;
		else if(localCenter[i]>boxSizes[i]/2)
			result[i] = sph.Radius-d[i], c++;
	}
	if(c>1)
	{
		result = {0,0,0};
		for(ushort i=0; i<3; i++)
		{
			if(localCenter[i]<-boxSizes[i]/2)
				result[i] = dist;
			else if(localCenter[i]>boxSizes[i]/2)
				result[i] = -dist;
		}
	}

    return Matrix3<T>(transform)*result;
}

template<typename T> Vector3<T> CheckPlaneSphereCollision(const Vector2<T>& planeSizes,
	const Matrix4<T>& planeTransform, const Sphere<T>& sph)
{
	const Vector3<T> localCenter = Vector3<T>(Inverse(planeTransform)*Vector4<T>(sph.Center, 1.0));

	static const Vector3<T> points[4] = {
		{-planeSizes.x/2, -planeSizes.y/2, 0},
		{planeSizes.x/2, -planeSizes.y/2, 0},
		{planeSizes.x/2, planeSizes.y/2, 0},
		{-planeSizes.x/2, planeSizes.y/2, 0}
	};
	const Plane<T> planePlane = {points[0], points[1], points[2]};
	const Sphere<T> localSphere = {localCenter, sph.Radius};

    //Если плоскость не пересекает сферу, то сфера не может пересекать треугольник
	if(planePlane.ClassifySphere(localSphere)!=0) return {0,0,0};


    const T distance = Dot(localCenter, planePlane.Normal)+planePlane.D;
    const Vector3<T> pointOnPlane = localSphere.Center-planePlane.Normal*distance;

    /*if(pointOnPlane.x<-planeSizes.x/2)
    	if(relCenter.DistanceTo(relCenter.ClosestPointOnLine(points[3], points[0]))<sphere.Radius) return Vec3(-1, 0, -1);
    	else return {0};
    if(pointOnPlane.x>size.x/2)
        	if(relCenter.DistanceTo(relCenter.ClosestPointOnLine(points[1], points[2]))<sphere.Radius) return Vec3(1, 0, -1);
        	else return {0};
    if(pointOnPlane.y<-size.y/2)
        	if(relCenter.DistanceTo(relCenter.ClosestPointOnLine(points[0], points[1]))<sphere.Radius) return Vec3(0, -1, -1);
        	else return {0};
    if(pointOnPlane.y>size.y/2)
        	if(relCenter.DistanceTo(relCenter.ClosestPointOnLine(points[3], points[2]))<sphere.Radius) return Vec3(0, 1, -1);
        	else return {0};*/

    if(pointOnPlane.x<-planeSizes.x/2-localSphere.Radius/2 || pointOnPlane.x>planeSizes.x/2+localSphere.Radius/2 ||
    		pointOnPlane.y<-planeSizes.y/2-localSphere.Radius/2 || pointOnPlane.y>planeSizes.y/2+localSphere.Radius/2)
			return {0,0,0};

    const Vector3<T> result = GetCollisionOffset(planePlane, sph.Radius, distance);

    return Matrix3<T>(planeTransform)*result;
}


template<typename T> T Distance(const Vector2<T>& pt, const Ellipse<T>& ellipse)
{
	pt -= ellipse.Center;
    Vector2<T> ptAbs = Abs(pt);
	if(ptAbs.x>ptAbs.y)
	{
		core::swap(pt.x, pt.y);
		core::swap(ellipse.a, ellipse.b);
	}
	
    T l = ellipse.b*ellipse.b - ellipse.a*ellipse.a;
    T m = ellipse.a*ptAbs.x/l, m2=m*m;
    T n = ellipse.b*ptAbs.y/l, n2=n*n;
    T c = (m2+n2-1)/3, c3=c*c*c;
    T q = c3+m2*n2*2;
    T d = c3+m2*n2;
    T g = m+m*n2;

    T co;

    if(d<0)
    {
        T p = Acos(q/c3)/3;
        T s = Cos(p);
        T t = Sin(p)*Sqrt(3.0);
        T rx = Sqrt(-c*(s+t+2)+m2);
        T ry = Sqrt(-c*(s-t+2)+m2);
        co = (ry+Sign(l)*rx+Abs(g)/(rx*ry)-m)/2;
    }
    else
    {
        T h = 2*m*n*Sqrt(d);
        T s = Sign(q+h)*Pow(Abs(q+h), 1.0/3.0);
        T u = Sign(q-h)*Pow(Abs(q-h), 1.0/3.0);
        T rx = -s-u-c*4+2*m2;
        T ry = (s-u)*1.73205080757f;
        T rm = Sqrt(rx*rx+ry*ry);
        T p = ry/Sqrt(rm-rx);
        co = (p+2.0*g/rm-m)/2.0;
    }

    T si = Sqrt(1-co*co);
    Vector2<T> closestPoint = Vector2<T>(ellipse.a*co, ellipse.b*si);
    return Distance(closestPoint, ptAbs)*Sign(ptAbs.y-closestPoint.y);
}

template<typename T, typename U> bool TestIntersection(const T& lhs, const U& rhs)
{
	return TestIntersection(rhs, lhs);
}

template<typename T> bool TestIntersection(const Triangle<T>& tri, const Sphere<T>& sph)
{
	const Plane<T> plane = tri.GetPlane();

	//Если плоскость не пересекает сферу, то сфера не может пересекать треугольник
	if(plane.ClassifySphere(sph)!=0) return false;

	const float distance = Dot(sph.Center, plane.Normal)+plane.D;
	return Vec3(sph.Center-plane.Normal*distance).IsInsideTriangle(tri) ||
		tri.EdgeSphereCollision(Sphere<T>(sph.Center, sph.Radius*0.3f));
}

template<typename T> bool TestIntersection(const Triangle<T>& tri1, const Triangle<T>& tri2)
{
	(void)tri1; (void)tri2;
	INTRA_ASSERT(!"Not implemented!");
	return false;
}


template<typename T> T AABB<T>::SqrDistance(const Vector3<T>& pt, Vector3<T>* nearestPoint) const
{
	const auto boxCenter = Center();
	const auto boxExtents = Extents();

	Vector3<T> closest = boxCenter;

	if(pt.x<min.x) closest.x = min.x;
	else if(pt.x>max.x) closest.x = max.x;
	else closest.x = pt.x;

	if(pt.y<min.y) closest.y = min.y;
	else if(pt.y>max.y) closest.y = max.y;
	else closest.y = pt.y;

	if(pt.z<min.z) closest.z = min.z;
	else if(pt.z>max.z) closest.z = max.z;
	else closest.z = pt.z;

	if(nearestPoint!=null) *nearestPoint=closest;

	return LengthSqr(pt-closest);
}

template<typename T> T DistanceSqr(const Vector3<T>& pt, const AABB<T>& aabb) {return aabb.SqrDistance(pt);}
template<typename T> T Distance(const Vector3<T>& pt, const AABB<T>& aabb) {return T(Sqrt(DistanceSqr(pt, aabb)));}

template<typename T> T DistanceSqr(const Vector3<T>& pt, const OBB<T>& obb)
{
	Matrix3<T> rot;
	Vector3<T> scaling;
	obb.transform.DecomposeTransform3(&rot, &scaling); //Поворот obb вокруг своего центра
	AABB<T> aabb = {scaling/-2, scaling/2}; //AABB с центром в начале координат
	return aabb.SqrDistance( (pt-obb.position)*rot ); //Умножение справа, потому что точка поворачивается вокруг aabb в противоположную повороту obb сторону
}

template<typename T> T Distance(const Vector3<T>& pt, const OBB<T>& obb) {return T(Sqrt(DistanceSqr(pt, obb)));}

template<typename T> bool TestIntersection(const AABB<T>& aabb, const Sphere<T>& sph)
{
	const T sqrDist = aabb.SqrDistance(sph.Center);
	const T sphSqrRadius = sph.Radius*sph.Radius;
	return sqrDist<=sphSqrRadius;
}


}}
