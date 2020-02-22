#pragma once

#include "Core/Core.h"
#include "Core/Numeric.h"
#include "Core/Type.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"

INTRA_BEGIN

#ifdef __GNUC__
//#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif

template<typename T, uint DIV, uint MUL=1> struct FixedPoint
{
	T Raw;

#ifdef __cpp_constexpr
	constexpr static auto Divisor = TSelect<uint, double, MUL == 1>(DIV)/MUL;
#else
	enum: uint {Divisor = DIV};
#endif
	static_assert(CPlainIntegral<T>, "Fixed point base must be integral.");
	static_assert(sizeof(T) < 8, "64-bit FixedPoint not supported!");

private:
	typedef TLargerInt<T> LargerT;

public:
	FixedPoint() = default;
	template<typename RHS> constexpr forceinline FixedPoint(RHS value): Raw(T(value*Divisor)) {}

	template<typename T2> explicit constexpr forceinline operator T2() const {return cast<T2>();}

	template<typename T2, uint DIV2> constexpr forceinline explicit FixedPoint(const FixedPoint<T2, DIV2>& rhs): Raw(T(LargerT(rhs.Raw)*Divisor/rhs.Divisor)) {}
	constexpr FixedPoint(const FixedPoint&) = default;

	INTRA_NODISCARD constexpr forceinline FixedPoint operator+(FixedPoint rhs) const {return FixedPoint(T(Raw + rhs.Raw), null);}
	INTRA_NODISCARD constexpr forceinline FixedPoint operator-(FixedPoint rhs) const {return FixedPoint(T(Raw - rhs.Raw), null);}
	INTRA_NODISCARD constexpr forceinline FixedPoint operator*(FixedPoint rhs) const {return FixedPoint(T(LargerT(Raw) * rhs.Raw / Divisor), null);}
	INTRA_NODISCARD constexpr forceinline FixedPoint operator/(FixedPoint rhs) const {return FixedPoint(T(LargerT(Raw) * Divisor / rhs.Raw), null);}
	INTRA_NODISCARD constexpr forceinline FixedPoint operator-() const {return CastFromInt(-Raw);}

	constexpr forceinline FixedPoint& operator+=(FixedPoint rhs) {Raw = T(Raw + rhs.Raw); return *this;}
	constexpr forceinline FixedPoint& operator-=(FixedPoint rhs) {Raw = T(Raw - rhs.Raw); return *this;}
	constexpr forceinline FixedPoint& operator*=(FixedPoint rhs) {Raw = T(LargerT(Raw) * rhs.Raw / Divisor); return *this;}
	constexpr forceinline FixedPoint& operator/=(FixedPoint rhs) {Raw = T(LargerT(Raw) * Divisor / rhs.Raw); return *this;}

	INTRA_NODISCARD template<typename U> constexpr forceinline FixedPoint operator+(U rhs) const {return FixedPoint(Raw+T(rhs*Divisor), null);}
	INTRA_NODISCARD template<typename U> constexpr forceinline FixedPoint operator-(U rhs) const {return FixedPoint(Raw-T(rhs*Divisor), null);}
	INTRA_NODISCARD template<typename U> constexpr forceinline FixedPoint operator*(U rhs) const {return FixedPoint(T(LargerT(Raw)*rhs), null);}
	INTRA_NODISCARD template<typename U> constexpr forceinline FixedPoint operator/(U rhs) const {return FixedPoint(T(LargerT(Raw)/rhs), null);}

	constexpr forceinline FixedPoint& operator=(const FixedPoint&) = default;
	template<typename U> constexpr forceinline FixedPoint& operator=(U rhs) {Raw = T(rhs*Divisor); return *this;}
	template<typename U> constexpr forceinline FixedPoint& operator+=(U rhs) {Raw = T(Raw+rhs*Divisor); return *this;}
	template<typename U> constexpr forceinline FixedPoint& operator-=(U rhs) {Raw = T(Raw-rhs*Divisor); return *this;}
	template<typename U> constexpr forceinline FixedPoint& operator*=(U rhs) {Raw = T(Raw*rhs); return *this;}
	template<typename U> constexpr forceinline FixedPoint& operator/=(U rhs) {Raw = T(Raw/rhs); return *this;}


	INTRA_NODISCARD constexpr forceinline bool operator==(FixedPoint rhs) const noexcept {return Raw == rhs.Raw;}
	INTRA_NODISCARD constexpr forceinline bool operator!=(FixedPoint rhs) const noexcept {return Raw != rhs.Raw;}
	INTRA_NODISCARD constexpr forceinline bool operator<(FixedPoint rhs) const noexcept {return Raw < rhs.Raw;}
	INTRA_NODISCARD constexpr forceinline bool operator>(FixedPoint rhs) const noexcept {return Raw > rhs.Raw;}
	INTRA_NODISCARD constexpr forceinline bool operator<=(FixedPoint rhs) const noexcept {return Raw <= rhs.Raw;}
	INTRA_NODISCARD constexpr forceinline bool operator>=(FixedPoint rhs) const noexcept {return Raw >= rhs.Raw;}

	INTRA_NODISCARD static constexpr forceinline FixedPoint CastFromInt(T v) noexcept {return FixedPoint(v, null);}

	static const FixedPoint Max;
	static const FixedPoint Min;

private:
	enum: T {max_t = LMaxOf(T()), min_t = LMinOf(T()), mask = CPlainSigned<T>? min_t: max_t};

	constexpr forceinline FixedPoint(T v, null_t): Raw(v) {}

	template<typename T2> constexpr forceinline Requires<CFloatingPoint<T2>, T2> cast() const {return T2(Raw)/Divisor;}
	template<typename T2> constexpr forceinline Requires<CIntegral<T2>, T2> cast() const {return T2(int64(Raw)/Divisor);}
};

