#pragma once

#include <Intra/Numeric/Rational.h>

namespace Intra { INTRA_BEGIN

constexpr struct TNaN
{
	template<CNumber T> [[nodiscard]] INTRA_FORCEINLINE constexpr bool operator==(T x) const noexcept
	{
		if constexpr(!CBasicFloatingPoint<T>) return false;
		else
		{
		#if defined(_M_FP_FAST) || defined(__FAST_MATH__)
			if(IsConstantEvaluated()) return false;
			return (BitCastTo<uint32>(x) << 1) > 0xFF000000u;
		#else
			return x != x;
		#endif
		}
	}
	template<CNumber T> [[nodiscard]] INTRA_FORCEINLINE constexpr bool operator!=(T x) const noexcept {return !operator==(x);}
	template<CNumber T> [[nodiscard]] friend INTRA_FORCEINLINE constexpr bool operator==(T x, TNaN nan) noexcept {return nan == x;}
	template<CNumber T> [[nodiscard]] friend INTRA_FORCEINLINE constexpr bool operator!=(T x, TNaN nan) noexcept {return nan != x;}
	template<CBasicFloatingPoint T> [[nodiscard]] INTRA_FORCEINLINE constexpr operator T() const noexcept {return T(__builtin_nan(""));}
} NaN;


template<CNumber RadixT, CNumber ExpT> struct NPower
{
	RadixT Radix = 2;
	ExpT Exp = 0;

	//TODO: numeric conversions, arithmetic operations
};

//TODO: generalize to work with custom types, including unlimited range integers
template<CNumber T, CNumber auto Divisor> struct Fixed
{
	T Raw{};

private:
	template<typename U> struct MulT1: TType<TBasicNumberWithRange<double(MinValueOf<T>)* MaxValueOf<U>, double(MaxValueOf<T>)* MaxValueOf<U>>> {};
	template<CBasicFloatingPoint U> struct MulT1<U>: TType<U> {};
	template<typename U> using MulT = typename MulT1<TUnqualRef<U>>::_;

public:
	Fixed() = default;
	template<typename RHS> constexpr Fixed(RHS value): Raw(T(value * MulT<RHS>(Divisor))) {}

	explicit INTRA_FORCEINLINE constexpr Fixed(decltype(Construct), T v): Raw(v) {}

	template<CNumber To> explicit constexpr operator To() const
	{
		if constexpr(CBasicIntegral<To>) return To(MulT<To>(Raw) / MulT<To>(Divisor));
		else return To(Raw.Value) / Divisor;
	}

	template<typename T2, CNumber auto Divisor2> constexpr explicit Fixed(const Fixed<T2, Divisor2>& rhs):
		Raw(MulT<T>(rhs.Raw)*MulT<T>(Divisor)/MulT<T>(Divisor2)) {}
	constexpr Fixed(const Fixed&) = default;

	[[nodiscard]] constexpr Fixed operator+(Fixed rhs) const {return Fixed(Construct, Raw + rhs.Raw);}
	[[nodiscard]] constexpr Fixed operator-(Fixed rhs) const {return Fixed(Construct, Raw - rhs.Raw);}
	[[nodiscard]] constexpr Fixed operator*(Fixed rhs) const {return Fixed(Construct, T(MulT<T>(Raw) * rhs.Raw / Divisor));}
	[[nodiscard]] constexpr Fixed operator/(Fixed rhs) const {return Fixed(Construct, T(MulT<T>(Raw) * Divisor / rhs.Raw));}
	[[nodiscard]] constexpr Fixed operator-() const {return Fixed(Construct, -Raw);}

	template<typename U> requires (!CSame<U, Fixed>) && CNumber<U>
	[[nodiscard]] constexpr Fixed operator+(U&& rhs) const
	{return Fixed(Construct, MulT<U>(Raw) + INTRA_FWD(rhs) * MulT<U>(Divisor));}

	template<typename U> requires (!CSame<U, Fixed>) && CNumber<U>
	[[nodiscard]] constexpr Fixed operator-(U&& rhs) const
	{return Fixed(Construct, MulT<U>(Raw) - INTRA_FWD(rhs) * MulT<U>(Divisor));}

	template<typename U> requires (!CSame<U, Fixed>) && CNumber<U>
	[[nodiscard]] constexpr Fixed operator*(U&& rhs) const
	{return Fixed(Construct, INTRA_FWD(rhs) * MulT<U>(Raw));}

	template<typename U> requires (!CSame<U, Fixed>) && CNumber<U>
	[[nodiscard]] constexpr Fixed operator/(U&& rhs) const
	{return Fixed(Construct, MulT<U>(Raw) / INTRA_FWD(rhs));}

	constexpr Fixed& operator=(const Fixed&) = default;

	template<typename U> requires (!CSame<U, Fixed>) && CNumber<U>
	constexpr Fixed& operator=(U rhs)
	{
		Raw = MulT<U>(rhs) * MulT<U>(Divisor);
		return *this;
	}

	bool operator==(const Fixed& rhs) const = default;
	[[nodiscard]] constexpr bool operator<(Fixed rhs) const noexcept {return Raw < rhs.Raw;}

	using TagGenOpAssign = TTag<>;
	using TagGenOpCompare = TTag<>;
	using TagGenMixedTypeAddOps = TTag<>;
	using TagGenMixedTypeMulOps = TTag<>;
};
template<CNumber T, CNumber auto Divisor> constexpr Fixed<T, Divisor>
	MinFiniteValueOf<Fixed<T, Divisor>> = Fixed<T, Divisor>(Construct, MinValueOf<T>);

template<CNumber T, CNumber auto Divisor> constexpr Fixed<T, Divisor>
	MaxFiniteValueOf<Fixed<T, Divisor>> = Fixed<T, Divisor>(Construct, MaxValueOf<T>);

template<CNumber T, CNumber auto Divisor, CNumber auto RangeMin, CNumber auto RangeMax> constexpr Fixed<T, Divisor>
	MaxStepInRange<Fixed<T, Divisor>, RangeMin, RangeMax> = Fixed<T, Divisor>(Construct, 1);

template<CNumber T, CNumber auto Divisor> constexpr Fixed<T, Divisor>
	MinNormPositiveValueOf<Fixed<T, Divisor>> = Fixed<T, Divisor>(Construct, 1);

template<CNumber T> using Normalized = Fixed<T, TUnsignedIntOfSizeAtLeast<sizeof(T) + !CBasicSigned<T>>(MaxValueOf<T>) + 1>;

#if INTRA_CONSTEXPR_TEST
static_assert(CNumber<Fixed<uint8, 256>>);
static_assert(MaxStepInRange<Fixed<uint8, 256>> == 1.0/256);
static_assert(MinValueOf<Fixed<uint8, 256>> == 0);
static_assert(MaxValueOf<Fixed<uint8, 256>> == 255.0/256);

static_assert(MaxStepInRange<Fixed<int8, 128>> == 1.0/128);
static_assert(MinValueOf<Fixed<int8, 128>> == -1);
static_assert(MaxValueOf<Fixed<int8, 128>> == 127.0/128);

static_assert(MaxStepInRange<Fixed<int32, 1000>> == Fixed<int32, 1000>(0.001));
static_assert(MinValueOf<Fixed<int32, 1000>> == Fixed<int32, 1000>(-2147483.648));
static_assert(MaxValueOf<Fixed<int32, 1000>> == Fixed<int32, 1000>(2147483.647));

static_assert(
	Fixed<uint8, 256>(1.0/256) +
	Fixed<uint8, 256>(2.0/256) ==
	Fixed<uint8, 256>(3.0/256));

static_assert(
	Fixed<uint8, 256>(178.0/256) *
	Fixed<uint8, 256>(122.0/256) ==
	Fixed<uint8, 256>(84.0/256));

static_assert(
	Fixed<uint8, 256>(178.0/256) +
	Fixed<uint8, 256>(122.0/256) ==
	Fixed<uint8, 256>(44.0/256));

static_assert(3 * Fixed<uint8, 256>(178.0/256) ==
	Fixed<uint8, 256>(22.0/256));

static_assert(
	Fixed<NWrapOverflow<int8>, 128>(-122.0/128) -
	Fixed<NWrapOverflow<int8>, 128>(118.0/128) ==
	Fixed<NWrapOverflow<int8>, 128>(16.0/128));

static_assert(
	Fixed<int, 65536>(37.662445068359375) *
	Fixed<int, 65536>(-122.5) ==
	-4613.6495208740234375);

static_assert(
	Fixed<int, 65536>(-4613.6495208740234375) /
	Fixed<int, 65536>(-37.662445068359375) ==
	Fixed<int, 65536>(122.5));
#endif

template<CBasicFloatingPoint Float> [[nodiscard]] constexpr int ExtractBiasedExponent(Float x) noexcept
{
#if defined(__GNUC__) && !defined(INTRA_CONSTEXPR_BITCAST_SUPPORT)
	if(IsConstantEvaluated()) return ExponentBiasOf<Float> + __builtin_ilogbl(x);
#endif
	constexpr int exponentMask = (1 << ExponentLenOf<Float>) - 1;
	const auto floatBits = BitCastTo<TUnsignedIntOfSizeAtLeast<sizeof(Float)>>(x);
	return int(floatBits >> NumMantissaExplicitBitsOf<Float>) & exponentMask;
}

template<CBasicFloatingPoint Float> [[nodiscard]] constexpr int ExtractExponent(Float x) noexcept
{
	return ExtractBiasedExponent(x) - ExponentBiasOf<Float>;
}

template<CBasicFloatingPoint Float> constexpr auto ExtractMantissaImplicit1(Float x) noexcept
{
	using UInt = TUnsignedIntOfSizeAtLeast<sizeof(Float)>;
#if defined(__GNUC__) && !defined(INTRA_CONSTEXPR_BITCAST_SUPPORT)
	if(IsConstantEvaluated())
	{
		if(x == Infinity || x == -Infinity) return UInt(0);
		if(x < 0) x = -x;
		x = Float(__builtin_scalbnl(x, -__builtin_ilogbl(x)) - 1);
		return UInt(x*(UInt(1) << NumMantissaExplicitBitsOf<Float>));
	}
#endif
	return BitCastTo<UInt>(x) & UInt(IntegerRangeMax<Float> - 1);
}

template<CBasicFloatingPoint Float> constexpr bool ExtractFloatSignBit(Float x) noexcept
{
	using UInt = TUnsignedIntOfSizeAtLeast<sizeof(Float)>;
#ifndef INTRA_CONSTEXPR_BITCAST_SUPPORT
	if(IsConstantEvaluated()) return x <= -0.0f;
#endif
	return BitCastTo<UInt>(x) >> (SizeofInBits<Float> - 1);
}

template<CBasicFloatingPoint Float> constexpr Float ComposeFloat(
	TUnsignedIntOfSizeAtLeast<sizeof(Float)> mantissaImplicit1, int exponent, bool negative) noexcept
{
	using UInt = decltype(mantissaImplicit1);
#if defined(__GNUC__) && !defined(INTRA_CONSTEXPR_BITCAST_SUPPORT)
	if(IsConstantEvaluated())
	{
		Float res = 1 + mantissaImplicit1*(Float(1) / (UInt(1) << NumMantissaExplicitBitsOf<Float>));
		res = Float(__builtin_scalbnl(res, exponent));
		return negative? -res: res;
	}
#endif
	return BitCastTo<Float>(mantissaImplicit1 |
		(UInt(ExponentBiasOf<Float> + exponent) << NumMantissaExplicitBitsOf<Float>) |
		(UInt(negative) << (SizeofInBits<Float> - 1)));
}

namespace z_D {
template<class To, class From> constexpr auto ConstexprBitCastBetweenFloatAndInt(const From& from) noexcept
{
	constexpr auto signBitShift = SizeofInBits<From> - 1;
	if constexpr(CBasicIntegral<From> && CBasicFloatingPoint<To>)
	{
		constexpr auto mantissaMask = (From(1) << NumMantissaExplicitBitsOf<To>) - 1;
		constexpr auto exponentMask = (From(1) << ExponentLenOf<To>) - 1;
		return ComposeFloat<To>(
			from & mantissaMask,
			((from >> NumMantissaExplicitBitsOf<To>) & exponentMask) - ExponentBiasOf<To>,
			from >> signBitShift);
	}
	else return ExtractMantissaImplicit1(from) |
		(To(ExtractExponent(from)) << NumMantissaExplicitBitsOf<From>) |
		(To(from < 0) << signBitShift);
}
}

struct HalfFloat
{
	HalfFloat() = default;

