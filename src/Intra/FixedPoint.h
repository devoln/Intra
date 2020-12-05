#pragma once

#include "Intra/Core.h"
#include "Intra/Functional.h"
#include "Intra/Numeric.h"
#include "Intra/Type.h"
#include "Intra/TypeSafe.h"

INTRA_BEGIN
template<typename T, unsigned DIV, template<typename> class Overflow = Overflow::CheckedInDebug>
struct FixedPoint
{
	static_assert(CUnqualedIntegral<T>, "Fixed point base must be integral.");
	static_assert(sizeof(T) <= sizeof(int32), "FixedPoint larger than 32 bit is not supported!");

	Overflow<T> Raw;

private:
	template<typename U> using MulT = TSelect<U,
		TIntMin<Min(sizeof(U)+sizeof(DIV), size_t(8)), CSignedIntegral<U>>,
		CFloatingPoint<U>>;

public:
	FixedPoint() = default;
	template<typename RHS> constexpr FixedPoint(RHS value): Raw(T(MulT<RHS>(value)*MulT<RHS>(DIV))) {}

	template<typename T2> explicit constexpr operator T2() const {return cast<T2>();}

	template<typename T2, unsigned DIV2> constexpr explicit FixedPoint(const FixedPoint<T2, DIV2>& rhs):
		Raw(MulT<T>(rhs.Raw)*MulT<T>(DIV)/MulT<T>(DIV2)) {}
	constexpr FixedPoint(const FixedPoint&) = default;

	[[nodiscard]] constexpr FixedPoint operator+(FixedPoint rhs) const
	{
		return FixedPoint(Raw + rhs.Raw, null);
	}
	[[nodiscard]] constexpr FixedPoint operator-(FixedPoint rhs) const
	{
		return FixedPoint(Raw - rhs.Raw, null);
	}
	[[nodiscard]] constexpr FixedPoint operator*(FixedPoint rhs) const
	{
		return FixedPoint(MulT<T>(Raw) * MulT<T>(rhs.Raw) / MulT<T>(DIV), null);
	}
	[[nodiscard]] constexpr FixedPoint operator/(FixedPoint rhs) const
	{
		return FixedPoint(MulT<T>(Raw) * MulT<T>(DIV) / MulT<T>(rhs.Raw), null);
	}
	[[nodiscard]] constexpr FixedPoint operator-() const
	{
		return CastFromInt(-Raw);
	}

	constexpr FixedPoint& operator+=(FixedPoint rhs)
	{
		Raw = Raw + rhs.Raw; return *this;
	}
	constexpr FixedPoint& operator-=(FixedPoint rhs)
	{
		Raw = Raw - rhs.Raw; return *this;
	}
	constexpr FixedPoint& operator*=(FixedPoint rhs)
	{
		Raw = MulT<T>(Raw) * MulT<T>(rhs.Raw) / MulT<T>(DIV); return *this;
	}
	constexpr FixedPoint& operator/=(FixedPoint rhs)
	{
		Raw = MulT<T>(Raw) * MulT<T>(DIV) / MulT<T>(rhs.Raw); return *this;
	}

	template<typename U> requires CArithmetic<U>
	[[nodiscard]] constexpr FixedPoint operator+(U rhs) const
	{
		return FixedPoint(MulT<U>(Raw) + MulT<U>(rhs)*MulT<U>(DIV), null);
	}
	template<typename U> requires CArithmetic<U>
	[[nodiscard]] constexpr FixedPoint operator-(U rhs) const
	{
		return FixedPoint(MulT<U>(Raw) - MulT<U>(rhs)*MulT<U>(DIV), null);
	}
	template<typename U> requires CArithmetic<U>
	[[nodiscard]] constexpr FixedPoint operator*(U rhs) const
	{
		return FixedPoint(MulT<U>(Raw) * MulT<U>(rhs), null);
	}
	template<typename U> requires CArithmetic<U>
	[[nodiscard]] constexpr FixedPoint operator/(U rhs) const
	{
		return FixedPoint(MulT<U>(Raw) / MulT<U>(rhs), null);
	}

	constexpr FixedPoint& operator=(const FixedPoint&) = default;

