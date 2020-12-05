#pragma once

#include "IntraX/Math/Vector2.h"

INTRA_BEGIN
namespace Math {

template<typename T> struct Ellipse
{
	Vector2<T> Center;
	T a, b;


};

template<typename T> T Distance(Ellipse<T> ellipse, Vector2<T> pt)
{
	pt -= ellipse.Center;
	const Vector2<T> ptAbs = Abs(pt);
	if(ptAbs.x > ptAbs.y)
	{
		Swap(pt.x, pt.y);
		Swap(ellipse.a, ellipse.b);
		//swap ptAbs???
	}
	
	const T l = ellipse.b*ellipse.b - ellipse.a*ellipse.a;
	const T m = ellipse.a*ptAbs.x/l, m2=m*m;
	const T n = ellipse.b*ptAbs.y/l;
	const T n2 = n*n;
	const T c = (m2+n2-1)/3;
	const T c3 = c*c*c;
	const T q = c3+m2*n2*2;
	const T d = c3+m2*n2;
	const T g = m+m*n2;

	T co;
	if(d<0)
	{
		const T p = Acos(q/c3)/3;
		const T s = Cos(p);
		const T t = Sin(p)*Sqrt(3.0);
		const T rx = Sqrt(-c*(s+t+2)+m2);
		const T ry = Sqrt(-c*(s-t+2)+m2);
		co = (ry+Sign(l)*rx+Abs(g)/(rx*ry)-m)/2;
	}
	else
	{
		const T h = 2*m*n*Sqrt(d);
		const T s = Sign(q+h)*Pow(Abs(q+h), T(0.333333333333333));
		const T u = Sign(q-h)*Pow(Abs(q-h), T(0.333333333333333));
		const T rx = -s-u-c*4+2*m2;
		const T ry = (s-u)*T(1.73205080757);
		const T rm = Sqrt(rx*rx+ry*ry);
		const T p = ry/Sqrt(rm-rx);
		co = (p+2*g/rm-m)/2;
	}

	const T si = Sqrt(T(1)-co*co);
	const Vector2<T> closestPoint = Vector2<T>(ellipse.a*co, ellipse.b*si);
	return Distance(closestPoint, ptAbs)*Sign(ptAbs.y-closestPoint.y);
}

}}