	constexpr explicit HalfFloat(decltype(Construct), uint16 s): AsUint16(s) {}
	INTRA_FORCEINLINE HalfFloat& operator=(const HalfFloat& rhs) = default;

	template<CFloatingPoint T> requires (!CSame<TRemoveConstRef<T>, HalfFloat>)
	INTRA_FORCEINLINE constexpr explicit HalfFloat(T f): AsUint16(fromFloat(float(f))) {}

	[[nodiscard]] INTRA_FORCEINLINE constexpr operator float() const {return toFloat(AsUint16);}

	template<typename T> requires CFloatingPoint<T> || CSame<T, HalfFloat>
	INTRA_FORCEINLINE constexpr HalfFloat& operator+=(T rhs) {return *this = HalfFloat(*this + float(rhs));}

	template<typename T> requires CFloatingPoint<T> || CSame<T, HalfFloat>
	INTRA_FORCEINLINE constexpr HalfFloat& operator-=(T rhs) {return *this = HalfFloat(*this - float(rhs));}

	template<typename T> requires CFloatingPoint<T> || CSame<T, HalfFloat>
	INTRA_FORCEINLINE constexpr HalfFloat& operator*=(T rhs) {return *this = HalfFloat(*this * float(rhs));}

	template<typename T> requires CFloatingPoint<T> || CSame<T, HalfFloat>
	INTRA_FORCEINLINE constexpr HalfFloat& operator/=(T rhs) {return *this = HalfFloat(*this / float(rhs));}