	template<typename U> requires CArithmetic<U>
	constexpr FixedPoint& operator=(U rhs)
	{
		Raw = MulT<U>(rhs)*MulT<U>(DIV);
		return *this;
	}
	template<typename U> requires CArithmetic<U>
	constexpr FixedPoint& operator+=(U rhs)
	{
		Raw = MulT<U>(Raw) + MulT<U>(rhs)*MulT<U>(DIV);
		return *this;
	}
	template<typename U> requires CArithmetic<U>
	constexpr FixedPoint& operator-=(U rhs)
	{
		Raw = MulT<U>(Raw) - MulT<U>(rhs)*MulT<U>(DIV);
		return *this;
	}
	template<typename U> requires CArithmetic<U>
	constexpr FixedPoint& operator*=(U rhs)
	{
		Raw = MulT<U>(Raw)*MulT<U>(rhs);
		return *this;
	}
	template<typename U> requires CArithmetic<U>
	constexpr FixedPoint& operator/=(U rhs)
	{
		Raw = MulT<U>(Raw) / MulT<U>(rhs);
		return *this;
	}

	[[nodiscard]] constexpr bool operator==(FixedPoint rhs) const noexcept {return Raw == rhs.Raw;}
	[[nodiscard]] constexpr bool operator!=(FixedPoint rhs) const noexcept {return Raw != rhs.Raw;}
	[[nodiscard]] constexpr bool operator<(FixedPoint rhs) const noexcept {return Raw < rhs.Raw;}
	[[nodiscard]] constexpr bool operator>(FixedPoint rhs) const noexcept {return Raw > rhs.Raw;}
	[[nodiscard]] constexpr bool operator<=(FixedPoint rhs) const noexcept {return Raw <= rhs.Raw;}
	[[nodiscard]] constexpr bool operator>=(FixedPoint rhs) const noexcept {return Raw >= rhs.Raw;}

	[[nodiscard]] static constexpr FixedPoint CastFromInt(T v) noexcept {return FixedPoint(v, null);}

	[[nodiscard]] static constexpr FixedPoint Max() {return CastFromInt(MaxValueOf<T>);}
	[[nodiscard]] static constexpr FixedPoint Min() {return CastFromInt(MinValueOf<T>);}
	[[nodiscard]] static constexpr FixedPoint Epsilon() {return CastFromInt(1);}

private:
	constexpr FixedPoint(Overflow<T> v, decltype(null)): Raw(v) {}

	template<typename T2> requires CFloatingPoint<T2>
	constexpr T2 cast() const {return T2(Raw.Value)/DIV;}
	template<typename T2> requires CIntegral<T2>
	constexpr T2 cast() const {return T2(MulT<T2>(Raw)/MulT<T2>(DIV));}
};

/// Unsigned 1 byte fixed point number: range [0; 1), step 1/256 (~ 2 decimal digits).
using Norm8 = FixedPoint<byte, 256>;
using RNorm8 = FixedPoint<byte, 256, Overflow::Wrap>;

/// Unsigned 2 byte fixed point number: range [0; 1), step 1/65536 (~ 4.5 decimal digits).
using Norm16 = FixedPoint<uint16, 65536>;
using RNorm16 = FixedPoint<uint16, 65536, Overflow::Wrap>;


/// Unsigned 2 byte fixed point number: range [0; 1], step 1/255 (~ 2 decimal digits).
/// Slower than Norm8 but its range includes 1.
using Norm8s = FixedPoint<byte, 255>;

/// Unsigned 2 byte fixed point number: range [0; 1], step 1/65535 (~ 4.5 decimal digits).
/// Slower than Norm16 but its range includes 1.
using Norm16s = FixedPoint<uint16, 65535>;

/// Unsigned 4 byte fixed point number: range [0; 1], step 1/(2^32 - 1) (~ 9.5 decimal digits).
using Norm32s = FixedPoint<unsigned, 4294967295>;


/// Signed 1 byte fixed point number: range [-1; 1), step 1/128 (~ 2 decimal digits).
using SNorm8 = FixedPoint<int8, 0x80>;
using RSNorm8 = FixedPoint<int8, 0x80, Overflow::Wrap>;

