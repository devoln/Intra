#pragma once

#include "Meta/Type.h"
#include "Vector.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif

namespace Intra {

template<typename T, uint DIV> struct FixedPoint
{
	enum: uint {Divisor = DIV};

	static_assert(Meta::IsIntegralType<T>::_, "Fixed point base must be integral.");
	static_assert(sizeof(T)<8, "64-bit FixedPoint not supported!");

	FixedPoint() = default;
	FixedPoint(const FixedPoint&) = default;
	template<typename RHS> constexpr forceinline FixedPoint(RHS value): data(T(value*DIV)) {}

	template<typename T2> forceinline operator T2() const {return cast<T2>();}

	template<typename T2, uint DIV2> constexpr forceinline explicit FixedPoint(const FixedPoint<T2, DIV2>& rhs): data(T(long64(rhs.data)*DIV/DIV2)) {}

	constexpr forceinline FixedPoint operator+(FixedPoint rhs) const {return FixedPoint(data+rhs.data);}
	constexpr forceinline FixedPoint operator-(FixedPoint rhs) const {return FixedPoint(data-rhs.data);}
	constexpr forceinline FixedPoint operator*(FixedPoint rhs) const {return FixedPoint(data*rhs.data/DIV);}
	constexpr forceinline FixedPoint operator/(FixedPoint rhs) const {return FixedPoint(data*DIV/rhs.data);}
	constexpr forceinline FixedPoint operator-() const {return CastFromInt(-data);}

	forceinline FixedPoint& operator+=(FixedPoint rhs) {data = T(data+rhs.data); return *this;}
	forceinline FixedPoint& operator-=(FixedPoint rhs) {data = T(data-rhs.data); return *this;}
	forceinline FixedPoint& operator*=(FixedPoint rhs) {data = T(data*rhs.data/DIV); return *this;}
	forceinline FixedPoint& operator/=(FixedPoint rhs) {data = (data*DIV/rhs.data); return *this;}

	template<typename U> constexpr forceinline FixedPoint operator+(U rhs) const {return FixedPoint(data+T(rhs*DIV));}
	template<typename U> constexpr forceinline FixedPoint operator-(U rhs) const {return FixedPoint(data-T(rhs*DIV));}
	template<typename U> constexpr forceinline FixedPoint operator*(U rhs) const {return FixedPoint(data*T(rhs));}
	template<typename U> constexpr forceinline FixedPoint operator/(U rhs) const {return FixedPoint(data/T(rhs));}

	template<typename U> forceinline FixedPoint& operator+=(U rhs) {data = T(data+T(rhs*DIV)); return *this;}
	template<typename U> forceinline FixedPoint& operator-=(U rhs) {data = T(data-T(rhs*DIV)); return *this;}
	template<typename U> forceinline FixedPoint& operator*=(U rhs) {data = T(data*T(rhs)); return *this;}
	template<typename U> forceinline FixedPoint& operator/=(U rhs) {data = T(data/T(rhs)); return *this;}


	constexpr forceinline bool operator==(FixedPoint rhs) const {return data==rhs.data;}
	constexpr forceinline bool operator!=(FixedPoint rhs) const {return data!=rhs.data;}
	constexpr forceinline bool operator<(FixedPoint rhs) const {return data<rhs.data;}
	constexpr forceinline bool operator>(FixedPoint rhs) const {return data>rhs.data;}
	constexpr forceinline bool operator<=(FixedPoint rhs) const {return data<=rhs.data;}
	constexpr forceinline bool operator>=(FixedPoint rhs) const {return data>=rhs.data;}

	static constexpr forceinline FixedPoint CastFromInt(T v) {return FixedPoint(v, null);}

	static const FixedPoint Max;
	static const FixedPoint Min;

private:
	T data;
	enum: T {max_t = (1LL << (sizeof(T)*8-Meta::IsSignedIntegralType<T>::_))-1}; //Наибольшее значение, которое может принимать тип T
	enum: T {min_t = T(Meta::IsUnsignedIntegralType<T>::_? 0: (-1LL-max_t))};

	constexpr forceinline FixedPoint(T v, null_t): data(v) {}

	template<typename T2> Meta::EnableIf<Meta::IsFloatType<T2>::_, T2> cast() const {return T2(data)/DIV;}
	template<typename T2> Meta::EnableIf<Meta::IsIntegralType<T2>::_, T2> cast() const {return T2(long64(data)/DIV);}
};

template<typename T, uint DIV> const FixedPoint<T, DIV> FixedPoint<T, DIV>::Max = FixedPoint<T, DIV>::CastFromInt(max_t);
template<typename T, uint DIV> const FixedPoint<T, DIV> FixedPoint<T, DIV>::Min = FixedPoint<T, DIV>::CastFromInt(min_t);

//Нормализованные типы в интервале [0;1)
typedef FixedPoint<byte, 0x100> norm8;
typedef FixedPoint<ushort, 0x10000> norm16;

//Нормализованные в интервале [0;1], но медленные
typedef FixedPoint<byte, 0xFF> norm8s;
typedef FixedPoint<ushort, 0xFFFF> norm16s;
typedef FixedPoint<uint, 0xFFFFFFFF> norm32s;

//Нормализованные типы в интервале [-1;1)
typedef FixedPoint<sbyte, 0x80> snorm8;
typedef FixedPoint<short, 0x8000> snorm16;
typedef FixedPoint<int, 0x80000000> snorm32;

//Нормализованные типы в интервале [-1;1], но медленные
typedef FixedPoint<sbyte, 0x7F> snorm8s;
typedef FixedPoint<short, 0x7FFF> snorm16s;
typedef FixedPoint<int, 0x7FFFFFFF> snorm32s;

#ifdef INTRA_USER_DEFINED_LITERAL_SUPPORT
forceinline constexpr norm8 operator"" _n8(real v) {return norm8(v);}
forceinline constexpr norm16 operator"" _n16(real v) {return norm16(v);}
forceinline constexpr norm16 operator"" _n(real v) {return norm16(v);}

forceinline constexpr norm8s operator"" _n8s(real v) {return norm8s(v);}
forceinline constexpr norm16s operator"" _n16s(real v) {return norm16s(v);}
forceinline constexpr norm16s operator"" _ns(real v) {return norm16s(v);}
forceinline constexpr norm32s operator"" _n32s(real v) {return norm32s(v);}

forceinline constexpr snorm8 operator"" _s8(real v) {return snorm8(v);}
forceinline constexpr snorm16 operator"" _s16(real v) {return snorm16(v);}
forceinline constexpr snorm16 operator"" _s(real v) {return snorm16(v);}
forceinline constexpr snorm32 operator"" _s32(real v) {return snorm32(v);}

forceinline constexpr snorm8s operator"" _s8s(real v) {return snorm8s(v);}
forceinline constexpr snorm16s operator"" _s16s(real v) {return snorm16s(v);}
forceinline constexpr snorm16s operator"" _ss(real v) {return snorm16s(v);}
forceinline constexpr snorm32s operator"" _s32s(real v) {return snorm32s(v);}
#endif

typedef FixedPoint<byte, 0x10> fixed8;
typedef FixedPoint<ushort, 0x100> fixed16;
typedef FixedPoint<uint, 0x10000> fixed32;
typedef FixedPoint<sbyte, 0x10> sfixed8;
typedef FixedPoint<short, 0x100> sfixed16;
typedef FixedPoint<int, 0x10000> sfixed32;

#ifdef INTRA_USER_DEFINED_LITERAL_SUPPORT
forceinline constexpr fixed8 operator"" _x8(real v) {return fixed8(v);}
forceinline constexpr fixed16 operator"" _x16(real v) {return fixed16(v);}
forceinline constexpr fixed16 operator"" _x(real v) {return fixed16(v);}
forceinline constexpr fixed32 operator"" _x32(real v) {return fixed32(v);}

forceinline constexpr sfixed8 operator"" _sx8(real v) {return sfixed8(v);}
forceinline constexpr sfixed16 operator"" _sx16(real v) {return sfixed16(v);}
forceinline constexpr sfixed16 operator"" _sx(real v) {return sfixed16(v);}
forceinline constexpr sfixed32 operator"" _sx32(real v) {return sfixed32(v);}
#endif



typedef FixedPoint<ushort, 100> decimal16_2;
typedef FixedPoint<short, 100> sdecimal16_2;
typedef FixedPoint<uint, 100> decimal32_2;
typedef FixedPoint<int, 100> sdecimal32_2;
typedef FixedPoint<uint, 1000> decimal32_3;
typedef FixedPoint<int, 1000> sdecimal32_3;




namespace Math {

typedef Vector2<norm8s> N8Vec2;
typedef Vector3<norm8s> N8Vec3;
typedef Vector4<norm8s> N8Vec4;

typedef Vector2<norm16s> N16Vec2;
typedef Vector3<norm16s> N16Vec3;
typedef Vector4<norm16s> N16Vec4;

typedef Vector2<norm32s> N32Vec2;
typedef Vector3<norm32s> N32Vec3;
typedef Vector4<norm32s> N32Vec4;


typedef Vector2<snorm8s> S8Vec2;
typedef Vector3<snorm8s> S8Vec3;
typedef Vector4<snorm8s> S8Vec4;

typedef Vector2<snorm16s> S16Vec2;
typedef Vector3<snorm16s> S16Vec3;
typedef Vector4<snorm16s> S16Vec4;

typedef Vector2<snorm32s> S32Vec2;
typedef Vector3<snorm32s> S32Vec3;
typedef Vector4<snorm32s> S32Vec4;

}

}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