	uint16 AsUint16 = 0;

private:
	static constexpr uint16 fromFloat(float f)
	{
		return uint16(
			(ExtractMantissaImplicit1(f) >> (NumMantissaExplicitBitsOf<float> - 10)) |
			(uint32(ExtractExponent(f) + 15) << 10) |
			(uint32(f < 0) << 15)
		);
	}

	static constexpr float toFloat(uint16 h)
	{
		return ComposeFloat<float>(
			uint32((h & 0x3FF) << (NumMantissaExplicitBitsOf<float> - 10)),
			((h >> 10) & 0x1F) - 15,
			!!(h >> 15));
	}
};

#if INTRA_CONSTEXPR_TEST
static_assert(ExtractBiasedExponent(1/3.14f) == 125);
static_assert(ExtractBiasedExponent(3.14f) == 128);
static_assert(ExtractBiasedExponent(6.14f) == 129);
static_assert(ExtractBiasedExponent(3.14) == 1024);
static_assert(ExtractExponent(3.14f) == ExtractExponent(3.14));
#endif

template<CUnqualedBasicUnsignedIntegral MantissaT, unsigned Radix = 2> struct GenericFloat;
namespace z_D {
constexpr GenericFloat<uint32, 10> BinaryFloatToDecimal(uint32 mantissa, int exponentOf2);
constexpr GenericFloat<uint64, 10> BinaryDoubleToDecimal(uint64 mantissa, int exponentOf2);
}

/// Represents Mantissa * Radix^Exponent * (Negative? -1: 1)
template<CUnqualedBasicUnsignedIntegral MantissaT, unsigned Radix> struct GenericFloat
{
	MantissaT Mantissa = 0;
	int16 Exponent = 0;
	bool Negative = false;

	GenericFloat() = default;

	INTRA_FORCEINLINE explicit constexpr GenericFloat(decltype(Construct), MantissaT mantissa, int16 exponent, bool negative):
		Mantissa(mantissa), Exponent(exponent), Negative(negative) {}

	template<CBasicFloatingPoint Float> constexpr GenericFloat(Float x)
	{
		if constexpr(Radix == 2)
		{
			Mantissa = ExtractMantissaImplicit1(x);
			Exponent = ExtractBiasedExponent(x);
			Negative = ExtractFloatSignBit(x);
			if(Exponent == 0) Exponent++;
			else Mantissa |= MantissaT(1) << NumMantissaExplicitBitsOf<Float>;
			Exponent -= ExponentBiasOf<Float> + NumMantissaExplicitBitsOf<Float>;
		}
		else if constexpr(Radix == 10)
		{
			auto bin = GenericFloat<TUnsignedIntOfSizeAtLeast<sizeof(Float)>, 2>(x);
			if constexpr(CSameSize<Float, float>) *this = z_D::BinaryFloatToDecimal(bin.Mantissa, bin.Exponent);
			else if constexpr(CSameSize<Float, double>) *this = z_D::BinaryDoubleToDecimal(bin.Mantissa, bin.Exponent);
			Negative = bin.Negative;
		}
		else *this = GenericFloat(GenericFloat<TUnsignedIntOfSizeAtLeast<sizeof(Float)>, 2>(x));
	}

	template<typename MantissaTFrom, unsigned RadixFrom> requires(!CSame<MantissaT, MantissaTFrom> || Radix != RadixFrom)
	constexpr GenericFloat(GenericFloat<MantissaTFrom, RadixFrom> rhs): Negative(rhs.Negative)
	{
		if constexpr(Radix == RadixFrom)
		{
			if(MantissaT(rhs.Mantissa) == rhs.Mantissa)
			{
				Mantissa = rhs.Mantissa;
				Exponent = rhs.Exponent;
				return;
			}
			auto m = rhs.Mantissa;
			auto e = rhs.Exponent;
			while(m > MaxValueOf<MantissaT>) m /= Radix, e++;
			Mantissa = MantissaT(m);
			Exponent = e;
			return;
		}
		else
		{
			if(-SizeofInBits<MantissaT> < rhs.Exponent && rhs.Exponent <= 0 &&
				IsMultipleOfPowerOf(rhs.Mantissa, RadixFrom, -rhs.Exponent))
			{
				Mantissa = RadixShiftRight<Radix>(rhs.Mantissa, -rhs.Exponent);
				Exponent = 0;
				Negative = rhs.Negative;
				return;
			}
			if constexpr(Radix == 10 && RadixFrom == 2)
			{
				if((rhs.Mantissa >> MantissaLenOf<float>) == 0 &&
					-ExponentBiasOf<float> -NumMantissaExplicitBitsOf<float> < rhs.Exponent &&
					rhs.Exponent <= ExponentBiasOf<float> -NumMantissaExplicitBitsOf<float>)
				{
					auto res = z_D::BinaryFloatToDecimal(rhs.Mantissa, rhs.Exponent);
					Mantissa = NDebugOverflow<MantissaT>(res.Mantissa);
					Exponent = res.Exponent;
					return;
				}
				else if((rhs.Mantissa >> MantissaLenOf<double>) == 0 &&
					-ExponentBiasOf<double> -NumMantissaExplicitBitsOf<double> < rhs.Exponent &&
					rhs.Exponent <= ExponentBiasOf<double> -NumMantissaExplicitBitsOf<double>)
				{
					auto res = z_D::BinaryDoubleToDecimal(rhs.Mantissa, rhs.Exponent);
					Mantissa = NDebugOverflow<MantissaT>(res.Mantissa);
					Exponent = res.Exponent;
					return;
				}
			}
		}
		INTRA_DEBUG_ASSERT(!"General case is not implemented yet");
		// TODO: exact implementation will require BigInt
	}

	GenericFloat TrimTrailingZeros() const
	{
		GenericFloat res = *this;
		while(FastModByConst<Radix>(res.Mantissa) == 0)
		{
			res.Mantissa = FastDivByConstTrunc<Radix>(res.Mantissa);
			res.Exponent++;
		}
		return res;
	}

	template<CBasicFloatingPoint Float> explicit operator Float() const
	{
		return Float(Mantissa) * Pow(Float(Radix), Exponent);
	}
};

// The following code was adapted from Ryu implementation https://github.com/ulfjack/ryu

namespace z_D {
enum {
	FLOAT_POW5_INV_BITCOUNT = 59, FLOAT_POW5_BITCOUNT = 61,
	DOUBLE_POW5_INV_BITCOUNT = 125, DOUBLE_POW5_BITCOUNT = 125
};

constexpr uint64 FLOAT_POW5_INV_SPLIT[31] =
{
	576460752303423489u, 461168601842738791u, 368934881474191033u, 295147905179352826u,
	472236648286964522u, 377789318629571618u, 302231454903657294u, 483570327845851670u,
	386856262276681336u, 309485009821345069u, 495176015714152110u, 396140812571321688u,
	316912650057057351u, 507060240091291761u, 405648192073033409u, 324518553658426727u,
	519229685853482763u, 415383748682786211u, 332306998946228969u, 531691198313966350u,
	425352958651173080u, 340282366920938464u, 544451787073501542u, 435561429658801234u,
	348449143727040987u, 557518629963265579u, 446014903970612463u, 356811923176489971u,
	570899077082383953u, 456719261665907162u, 365375409332725730u
};

constexpr uint64 FLOAT_POW5_SPLIT[47] =
{
	1152921504606846976u, 1441151880758558720u, 1801439850948198400u, 2251799813685248000u,
	1407374883553280000u, 1759218604441600000u, 2199023255552000000u, 1374389534720000000u,
	1717986918400000000u, 2147483648000000000u, 1342177280000000000u, 1677721600000000000u,
	2097152000000000000u, 1310720000000000000u, 1638400000000000000u, 2048000000000000000u,
	1280000000000000000u, 1600000000000000000u, 2000000000000000000u, 1250000000000000000u,
	1562500000000000000u, 1953125000000000000u, 1220703125000000000u, 1525878906250000000u,
	1907348632812500000u, 1192092895507812500u, 1490116119384765625u, 1862645149230957031u,
	1164153218269348144u, 1455191522836685180u, 1818989403545856475u, 2273736754432320594u,
	1421085471520200371u, 1776356839400250464u, 2220446049250313080u, 1387778780781445675u,
	1734723475976807094u, 2168404344971008868u, 1355252715606880542u, 1694065894508600678u,
	2117582368135750847u, 1323488980084844279u, 1654361225106055349u, 2067951531382569187u,
	1292469707114105741u, 1615587133892632177u, 2019483917365790221u
};

constexpr GenericFloat<uint32, 10> BinaryFloatToDecimal(uint32 mantissa, int exponentOf2)
{
	INTRA_PRECONDITION((mantissa >> MantissaLenOf<float>) == 0);
	INTRA_PRECONDITION(-ExponentBiasOf<float> - NumMantissaExplicitBitsOf<float> < exponentOf2);
	INTRA_PRECONDITION(exponentOf2 <= ExponentBiasOf<float> - NumMantissaExplicitBitsOf<float>);

	// We subtract 2 so that the bounds computation has 2 additional bits.
	exponentOf2 -= 2;
	const bool even = (mantissa & 1) == 0;
	const bool acceptBounds = even;

	// Step 2: Determine the interval of valid decimal representations.
	const uint32 mv = 4 * mantissa;
	const uint32 mp = 4 * mantissa + 2;
	const uint32 mmShift = (mantissa & NumBitsToMask<uint32>(NumMantissaExplicitBitsOf<float>)) != 0 ||
		exponentOf2 + ExponentBiasOf<float> + NumMantissaExplicitBitsOf<float> == 1;
	const uint32 mm = 4 * mantissa - 1 - mmShift;

	// Step 3: Convert to a decimal power base using 64-bit arithmetic.
	uint32 vr = 0, vp = 0, vm = 0;
	int32 e10 = 0;
	bool vmIsTrailingZeros = false;
	bool vrIsTrailingZeros = false;
	uint8 lastRemovedDigit = 0;
	if(exponentOf2 >= 0)
	{
		const uint32 q = FloorLog10OfPow2Approx(uint32(exponentOf2));
		e10 = int32(q);
		const int32 k = FLOAT_POW5_INV_BITCOUNT + BitWidthOfPow<5>(q) - 1;
		const int32 i = -exponentOf2 + int32(q) + k;
		vr = MulShift(mv, FLOAT_POW5_INV_SPLIT[q], i);
		vp = MulShift(mp, FLOAT_POW5_INV_SPLIT[q], i);
		vm = MulShift(mm, FLOAT_POW5_INV_SPLIT[q], i);
		if(q != 0 && (vp - 1) / 10 <= vm / 10)
		{
			// We need to know one removed digit even if we are not going to loop below. We could use
			// q = X - 1 above, except that would require 33 bits for the result, and we've found that
			// 32-bit arithmetic is faster even on 64-bit machines.
			const int32 l = FLOAT_POW5_INV_BITCOUNT + BitWidthOfPow<5>(q - 1) - 1;
			lastRemovedDigit = uint8(MulShift(mv, FLOAT_POW5_INV_SPLIT[q - 1], -exponentOf2 + int32(q) - 1 + l) % 10);
		}
		if(q <= 9)
		{
			// The largest power of 5 that fits in 24 bits is 5^10, but q <= 9 seems to be safe as well.
			// Only one of mp, mv, and mm can be a multiple of 5, if any.
			if(mv % 5 == 0) vrIsTrailingZeros = IsMultipleOfPowerOf(mv, 5, int(q));
			else if(acceptBounds) vmIsTrailingZeros = IsMultipleOfPowerOf(mm, 5, int(q));
			else vp -= IsMultipleOfPowerOf(mp, 5, int(q));
		}
	}
	else
	{
		const uint32 q = FloorLog10OfPow5Approx(uint32(-exponentOf2));
		e10 = int32(q) + exponentOf2;
		const int32 i = -e10;
		const int32 k = BitWidthOfPow<5>(uint32(i)) - FLOAT_POW5_BITCOUNT;
		int32 j = int32(q) - k;
		vr = MulShift(mv, FLOAT_POW5_SPLIT[i], j);
		vp = MulShift(mp, FLOAT_POW5_SPLIT[i], j);
		vm = MulShift(mm, FLOAT_POW5_SPLIT[i], j);
		if(q != 0 && (vp - 1) / 10 <= vm / 10)
		{
			j = int32(q) - 1 - BitWidthOfPow<5>(uint32(i) + 1) - FLOAT_POW5_BITCOUNT;
			lastRemovedDigit = uint8(MulShift(mv, FLOAT_POW5_SPLIT[i + 1], j) % 10);
		}
		if(q <= 1)
		{
			// {vr,vp,vm} is trailing zeros if {mv,mp,mm} has at least q trailing 0 bits.
			// mv = 4 * mantissa, so it always has at least two trailing 0 bits.
			vrIsTrailingZeros = true;
			if(acceptBounds) vmIsTrailingZeros = mmShift == 1; // mm = mv - 1 - mmShift, so it has 1 trailing 0 bit iff mmShift == 1.
			else vp--; // mp = mv + 2, so it always has at least one trailing 0 bit.
		}
		else if(q < 31)
		{ // TODO(ulfjack): Use a tighter bound here.
			vrIsTrailingZeros = IsMultipleOfPowerOf(mv, 2, int(q - 1));
		}
	}

	// Step 4: Find the shortest decimal representation in the interval of valid representations.
	int removed = 0;
	uint32 output = 0;
	if(!vmIsTrailingZeros && !vrIsTrailingZeros && !Config::DisableAllOptimizations)
	{
		// Specialized for the common case (~96.0%). Percentages below are relative to this.
		// Loop iterations below (approximately):
		// 0: 13.6%, 1: 70.7%, 2: 14.1%, 3: 1.39%, 4: 0.14%, 5+: 0.01%
		while(vp / 10 > vm / 10)
		{
			lastRemovedDigit = uint8(vr % 10);
			vr /= 10, vp /= 10, vm /= 10;
			removed++;
		}
		output = vr + (vr == vm || lastRemovedDigit >= 5); // We need to take vr + 1 if vr is outside bounds or we need to round up.
	}
	else
	{
		// General case, which happens rarely (~4.0%).
		while(vp / 10 > vm / 10)
		{
			vmIsTrailingZeros &= FastModByConst<10u>(vm) == 0;
			vrIsTrailingZeros &= lastRemovedDigit == 0;
			lastRemovedDigit = uint8(vr % 10);
			vr /= 10, vp /= 10, vm /= 10;
			removed++;
		}
		if(vmIsTrailingZeros) while(vm % 10 == 0)
		{
			vrIsTrailingZeros &= lastRemovedDigit == 0;
			lastRemovedDigit = uint8(vr % 10);
			vr /= 10, vp /= 10, vm /= 10;
			removed++;
		}
		if(vrIsTrailingZeros && lastRemovedDigit == 5 && vr % 2 == 0)
		{
			// Round even if the exact number is .....50..0.
			lastRemovedDigit = 4;
		}
		// We need to take vr + 1 if vr is outside bounds or we need to round up.
		output = vr;
		if(vr == vm && (!acceptBounds || !vmIsTrailingZeros) || lastRemovedDigit >= 5) output++;
	}
	const int exp = e10 + removed;
	return GenericFloat<uint32, 10>(Construct, output, int16(exp), false);
}

constexpr uint64 DOUBLE_POW5_INV_SPLIT2[15][2] = {
	{                    1u, 2305843009213693952u },
	{  5955668970331000884u, 1784059615882449851u },
	{  8982663654677661702u, 1380349269358112757u },
	{  7286864317269821294u, 2135987035920910082u },
	{  7005857020398200553u, 1652639921975621497u },
	{ 17965325103354776697u, 1278668206209430417u },
	{  8928596168509315048u, 1978643211784836272u },
	{ 10075671573058298858u, 1530901034580419511u },
	{   597001226353042382u, 1184477304306571148u },
	{  1527430471115325346u, 1832889850782397517u },
	{ 12533209867169019542u, 1418129833677084982u },
	{  5577825024675947042u, 2194449627517475473u },
	{ 11006974540203867551u, 1697873161311732311u },
	{ 10313493231639821582u, 1313665730009899186u },
	{ 12701016819766672773u, 2032799256770390445u }
};
constexpr uint32 POW5_INV_OFFSETS[19] = {
	0x54544554, 0x04055545, 0x10041000, 0x00400414, 0x40010000, 0x41155555,
	0x00000454, 0x00010044, 0x40000000, 0x44000041, 0x50454450, 0x55550054,
	0x51655554, 0x40004000, 0x01000001, 0x00010500, 0x51515411, 0x05555554,
	0x00000000
};

constexpr uint64 DOUBLE_POW5_SPLIT2[13][2] = {
	{                    0u, 1152921504606846976u },
	{                    0u, 1490116119384765625u },
	{  1032610780636961552u, 1925929944387235853u },
	{  7910200175544436838u, 1244603055572228341u },
	{ 16941905809032713930u, 1608611746708759036u },
	{ 13024893955298202172u, 2079081953128979843u },
	{  6607496772837067824u, 1343575221513417750u },
	{ 17332926989895652603u, 1736530273035216783u },
	{ 13037379183483547984u, 2244412773384604712u },
	{  1605989338741628675u, 1450417759929778918u },
	{  9630225068416591280u, 1874621017369538693u },
	{   665883850346957067u, 1211445438634777304u },
	{ 14931890668723713708u, 1565756531257009982u }
};
constexpr uint32 POW5_OFFSETS[21] = {
	0, 0, 0, 0, 0x40000000, 0x59695995,
	0x55545555, 0x56555515, 0x41150504, 0x40555410, 0x44555145, 0x44504540,
	0x45555550, 0x40004000, 0x96440440, 0x55565565, 0x54454045, 0x40154151,
	0x55559155, 0x51405555, 0x00000105
};

constexpr uint64 DOUBLE_POW5_TABLE[] = {
	1, 5, 25, 125, 625, 3125, 15625, 78125, 390625,
	1953125, 9765625, 48828125, 244140625, 1220703125, 6103515625,
	30517578125, 152587890625, 762939453125, 3814697265625,
	19073486328125, 95367431640625, 476837158203125,
	2384185791015625, 11920928955078125, 59604644775390625,
	298023223876953125 //, 1490116119384765625
};

constexpr Array<uint64, 2> double_computePow5(uint32 i)
{
	enum: uint32 {Pow5TableSize = Length(DOUBLE_POW5_TABLE)};
	const uint32 base = i / Pow5TableSize;
	const uint32 base2 = base * Pow5TableSize;
	const uint32 offset = i - base2;
	const auto mul = DOUBLE_POW5_SPLIT2[base];
	if(offset == 0) return {mul[0], mul[1]};
	const uint64 m = DOUBLE_POW5_TABLE[offset];
	uint64 high1 = 0;
	const uint64 low1 = Mul64To128(m, mul[1], Out(high1));
	uint64 high0 = 0;
	const uint64 low0 = Mul64To128(m, mul[0], Out(high0));
	const uint64 sum = high0 + low1;
	if(sum < high0) high1++; // overflow into high1
	// high1 | sum | low0
	const int delta = BitWidthOfPow<5>(i) - BitWidthOfPow<5>(base2);
	return {
		ShiftRight128UpTo64(low0, sum, delta) + ((POW5_OFFSETS[i / 16] >> ((i % 16) << 1)) & 3),
		ShiftRight128UpTo64(sum, high1, delta)
	};
}

// Computes 5^-i in the form required by Ryu, and stores it in the given pointer.
constexpr Array<uint64, 2> double_computeInvPow5(uint32 i)
{
	enum: uint32 {Pow5TableSize = Length(DOUBLE_POW5_TABLE)};
	const uint32 base = (i + Pow5TableSize - 1) / Pow5TableSize;
	const uint32 base2 = base * Pow5TableSize;
	const uint32 offset = base2 - i;
	const auto mul = DOUBLE_POW5_INV_SPLIT2[base]; // 1/5^base2
	if(offset == 0) return {mul[0], mul[1]};
	const uint64 m = DOUBLE_POW5_TABLE[offset];
	uint64 high1 = 0;
	const uint64 low1 = Mul64To128(m, mul[1], Out(high1));
	uint64 high0 = 0;
	const uint64 low0 = Mul64To128(m, mul[0] - 1, Out(high0));
	const uint64 sum = high0 + low1;
	if(sum < high0) high1++; // overflow into high1
	// high1 | sum | low0
	const int delta = BitWidthOfPow<5>(base2) - BitWidthOfPow<5>(i);
	return {
		ShiftRight128UpTo64(low0, sum, delta) + 1 + ((POW5_INV_OFFSETS[i / 16] >> ((i % 16) << 1)) & 3),
		ShiftRight128UpTo64(sum, high1, delta)
	};
}

INTRA_NOINLINE constexpr GenericFloat<uint64, 10> BinaryDoubleToDecimal(uint64 mantissa, int exponentOf2)
{
	INTRA_PRECONDITION((mantissa >> MantissaLenOf<float>) == 0);
	INTRA_PRECONDITION(-ExponentBiasOf<float> - NumMantissaExplicitBitsOf<float> < exponentOf2);
	INTRA_PRECONDITION(exponentOf2 <= ExponentBiasOf<float> - NumMantissaExplicitBitsOf<float>);

	const auto mulShiftAll64 = [](uint64 m, Array<uint64, 2> mul, int j, Out<uint64> vp, Out<uint64> vm, uint32 mmShift) {
		vp = MulShift64(4 * m + 2, mul[0], mul[1], j);
		vm = MulShift64(4 * m - 1 - mmShift, mul[0], mul[1], j);
		return MulShift64(4 * m, mul[0], mul[1], j);
	};

	// We subtract 2 so that the bounds computation has 2 additional bits.
	exponentOf2 -= 2;
	const bool even = (mantissa & 1) == 0;
	const bool acceptBounds = even;

	// Step 2: Determine the interval of valid decimal representations.
	const uint64 mv = 4 * mantissa;
	const uint64 mmShift = (mantissa & NumBitsToMask<uint64>(NumMantissaExplicitBitsOf<float>)) != 0 ||
		exponentOf2 + ExponentBiasOf<float> + NumMantissaExplicitBitsOf<float> == 1;

	// Step 3: Convert to a decimal power base using 128-bit arithmetic.
	uint64 vr = 0, vp = 0, vm = 0;
	int32 e10 = 0;
	bool vmIsTrailingZeros = false, vrIsTrailingZeros = false;
	if(exponentOf2 >= 0)
	{
		const uint32 q = FloorLog10OfPow2Approx(uint32(exponentOf2)) - (exponentOf2 > 3);
		e10 = int32(q);
		const int32 k = DOUBLE_POW5_INV_BITCOUNT + BitWidthOfPow<5>(q) - 1;
		const int32 i = -exponentOf2 + int32(q) + k;
		const auto pow5 = double_computeInvPow5(q);
		vr = mulShiftAll64(uint64(exponentOf2), pow5, i, Out(vp), Out(vm), mmShift);
		if(q <= 21)
		{
			const uint32 mvMod5 = FastModByConst<5u>(mv);
			if(mvMod5 == 0) vrIsTrailingZeros = IsMultipleOfPowerOf(mv, 5, int(q));
			else if(acceptBounds) vmIsTrailingZeros = IsMultipleOfPowerOf(mv - 1 - mmShift, 5, int(q));
			else vp -= IsMultipleOfPowerOf(mv + 2, 5, int(q));
		}
	}
	else
	{
		const uint32 q = FloorLog10OfPow5Approx(uint32(-exponentOf2)) - (-exponentOf2 > 1);
		e10 = int32(q) + exponentOf2;
		const uint32 i = uint32(-e10);
		const int32 k = BitWidthOfPow<5>(uint32(i)) - DOUBLE_POW5_BITCOUNT;
		const int32 j = int32(q) - k;
		const auto pow5 = double_computePow5(i);
		vr = mulShiftAll64(mantissa, pow5, j, Out(vp), Out(vm), mmShift);
		if(q <= 1)
		{
			// {vr,vp,vm} is trailing zeros if {mv,mp,mm} has at least q trailing 0 bits.
			// mv = 4 * mantissa, so it always has at least two trailing 0 bits.
			vrIsTrailingZeros = true;
			if (acceptBounds) vmIsTrailingZeros = mmShift == 1;
			else vp--; // mp = mv + 2, so it always has at least one trailing 0 bit.
		}
		else if(q < 63) vrIsTrailingZeros = IsMultipleOfPowerOf(mv, 2, int(q));
	}

	// Step 4: Find the shortest decimal representation in the interval of valid representations.
	int removed = 0;
	uint8 lastRemovedDigit = 0;
	uint64 output = 0;
	// On average, we remove ~2 digits.
	if(!vmIsTrailingZeros && !vrIsTrailingZeros && !Config::DisableAllOptimizations)
	{
		// Specialized for the common case (~99.3%). Percentages below are relative to this.
		bool roundUp = false;
		const uint64 vpDiv100 = FastDivByConstTrunc<100u>(vp);
		const uint64 vmDiv100 = FastDivByConstTrunc<100u>(vm);
		if(vpDiv100 > vmDiv100) // Optimization: remove two digits at a time (~86.2%).
		{
			const uint64 vrDiv100 = FastDivByConstTrunc<100u>(vr);
			const uint32 vrMod100 = uint32(vr) - 100 * uint32(vrDiv100);
			roundUp = vrMod100 >= 50;
			vr = vrDiv100, vp = vpDiv100, vm = vmDiv100;
			removed += 2;
		}
		// Loop iterations below (approximately), without optimization above:
		// 0: 0.03%, 1: 13.8%, 2: 70.6%, 3: 14.0%, 4: 1.40%, 5: 0.14%, 6+: 0.02%
		// Loop iterations below (approximately), with optimization above:
		// 0: 70.6%, 1: 27.8%, 2: 1.40%, 3: 0.14%, 4+: 0.02%
		for(;;)
		{
			const uint64 vpDiv10 = FastDivByConstTrunc<10u>(vp);
			const uint64 vmDiv10 = FastDivByConstTrunc<10u>(vm);
			if(vpDiv10 <= vmDiv10) break;
			const uint64 vrDiv10 = FastDivByConstTrunc<10u>(vr);
			const uint32 vrMod10 = uint32(vr) - 10*uint32(vrDiv10);
			roundUp = vrMod10 >= 5;
			vr = vrDiv10, vp = vpDiv10, vm = vmDiv10;
			removed++;
		}
		// We need to take vr + 1 if vr is outside bounds or we need to round up.
		output = vr + (vr == vm || roundUp);
	}
	else // General case, which happens rarely (~0.7%).
	{
		for (;;)
		{
			const uint64 vpDiv10 = FastDivByConstTrunc<10u>(vp);
			const uint64 vmDiv10 = FastDivByConstTrunc<10u>(vm);
			if(vpDiv10 <= vmDiv10) break;
			const uint32 vmMod10 = uint32(vm) - 10*uint32(vmDiv10);
			const uint64 vrDiv10 = FastDivByConstTrunc<10u>(vr);
			const uint32 vrMod10 = uint32(vr) - 10*uint32(vrDiv10);
			vmIsTrailingZeros &= vmMod10 == 0;
			vrIsTrailingZeros &= lastRemovedDigit == 0;
			lastRemovedDigit = uint8(vrMod10);
			vr = vrDiv10, vp = vpDiv10, vm = vmDiv10;
			removed++;
		}

		if(vmIsTrailingZeros) for(;;)
		{
			const uint64 vmDiv10 = FastDivByConstTrunc<10u>(vm);
			const uint32 vmMod10 = uint32(vm) - 10*uint32(vmDiv10);
			if(vmMod10 != 0) break;
			const uint64 vpDiv10 = FastDivByConstTrunc<10u>(vp);
			const uint64 vrDiv10 = FastDivByConstTrunc<10u>(vr);
			const uint32 vrMod10 = uint32(vr) - 10*uint32(vrDiv10);
			vrIsTrailingZeros &= lastRemovedDigit == 0;
			lastRemovedDigit = uint8(vrMod10);
			vr = vrDiv10, vp = vpDiv10, vm = vmDiv10;
			removed++;
		}
		if(vrIsTrailingZeros && lastRemovedDigit == 5 && vr % 2 == 0)
			lastRemovedDigit = 4; // Round even if the exact number is .....50..0.
		// We need to take vr + 1 if vr is outside bounds or we need to round up.
		output = vr + ((vr == vm && (!acceptBounds || !vmIsTrailingZeros)) || lastRemovedDigit >= 5);
	}
	return GenericFloat<uint64, 10>(Construct, output, int16(e10 + removed), false);
}

}
} INTRA_END
