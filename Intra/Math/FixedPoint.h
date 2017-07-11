#pragma once

#include "Cpp/Features.h"
#include "Cpp/Warnings.h"

#include "Meta/Type.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif

namespace Intra { namespace Math {

template<typename T, uint DIV> struct FixedPoint
{
	enum: uint {Divisor = DIV};

	static_assert(Meta::IsIntegralType<T>::_, "Fixed point base must be integral.");
	static_assert(sizeof(T)<8, "64-bit FixedPoint not supported!");

	FixedPoint() = default;
	template<typename RHS> constexpr forceinline FixedPoint(RHS value): data(T(value*DIV)) {}

	template<typename T2> forceinline operator T2() const {return cast<T2>();}

	template<typename T2, uint DIV2> constexpr forceinline explicit FixedPoint(const FixedPoint<T2, DIV2>& rhs): data(T(long64(rhs.data)*DIV/DIV2)) {}
	FixedPoint(const FixedPoint&) = default;

	constexpr forceinline FixedPoint operator+(FixedPoint rhs) const {return FixedPoint(data + rhs.data);}
	constexpr forceinline FixedPoint operator-(FixedPoint rhs) const {return FixedPoint(data - rhs.data);}
	constexpr forceinline FixedPoint operator*(FixedPoint rhs) const {return FixedPoint(data * rhs.data / DIV);}
	constexpr forceinline FixedPoint operator/(FixedPoint rhs) const {return FixedPoint(data * DIV / rhs.data);}
	constexpr forceinline FixedPoint operator-() const {return CastFromInt(-data);}

	forceinline FixedPoint& operator+=(FixedPoint rhs) {data = T(data+rhs.data); return *this;}
	forceinline FixedPoint& operator-=(FixedPoint rhs) {data = T(data-rhs.data); return *this;}
	forceinline FixedPoint& operator*=(FixedPoint rhs) {data = T(data*rhs.data/DIV); return *this;}
	forceinline FixedPoint& operator/=(FixedPoint rhs) {data = (data*DIV/rhs.data); return *this;}

	template<typename U> constexpr forceinline FixedPoint operator+(U rhs) const {return FixedPoint(data+T(rhs*DIV));}
	template<typename U> constexpr forceinline FixedPoint operator-(U rhs) const {return FixedPoint(data-T(rhs*DIV));}
	template<typename U> constexpr forceinline FixedPoint operator*(U rhs) const {return FixedPoint(data*T(rhs));}
	template<typename U> constexpr forceinline FixedPoint operator/(U rhs) const {return FixedPoint(data/T(rhs));}

	FixedPoint& operator=(const FixedPoint&) = default;
	template<typename U> forceinline FixedPoint& operator=(U rhs) {data = T(rhs*DIV); return *this;}
	template<typename U> forceinline FixedPoint& operator+=(U rhs) {data = T(data+T(rhs*DIV)); return *this;}
	template<typename U> forceinline FixedPoint& operator-=(U rhs) {data = T(data-T(rhs*DIV)); return *this;}
	template<typename U> forceinline FixedPoint& operator*=(U rhs) {data = T(data*T(rhs)); return *this;}
	template<typename U> forceinline FixedPoint& operator/=(U rhs) {data = T(data/T(rhs)); return *this;}


	constexpr forceinline bool operator==(FixedPoint rhs) const {return data == rhs.data;}
	constexpr forceinline bool operator!=(FixedPoint rhs) const {return data != rhs.data;}
	constexpr forceinline bool operator<(FixedPoint rhs) const {return data < rhs.data;}
	constexpr forceinline bool operator>(FixedPoint rhs) const {return data > rhs.data;}
	constexpr forceinline bool operator<=(FixedPoint rhs) const {return data <= rhs.data;}
	constexpr forceinline bool operator>=(FixedPoint rhs) const {return data >= rhs.data;}

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

//! Беззнаковое число размером 1 байт в интервале [0; 1) с точностью до 1/256 (~ 2 десятичных разряда после запятой).
typedef FixedPoint<byte, 0x100> Norm8;

//! Беззнаковое число размером 2 байта в интервале [0; 1) с точностью до 1/65536 (~ 4,5 десятичных разряда после запятой).
typedef FixedPoint<ushort, 0x10000> Norm16;


//! Беззнаковое число размером 1 байт в интервале [0; 1] с точностью до 1/255 (~ 2 десятичных разряда после запятой).
//! Медленнее, чем Norm8.
typedef FixedPoint<byte, 0xFF> Norm8s;

//! Беззнаковое число размером 2 байта в интервале [0; 1] с точностью до 1/65535 (~ 4,5 десятичных разряда после запятой).
//! Медленнее, чем Norm16.
typedef FixedPoint<ushort, 0xFFFF> Norm16s;

//! Беззнаковое число размером 4 байта в интервале [0; 1] с точностью до 1/(2^32 - 1) (~ 9,5 десятичных разряда после запятой).
typedef FixedPoint<uint, 0xFFFFFFFF> Norm32s;


//! Знаковое число размером 1 байт в интервале [-1; 1) с точностью до 1/128 (~ 2 десятичных разряда после запятой).
typedef FixedPoint<sbyte, 0x80> SNorm8;

//! Знаковое число размером 2 байта в интервале [-1; 1) с точностью до 1/32768 (~ 4,5 десятичных разрядов после запятой).
typedef FixedPoint<short, 0x8000> SNorm16;

//! Знаковое число размером 4 байта в интервале [-1; 1) с точностью до 2^-31 (~ 9 десятичных разрядов после запятой).
typedef FixedPoint<int, 0x80000000> SNorm32;


//! Знаковое число размером 1 байт в интервале [-1; 1] с точностью до 1/127.5 (~ 2 десятичных разряда после запятой).
//! Медленнее, чем SNorm8.
typedef FixedPoint<sbyte, 0x7F> SNorm8s;

//! Знаковое число размером 2 байта в интервале [-1; 1] с точностью до 1/32767.5 (~ 4,5 десятичных разряда после запятой).
//! Медленнее, чем SNorm16.
typedef FixedPoint<short, 0x7FFF> SNorm16s;

//! Знаковое число размером 4 байта в интервале [-1; 1] с точностью до 1/(2^31 - 1) (~ 9 десятичных разрядов после запятой).
//! Медленнее, чем SNorm32.
typedef FixedPoint<int, 0x7FFFFFFF> SNorm32s;

#ifdef INTRA_USER_DEFINED_LITERAL_SUPPORT
namespace FixedLiterals {
forceinline constexpr Norm8 operator"" _n8(real v) {return Norm8(v);}
forceinline constexpr Norm16 operator"" _n16(real v) {return Norm16(v);}
forceinline constexpr Norm16 operator"" _n(real v) {return Norm16(v);}

forceinline constexpr Norm8s operator"" _n8s(real v) {return Norm8s(v);}
forceinline constexpr Norm16s operator"" _n16s(real v) {return Norm16s(v);}
forceinline constexpr Norm16s operator"" _ns(real v) {return Norm16s(v);}
forceinline constexpr Norm32s operator"" _n32s(real v) {return Norm32s(v);}

forceinline constexpr SNorm8 operator"" _s8(real v) {return SNorm8(v);}
forceinline constexpr SNorm16 operator"" _s16(real v) {return SNorm16(v);}
forceinline constexpr SNorm16 operator"" _s(real v) {return SNorm16(v);}
forceinline constexpr SNorm32 operator"" _s32(real v) {return SNorm32(v);}

forceinline constexpr SNorm8s operator"" _s8s(real v) {return SNorm8s(v);}
forceinline constexpr SNorm16s operator"" _s16s(real v) {return SNorm16s(v);}
forceinline constexpr SNorm16s operator"" _ss(real v) {return SNorm16s(v);}
forceinline constexpr SNorm32s operator"" _s32s(real v) {return SNorm32s(v);}
}
#endif


//! Беззнаковое число размером 1 байт в интервале [0; 16) с точностью до 1/16 (~ 1 десятичный разряд после запятой).
typedef FixedPoint<byte, 0x10> Fixed8;

//! Беззнаковое число размером 2 байта в интервале [0; 256) с точностью до 1/256 (~ 2 десятичных разряда после запятой).
typedef FixedPoint<ushort, 0x100> Fixed16;

//! Беззнаковое число размером 4 байта в интервале [0; 65536) с точностью до 1/65536 (~ 4,5 десятичных разряда после запятой).
typedef FixedPoint<uint, 0x10000> Fixed32;

//! Знаковое число размером 1 байт в интервале [-8; 8) с точностью до 1/16 (~ 1 десятичный разряд после запятой).
typedef FixedPoint<sbyte, 0x10> SFixed8;

//! Знаковое число размером 2 байта в интервале [-128; 128) с точностью до 1/256 (~ 2 десятичных разряда после запятой).
typedef FixedPoint<short, 0x100> SFixed16;

//! Знаковое число размером 4 байта в интервале [-32768; 32768) с точностью до 1/65536 (~ 4,5 десятичных разряда после запятой).
typedef FixedPoint<int, 0x10000> SFixed32;

#ifdef INTRA_USER_DEFINED_LITERAL_SUPPORT
namespace FixedLiterals {
forceinline constexpr Fixed8 operator"" _x8(real v) {return Fixed8(v);}
forceinline constexpr Fixed16 operator"" _x16(real v) {return Fixed16(v);}
forceinline constexpr Fixed16 operator"" _x(real v) {return Fixed16(v);}
forceinline constexpr Fixed32 operator"" _x32(real v) {return Fixed32(v);}

forceinline constexpr SFixed8 operator"" _sx8(real v) {return SFixed8(v);}
forceinline constexpr SFixed16 operator"" _sx16(real v) {return SFixed16(v);}
forceinline constexpr SFixed16 operator"" _sx(real v) {return SFixed16(v);}
forceinline constexpr SFixed32 operator"" _sx32(real v) {return SFixed32(v);}
}
#endif


//! Беззнаковое число размером 2 байта в интервале [0; 655,35] с точностью до 0.01 (ровно 2 десятичных разряда после запятой).
typedef FixedPoint<ushort, 100> Decimal16_2;

//! Знаковое число размером 2 байта в интервале [-327,68; 327,67] с точностью до 0.01 (ровно 2 десятичных разряда после запятой).
typedef FixedPoint<short, 100> SDecimal16_2;

//! Беззнаковое число размером 4 байта в интервале [0; 42949672,95] с точностью до 1/100 (ровно 2 десятичных разряда после запятой).
typedef FixedPoint<uint, 100> Decimal32_2;

//! Знаковое число размером 4 байта в интервале [-21474836,48; 21474836,47] с точностью до 0,01 (ровно 2 десятичных разряда после запятой).
typedef FixedPoint<int, 100> SDecimal32_2;

//! Беззнаковое число размером 4 байта в интервале [0; 4294967,295] с точностью до 0,001 (ровно 3 десятичных разряда после запятой).
typedef FixedPoint<uint, 1000> Decimal32_3;

//! Знаковое число размером 4 байта в интервале [-2147483,648; 2147483,647] с точностью до 0,001 (ровно 3 десятичных разряда после запятой).
typedef FixedPoint<int, 1000> SDecimal32_3;



typedef Vector2<Norm8s> N8Vec2;
typedef Vector3<Norm8s> N8Vec3;
typedef Vector4<Norm8s> N8Vec4;

typedef Vector2<Norm16s> N16Vec2;
typedef Vector3<Norm16s> N16Vec3;
typedef Vector4<Norm16s> N16Vec4;

typedef Vector2<Norm32s> N32Vec2;
typedef Vector3<Norm32s> N32Vec3;
typedef Vector4<Norm32s> N32Vec4;


typedef Vector2<SNorm8s> S8Vec2;
typedef Vector3<SNorm8s> S8Vec3;
typedef Vector4<SNorm8s> S8Vec4;

typedef Vector2<SNorm16s> S16Vec2;
typedef Vector3<SNorm16s> S16Vec3;
typedef Vector4<SNorm16s> S16Vec4;

typedef Vector2<SNorm32s> S32Vec2;
typedef Vector3<SNorm32s> S32Vec3;
typedef Vector4<SNorm32s> S32Vec4;

}
using Math::Norm8;
using Math::SNorm8;
using Math::Norm16;
using Math::SNorm16;
using Math::SNorm32;

using Math::Norm8s;
using Math::SNorm8s;
using Math::Norm16s;
using Math::SNorm16s;
using Math::Norm32s;
using Math::SNorm32s;

using Math::Fixed8;
using Math::SFixed8;
using Math::Fixed16;
using Math::SFixed16;
using Math::Fixed32;
using Math::SFixed32;

using Math::Decimal16_2;
using Math::SDecimal16_2;
using Math::Decimal32_2;
using Math::SDecimal32_2;
using Math::Decimal32_3;
using Math::SDecimal32_3;

}

INTRA_WARNING_POP