template<typename T, uint DIV, uint MUL> constexpr const FixedPoint<T, DIV, MUL> FixedPoint<T, DIV, MUL>::Max = FixedPoint<T, DIV, MUL>::CastFromInt(max_t);
template<typename T, uint DIV, uint MUL> constexpr const FixedPoint<T, DIV, MUL> FixedPoint<T, DIV, MUL>::Min = FixedPoint<T, DIV, MUL>::CastFromInt(min_t);

//! Unsigned 1 byte fixed point number: range [0; 1), step 1/256 (~ 2 decimal digits).
typedef FixedPoint<byte, 256> Norm8;

//! Unsigned 2 byte fixed point number: range [0; 1), step 1/65536 (~ 4.5 decimal digits).
typedef FixedPoint<ushort, 65536> Norm16;


//! Unsigned 2 byte fixed point number: range [0; 1], step 1/255 (~ 2 decimal digits).
//! Slower than Norm8 but its range includes 1.
typedef FixedPoint<byte, 255> Norm8s;

//! Unsigned 2 byte fixed point number: range [0; 1], step 1/65535 (~ 4.5 decimal digits).
//! Slower than Norm16 but its range includes 1.
typedef FixedPoint<ushort, 65535> Norm16s;

//! Unsigned 4 byte fixed point number: range [0; 1], step 1/(2^32 - 1) (~ 9.5 decimal digits).
typedef FixedPoint<uint, 4294967295> Norm32s;


//! Signed 1 byte fixed point number: range [-1; 1), step 1/128 (~ 2 decimal digits).
typedef FixedPoint<sbyte, 0x80> SNorm8;

//! Signed 2 byte fixed point number: range [-1; 1), step 1/32768 (~ 4.5 decimal digits).
typedef FixedPoint<short, 0x8000> SNorm16;

//! Signed 4 byte fixed point number: range [-1; 1), step 2^-31 (~ 9 decimal digits).
typedef FixedPoint<int, 0x80000000> SNorm32;


//! Signed 1 byte fixed point number: range [-1; 1] step 1/127.5 (~ 2 decimal digits).
//! Slower than SNorm8.
typedef FixedPoint<sbyte, 0x7F> SNorm8s;

//! Signed 2 byte fixed point number: range [-1; 1], step 1/32767.5 (~ 4.5 decimal digits).
//! Slower than SNorm16.
typedef FixedPoint<short, 0x7FFF> SNorm16s;

//! Signed 4 byte fixed point number: range [-1; 1], step 1/(2^31 - 1) (~ 9 decimal digits).
//! Slower than SNorm32.
typedef FixedPoint<int, 0x7FFFFFFF> SNorm32s;

#if defined(__cpp_user_defined_literals) && __cpp_user_defined_literals >= 200809
namespace FixedLiterals {
INTRA_NODISCARD constexpr forceinline Norm8 operator"" _n8(long double v) {return Norm8(v);}
INTRA_NODISCARD constexpr forceinline Norm16 operator"" _n16(long double v) {return Norm16(v);}
INTRA_NODISCARD constexpr forceinline Norm16 operator"" _n(long double v) {return Norm16(v);}

INTRA_NODISCARD constexpr forceinline Norm8s operator"" _n8s(long double v) {return Norm8s(v);}
INTRA_NODISCARD constexpr forceinline Norm16s operator"" _n16s(long double v) {return Norm16s(v);}
INTRA_NODISCARD constexpr forceinline Norm16s operator"" _ns(long double v) {return Norm16s(v);}
INTRA_NODISCARD constexpr forceinline Norm32s operator"" _n32s(long double v) {return Norm32s(v);}

INTRA_NODISCARD constexpr forceinline SNorm8 operator"" _s8(long double v) {return SNorm8(v);}
INTRA_NODISCARD constexpr forceinline SNorm16 operator"" _s16(long double v) {return SNorm16(v);}
INTRA_NODISCARD constexpr forceinline SNorm16 operator"" _s(long double v) {return SNorm16(v);}
INTRA_NODISCARD constexpr forceinline SNorm32 operator"" _s32(long double v) {return SNorm32(v);}

INTRA_NODISCARD constexpr forceinline SNorm8s operator"" _s8s(long double v) {return SNorm8s(v);}
INTRA_NODISCARD constexpr forceinline SNorm16s operator"" _s16s(long double v) {return SNorm16s(v);}
INTRA_NODISCARD constexpr forceinline SNorm16s operator"" _ss(long double v) {return SNorm16s(v);}
INTRA_NODISCARD constexpr forceinline SNorm32s operator"" _s32s(long double v) {return SNorm32s(v);}
}
#endif