/// Signed 2 byte fixed point number: range [-1; 1), step 1/32768 (~ 4.5 decimal digits).
using SNorm16 = FixedPoint<short, 0x8000>;
using RSNorm16 = FixedPoint<short, 0x8000, Overflow::Wrap>;

/// Signed 4 byte fixed point number: range [-1; 1), step 2^-31 (~ 9 decimal digits).
using SNorm32 = FixedPoint<int, 0x80000000>;


/// Signed 1 byte fixed point number: range [-1; 1] step 1/127.5 (~ 2 decimal digits).
/// Slower than SNorm8.
using SNorm8s = FixedPoint<int8, 0x7F>;

/// Signed 2 byte fixed point number: range [-1; 1], step 1/32767.5 (~ 4.5 decimal digits).
/// Slower than SNorm16.
using SNorm16s = FixedPoint<short, 0x7FFF>;

/// Signed 4 byte fixed point number: range [-1; 1], step 1/(2^31 - 1) (~ 9 decimal digits).
/// Slower than SNorm32.
using SNorm32s = FixedPoint<int, 0x7FFFFFFF>;

namespace FixedLiterals {
[[nodiscard]] constexpr Norm8 operator"" _n8(long double v) {return Norm8(v);}
[[nodiscard]] constexpr Norm16 operator"" _n16(long double v) {return Norm16(v);}
[[nodiscard]] constexpr Norm16 operator"" _n(long double v) {return Norm16(v);}

[[nodiscard]] constexpr Norm8s operator"" _n8s(long double v) {return Norm8s(v);}
[[nodiscard]] constexpr Norm16s operator"" _n16s(long double v) {return Norm16s(v);}
[[nodiscard]] constexpr Norm16s operator"" _ns(long double v) {return Norm16s(v);}
[[nodiscard]] constexpr Norm32s operator"" _n32s(long double v) {return Norm32s(v);}

[[nodiscard]] constexpr SNorm8 operator"" _s8(long double v) {return SNorm8(v);}
[[nodiscard]] constexpr SNorm16 operator"" _s16(long double v) {return SNorm16(v);}
[[nodiscard]] constexpr SNorm16 operator"" _s(long double v) {return SNorm16(v);}
[[nodiscard]] constexpr SNorm32 operator"" _s32(long double v) {return SNorm32(v);}

[[nodiscard]] constexpr SNorm8s operator"" _s8s(long double v) {return SNorm8s(v);}
[[nodiscard]] constexpr SNorm16s operator"" _s16s(long double v) {return SNorm16s(v);}
[[nodiscard]] constexpr SNorm16s operator"" _ss(long double v) {return SNorm16s(v);}
[[nodiscard]] constexpr SNorm32s operator"" _s32s(long double v) {return SNorm32s(v);}
}


/// Unsigned 1 byte fixed point number: range [0; 16), step 1/16 (~ 1 fractional decimal digit).
using Fixed8 = FixedPoint<byte, 0x10>;

/// Unsigned 2 byte fixed point number: range [0; 256), step 1/256 (~ 2  fractionaldecimal digits).
using Fixed16 = FixedPoint<uint16, 0x100>;

/// Unsigned 4 byte fixed point number: range [0; 65536), step 1/65536 (~ 4.5 fractional decimal digits).
using Fixed32 = FixedPoint<uint32, 0x10000>;

/// Signed 1 byte fixed point number: range [-8; 8), step 1/16 (~ 1 fractionl decimal digit).
using SFixed8 = FixedPoint<int8, 0x10>;

/// Signed 2 byte fixed point number: range [-128; 128), step 1/256 (~ 2 fractional decimal digits).
using SFixed16 = FixedPoint<short, 0x100>;

/// Signed 4 byte fixed point number: range [-32768; 32768), step 1/65536 (~ 4.5 fractional decimal digits).
using SFixed32 = FixedPoint<int, 0x10000>;

namespace FixedLiterals {
[[nodiscard]] constexpr Fixed8 operator"" _x8(long double v) {return Fixed8(v);}
[[nodiscard]] constexpr Fixed16 operator"" _x16(long double v) {return Fixed16(v);}
[[nodiscard]] constexpr Fixed16 operator"" _x(long double v) {return Fixed16(v);}
[[nodiscard]] constexpr Fixed32 operator"" _x32(long double v) {return Fixed32(v);}

[[nodiscard]] constexpr SFixed8 operator"" _sx8(long double v) {return SFixed8(v);}
[[nodiscard]] constexpr SFixed16 operator"" _sx16(long double v) {return SFixed16(v);}
[[nodiscard]] constexpr SFixed16 operator"" _sx(long double v) {return SFixed16(v);}
[[nodiscard]] constexpr SFixed32 operator"" _sx32(long double v) {return SFixed32(v);}
}