//! Unsigned 1 byte fixed point number: range [0; 16), step 1/16 (~ 1 fractional decimal digit).
typedef FixedPoint<byte, 0x10> Fixed8;

//! Unsigned 2 byte fixed point number: range [0; 256), step 1/256 (~ 2  fractionaldecimal digits).
typedef FixedPoint<ushort, 0x100> Fixed16;

//! Unsigned 4 byte fixed point number: range [0; 65536), step 1/65536 (~ 4.5 fractional decimal digits).
typedef FixedPoint<uint32, 0x10000> Fixed32;

//! Signed 1 byte fixed point number: range [-8; 8), step 1/16 (~ 1 fractionl decimal digit).
typedef FixedPoint<sbyte, 0x10> SFixed8;

//! Signed 2 byte fixed point number: range [-128; 128), step 1/256 (~ 2 fractional decimal digits).
typedef FixedPoint<short, 0x100> SFixed16;

//! Signed 4 byte fixed point number: range [-32768; 32768), step 1/65536 (~ 4.5 fractional decimal digits).
typedef FixedPoint<int, 0x10000> SFixed32;

#if defined(__cpp_user_defined_literals) && __cpp_user_defined_literals >= 200809
namespace FixedLiterals {
INTRA_NODISCARD constexpr forceinline Fixed8 operator"" _x8(long double v) {return Fixed8(v);}
INTRA_NODISCARD constexpr forceinline Fixed16 operator"" _x16(long double v) {return Fixed16(v);}
INTRA_NODISCARD constexpr forceinline Fixed16 operator"" _x(long double v) {return Fixed16(v);}
INTRA_NODISCARD constexpr forceinline Fixed32 operator"" _x32(long double v) {return Fixed32(v);}

INTRA_NODISCARD constexpr forceinline SFixed8 operator"" _sx8(long double v) {return SFixed8(v);}
INTRA_NODISCARD constexpr forceinline SFixed16 operator"" _sx16(long double v) {return SFixed16(v);}
INTRA_NODISCARD constexpr forceinline SFixed16 operator"" _sx(long double v) {return SFixed16(v);}
INTRA_NODISCARD constexpr forceinline SFixed32 operator"" _sx32(long double v) {return SFixed32(v);}
}
#endif


//! Unsigned 2 byte fixed point decimal number: range [0; 655.35], step 0.01 (exactly 2 fractional decimal digits).
typedef FixedPoint<ushort, 100> Decimal16_2;

//! Signed 2 byte fixed point decimal number: range [-327.68; 327.67], step 0.01 (exactly 2 fractional decimal digits).
typedef FixedPoint<short, 100> SDecimal16_2;

//! Unsigned 4 byte fixed point decimal number: range [0; 42949672.95], step 0.01 (exactly 2 fractional decimal digits).
typedef FixedPoint<uint, 100> Decimal32_2;

//! Signed 4 byte fixed point decimal number: range [-21474836.48; 21474836.47], step 0.01 (exactly 2 fractional decimal digits).
typedef FixedPoint<int, 100> SDecimal32_2;

//! Unsigned 4 byte fixed point decimal number: range [0; 4294967.295], step 0.001 (exactly 3 fractional decimal digits).
typedef FixedPoint<uint, 1000> Decimal32_3;

//! Signed 4 byte fixed point decimal number: range [-2147483.648; 2147483.647], step 0.001 (exactly 3 fractional decimal digits).
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

#if INTRA_CONSTEXPR_TEST
static_assert(Norm8(1.0/256) + Norm8(2.0/256) == Norm8(3.0/256), "TEST FAILED");
static_assert(Norm8(178.0/256) + Norm8(122.0/256) == Norm8(44.0/256), "TEST FAILED");
static_assert(SNorm8(-122.0/128) - SNorm8(118.0/128) == SNorm8(16.0/128), "TEST FAILED"); //TODO: is this UB (signed integer overflow)? If it is why does it compile in constexpr context?
static_assert(Norm8(178.0/256) * Norm8(122.0/256) == Norm8(84.0/256), "TEST FAILED");
static_assert(Norm8(178.0/256) * 3 == Norm8(22.0/256), "TEST FAILED");
static_assert(SFixed32(37.662445068359375) * SFixed32(-122.5) == -4613.6495208740234375, "TEST FAILED");
static_assert(SFixed32(-4613.6495208740234375) / SFixed32(-37.662445068359375) == SFixed32(122.5), "TEST FAILED");
#endif

INTRA_END