/// Unsigned 2 byte fixed point decimal number: range [0; 655.35], step 0.01 (exactly 2 fractional decimal digits).
using Decimal16_2 = FixedPoint<uint16, 100>;

/// Signed 2 byte fixed point decimal number: range [-327.68; 327.67], step 0.01 (exactly 2 fractional decimal digits).
using SDecimal16_2 = FixedPoint<short, 100>;

/// Unsigned 4 byte fixed point decimal number: range [0; 42949672.95], step 0.01 (exactly 2 fractional decimal digits).
using Decimal32_2 = FixedPoint<unsigned, 100>;

/// Signed 4 byte fixed point decimal number: range [-21474836.48; 21474836.47], step 0.01 (exactly 2 fractional decimal digits).
using SDecimal32_2 = FixedPoint<int, 100>;

/// Unsigned 4 byte fixed point decimal number: range [0; 4294967.295], step 0.001 (exactly 3 fractional decimal digits).
using Decimal32_3 = FixedPoint<unsigned, 1000>;

/// Signed 4 byte fixed point decimal number: range [-2147483.648; 2147483.647], step 0.001 (exactly 3 fractional decimal digits).
using SDecimal32_3 = FixedPoint<int, 1000>;


/// Fixed point 16:16 natural logarithm implementation.
constexpr int FixedPointLog(uint32 x)
{
	int y = 0xa65af;
	if(x < 0x00008000) x <<= 16, y -= 0xb1721;
	if(x < 0x00800000) x <<= 8, y -= 0x58b91;
	if(x < 0x08000000) x <<= 4, y -= 0x2c5c8;
	if(x < 0x20000000) x <<= 2, y -= 0x162e4;
	if(x < 0x40000000) x <<= 1, y -= 0x0b172;
	unsigned t = x + (x >> 1); if((t & 0x80000000u) == 0) x = t, y -= 0x067cd;
	t = x + (x >> 2); if((t & 0x80000000u) == 0) x = t, y -= 0x03920;
	t = x + (x >> 3); if((t & 0x80000000u) == 0) x = t, y -= 0x01e27;
	t = x + (x >> 4); if((t & 0x80000000u) == 0) x = t, y -= 0x00f85;
	t = x + (x >> 5); if((t & 0x80000000u) == 0) x = t, y -= 0x007e1;
	t = x + (x >> 6); if((t & 0x80000000u) == 0) x = t, y -= 0x003f8;
	t = x + (x >> 7); if((t & 0x80000000u) == 0) x = t, y -= 0x001fe;
	x = 0x80000000u - x;
	y -= int(x >> 15);
	return y;
}


#if INTRA_CONSTEXPR_TEST
static_assert(Norm8(1.0/256) + Norm8(2.0/256) == Norm8(3.0/256));
static_assert(RNorm8(178.0/256) + RNorm8(122.0/256) == RNorm8(44.0/256));
static_assert(RSNorm8(-122.0/128) - RSNorm8(118.0/128) == RSNorm8(16.0/128)); //TODO: is this UB (signed integer overflow)? If it is why does it compile in constexpr context?
static_assert(Norm8(178.0/256) * Norm8(122.0/256) == Norm8(84.0/256));
static_assert(RNorm8(178.0/256) * 3 == RNorm8(22.0/256));
static_assert(SFixed32(37.662445068359375) * SFixed32(-122.5) == -4613.6495208740234375);
static_assert(SFixed32(-4613.6495208740234375) / SFixed32(-37.662445068359375) == SFixed32(122.5));

static_assert(FixedPointLog(65536) == 0);
static_assert(FixedPointLog(100000) == 27694);
static_assert(FixedPointLog(uint32(2.71828182845904523536*65536)) == 65537); // must be 65536, rounding error?
#endif

INTRA_END
