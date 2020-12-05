#pragma once

/** Header file for advanced float manipulation:
  1) Infinity
  2) NaN
  3) mantissa and exponent extraction
  4) binary to decimal float conversion
  5) half float class
*/

#include "Intra/Type.h"

INTRA_BEGIN
INTRA_IGNORE_WARNS_MSVC(4310) //cast truncates constant value

template<size_t MinSize> using TUnsignedIntMin = TPackAt<MinSize-1,
	uint8, uint16, uint32, uint32, uint64, uint64, uint64, uint64>;
template<size_t MinSize> using TSignedIntMin = TPackAt<MinSize-1,
	int8, int16, int32, int32, int64, int64, int64, int64>;

template<size_t MinSize, bool Signed> using TIntMin = TSelect<
	TSignedIntMin<MinSize>,
	TUnsignedIntMin<MinSize>,
	Signed>;

template<size_t MinSize> using TFloatMin = TPackAt<(MinSize-1)/4, float, double>;

template<typename T> using TToUnsigned = TUnsignedIntMin<CIntegral<T>? sizeof(T): 0xBad>;
template<typename T> using TToSigned = TSelect<TSignedIntMin<CIntegral<T>? sizeof(T): 0>, T, CIntegral<T>>;

namespace z_D {
template<typename T, bool = CArithmetic<T>> struct TToIntegral_ {};
template<typename T> struct TToIntegral_<T, true> {using _ = TIntMin<sizeof(T), CSigned<T>>;};
}
template<typename T> using TToIntegral = typename z_D::TToIntegral_<TRemoveConstRef<T>>::_;

#ifndef __FLT_MAX__ //MSVC
#define __FLT_MAX__ 3.402823466e+38f
#define __DBL_MAX__ 1.7976931348623158e+308
#define __LDBL_MAX__ 1.7976931348623158e+308l
#define __FLT_MIN__ 1.175494351e-38f
#define __DBL_MIN__ 2.2250738585072014e-308
#define __LDBL_MIN__ 2.2250738585072014e-308l
#define __FLT_DENORM_MIN__ 1.401298464e-45f
#define __DBL_DENORM_MIN__ 4.9406564584124654e-324
#define __LDBL_DENORM_MIN__ 4.9406564584124654e-324l
#define __FLT_EPSILON__ 1.192092896e-7f
#define __DBL_EPSILON__ 2.2204460492503131e-16
#define __LDBL_EPSILON__ 2.2204460492503131e-16l
#endif

constexpr float Infinity = __builtin_huge_valf();

template<typename T> constexpr T MaxValueOf = [] {
	if constexpr(CFixedPoint<T>) return T::Max();
	else if constexpr(CScalar<T>) return VMapByType<TUnqual<T>,
		CSigned<char>? char(127): char(255),
	#ifdef __cpp_char8_t
		char8_t(255),
	#endif
		char16_t(65535),
		char32_t(4294967295),
		uint8(255),
		uint16(65535),
		uint32(4294967295),
		uint64(18446744073709551615),
		int8(127),
		int16(32767),
		int32(2147483647),
		int64(9223372036854775807),
		float(__FLT_MAX__),
		double(__DBL_MAX__),
		static_cast<long double>(__LDBL_MAX__)
	>;
}();

template<typename T> constexpr T MinValueOf = [] {
	if constexpr(CFixedPoint<T>) return T::Min();
	else if constexpr(CUnsignedIntegral<T>) return T(0);
	else if constexpr(CArithmetic<T>) return VMapByType<TUnqual<T>,
		char(-128),
		int8(-128),
		int16(-32768),
		int32(-2147483648),
		int64(-9223372036854775807-1),
		float(-__FLT_MAX__),
		double(-__DBL_MAX__),
		static_cast<long double>(-__LDBL_MAX__)
	>;
}();

template<typename T> constexpr auto SignBitMaskOf = [] {
	if constexpr(MinValueOf<T> >= 0) return 0;
	return TUnsignedIntMin<sizeof(T)>(1) << (sizeof(T)*8 - 1);
}();

template<typename T> constexpr T MaxValueOfWithInfinity = CFloatingPoint<T>? T(Infinity): MaxValueOf<T>;
template<typename T> constexpr T MinValueOfWithInfinity = CFloatingPoint<T>? T(-Infinity): MinValueOf<T>;

template<typename T> constexpr T MinNormPositiveValueOf = [] {
	if constexpr(CFixedPoint<T>) return T::Epsilon();
	else if constexpr(CIntegral<T>) return 1;
	else if constexpr(CFloatingPoint<T>) return VMapByType<TUnqual<T>,
		float(__FLT_MIN__),
		double(__DBL_MIN__),
		static_cast<long double>(__LDBL_MIN__)
	>;
}();

/// Get the number of mantissa bits in floating point representation. Equals to 0 for other types.
template<typename T> constexpr int NumMantissaExplicitBitsOf = [] {
	if constexpr(CFloatingPoint<T>) return sizeof(T) == sizeof(float)? 23: 52;
}();

/// Get mantissa length in bits including implicit 1.
/// Value is valid for basic arithmetic types and FixedPoint.
template<typename T> constexpr int MantissaLenOf = [] {
	if constexpr(CFloatingPoint<T>)
		return NumMantissaExplicitBitsOf<T> + 1;
	else if constexpr(CIntegral<T> || CFixedPoint<T>)
		return sizeof(T)*8 - CSigned<T>;
}();

template<typename T> constexpr int ExponentLenOf = [] {
	if constexpr(CFloatingPoint<T>)
		return sizeof(T) == sizeof(float)? 8: 11;
	else if constexpr(CIntegral<T> || CFixedPoint<T>) return 0;
}();

template<typename T> constexpr int ExponentBiasOf = [] {
	if constexpr(CFloatingPoint<T>)
		return sizeof(T) == sizeof(float)? 127: 1023;
	else if constexpr(CIntegral<T>) return 0;
}();

/// The difference between 1.0 and the next representable value of the given type 
template<typename T> constexpr T EpsilonOf = [] {
	if constexpr(CIntegral<T>) return 1;
	else if constexpr(CFixedPoint<T>) return T::Epsilon();
	else if constexpr(CFloatingPoint<T>) return VMapByType<TUnqual<T>,
		float(__FLT_EPSILON__),
		double(__DBL_EPSILON__),
		static_cast<long double>(__LDBL_EPSILON__)
	>;
}();

template<typename T> constexpr auto IntegerRangeMax = [] {
	if constexpr(CIntegral<T>) return MaxValueOf<T>;
	else if constexpr(CFixedPoint<T>) return EpsilonOf<T> <= 1? uint64(T::Max()): 0;
	else if constexpr(CFloatingPoint<T>) return 1ll << MantissaLenOf<T>;
	else return 0;
}();
template<typename T> constexpr auto IntegerRangeMin = [] {
	if constexpr(CIntegral<T>) return MinValueOf<T>;
	else if constexpr(CFixedPoint<T>) return EpsilonOf<T> <= 1? int64(T::Min()): 0;
	else if constexpr(CFloatingPoint<T>) return -IntegerRangeMax<T>;
	else return 0;
}();

#if INTRA_CONSTEXPR_TEST
static_assert(MaxValueOf<double> == __DBL_MAX__);
static_assert(IntegerRangeMax<int16> == 32767);
static_assert(IntegerRangeMin<int16> == -32768);
static_assert(IntegerRangeMax<float> == 16777216);
static_assert(IntegerRangeMin<float> == -16777216);
static_assert(IntegerRangeMax<double> == 9007199254740992 || sizeof(double) == sizeof(float));
#endif

template<typename From, typename To> concept CNoWrapConvertible = [] {
	if constexpr(CArithmetic<From> && CArithmetic<To>)
		return MaxValueOfWithInfinity<From> <= MaxValueOfWithInfinity<To> &&
			MinValueOfWithInfinity<From> >= MinValueOfWithInfinity<To>;
	return false;
}();

template<typename From, typename To> concept CLosslessConvertible = [] {
	if constexpr(CIntegral<From>)
		return IntegerRangeMax<From> <= IntegerRangeMax<To> &&
			IntegerRangeMin<From> >= IntegerRangeMin<To>;
	if constexpr(CReal<From>)
		return CReal<To> &&
			MaxValueOf<From> <= MaxValueOf<To> &&
			MinValueOf<From> >= MinValueOf<To> &&
			EpsilonOf<From> <= EpsilonOf<To>;
	return false;
}();

//TODO: modify these concepts using requires expressions to support any custom type
template<typename T> concept CSupportsIntegers = IntegerRangeMin<T> != IntegerRangeMax<T>;
template<typename T> concept CSupportsSignedIntegers = CSupportsIntegers<T> && IntegerRangeMin<T> < 0;
template<typename T> concept CNumber = CArithmeticType<T> && CConvertibleTo<T, double>;

template<typename R, typename T> class TCommonRange
{
	static_assert(CUnqualedIntegral<R> && CUnqualedIntegral<T>);
	static constexpr R calcMin()
	{
		if constexpr(CUnsignedIntegral<R> || CUnsignedIntegral<T>) return 0;
		else if constexpr(sizeof(R) >= sizeof(T)) return R(MinValueOf<T>);
		else return MinValueOf<R>;
	}
	static constexpr R calcMax()
	{
		constexpr bool isNarrowingCast = sizeof(R) < sizeof(T) ||
			sizeof(R) == sizeof(T) && CUnqualedUnsignedIntegral<R> != CUnqualedUnsignedIntegral<T>;
		if constexpr(!isNarrowingCast || sizeof(R) > sizeof(T) || CUnqualedUnsignedIntegral<R>) return R(MaxValueOf<T>);
		else return MaxValueOf<R>;
	}
public:
	static constexpr R Min = calcMin();
	static constexpr R Max = calcMax();
};

namespace z_D {
template<class To, class From> INTRA_FORCEINLINE
#ifdef INTRA_CONSTEXPR_BITCAST_SUPPORT
#define INTRAZ_D_BITCAST_CONSTEXPR 1
constexpr
#else
#define INTRAZ_D_BITCAST_CONSTEXPR 0
#endif
To utilBitCast(const From& from) noexcept
{
#ifdef INTRA_CONSTEXPR_BITCAST_SUPPORT
	return __builtin_bit_cast(To, from);
#else
	To to;
	__builtin_memcpy(&to, &from, sizeof(to));
	return to;
#endif
}
}

template<typename T> requires CIntegral<T>
[[nodiscard]] constexpr bool CanMultiplyWithoutOverflow(T a, T b)
{
#if defined(__GNUC__) || defined(__clang__) || defined(__INTEL_COMPILER)
#ifdef __INTEL_COMPILER
	if(IsConstantEvaluated(a, b))
#endif
		return !__builtin_mul_overflow(x, y, &x);
#endif
#if !defined(__GNUC__) && !defined(__clang__) || defined(__INTEL_COMPILER)
	using MaxType = TIntMin<sizeof(int64), CSigned<T>>;
	if(MaxType(MaxValueOf<T>) < MaxValueOf<MaxType>)
	{
		const auto mul = MaxType(a)*MaxType(b);
		return mul == MaxType(T(mul));
	}
	if(a == 0) return true;
	const auto apos = TToUnsigned<T>(a < 0? -a: a);
	const auto bpos = TToUnsigned<T>(b < 0? -b: b);
	const auto mulPos = apos*bpos;
	return mulPos <= TToUnsigned<T>(MaxValueOf<T>) && apos == mulPos / apos;
#endif
}

template<typename T> requires CUnsignedIntegral<T>
[[nodiscard]] constexpr T CeilToNextPow2(T v)
{
	v--;
	v |= v >> 1u;
	v |= v >> 2u;
	v |= v >> 4u;
	v |= v >> 8u;
	if constexpr(sizeof(v) >= sizeof(uint32))
	{
		v |= v >> 16u;
		if constexpr(sizeof(v) >= sizeof(uint64)) v |= v >> 32u;
	}
	return ++v;
}

/// Integer logarithm of base 2, rounded to a smaller integer.
[[nodiscard]] constexpr byte Log2i(uint32 x)
{
	if(x == 0) return byte(255);
	uint32 n = 31;
	if(x <= 0x0000ffffu) n -= 16, x <<= 16u;
	if(x <= 0x00ffffffu) n -= 8, x <<= 8u;
	if(x <= 0x0fffffffu) n -= 4, x <<= 4u;
	if(x <= 0x3fffffffu) n -= 2, x <<= 2u;
	if(x <= 0x7fffffffu) n--;
	return byte(n);
}

template<typename T> requires CIntegral<T>
[[nodiscard]] constexpr bool IsPow2(T x) {return x != 0 && ((x & (x - 1)) == 0);}


constexpr class TNaN
{
    static constexpr bool isNan(float x) noexcept
    {
#if defined(_M_FP_FAST) || defined(__FAST_MATH__)
        if(IsConstantEvaluated(x)) return false;
        return (z_D::utilBitCast<uint32>(x) << 1) > 0xFF000000u;
#else
        return x != x;
#endif
    }
public:
    template<typename T> requires CFloatingPoint<T>
	[[nodiscard]] constexpr bool operator==(T x) const noexcept {return isNan(x);}

	template<typename T> requires CFloatingPoint<T>
	[[nodiscard]] constexpr bool operator!=(T x) const noexcept {return !isNan(x);}

	template<typename T> requires CFloatingPoint<T>
	friend constexpr bool operator==(T x, TNaN) noexcept {return isNan(x);}

	template<typename T> requires CFloatingPoint<T>
	friend constexpr bool operator!=(T x, TNaN) noexcept {return !isNan(x);}

	template<typename T> requires CFloatingPoint<T>
    constexpr operator T() const noexcept {return T(__builtin_nan(""));}
} NaN;


namespace z_D {
template<typename Float> constexpr int ILog2_(Float x) noexcept
{
#ifdef INTRA_MATH_CONSTEXPR_SUPPORT
	return __builtin_ilogbl(x);
#else
	if(x < 0) x = -x;
	for(int exponent = 0;;)
	{
		if(x >= 2) exponent++, x *= Float(0.5);
		else if(x < 1) exponent--, x *= 2;
		else return exponent;
	}
#endif
}
template<typename T> constexpr T Pow2Int(int y) noexcept
{
	constexpr auto BitsPerStep = sizeof(size_t)*8 - 1;
	T res = 1;
	int p = y < 0? -y: y;
	while(p >= BitsPerStep) res *= size_t(1) << BitsPerStep, p -= BitsPerStep;
	if(p) res *= size_t(1) << p;
	return y >= 0? res: 1 / res;
}
template<typename Float> constexpr Float Scalbn(Float value, int exponent) noexcept
{
#ifdef INTRA_MATH_CONSTEXPR_SUPPORT
	return Float(__builtin_scalbnl(value, exponent));
#else
	return value*Pow2Int<Float>(exponent);
#endif
}
}

template<typename Float> requires CFloatingPoint<Float>
[[nodiscard]] constexpr int ExtractBiasedExponent(Float x) noexcept
{
	if(!IsConstantEvaluated(x) || INTRAZ_D_BITCAST_CONSTEXPR)
	{
		constexpr int exponentMask = (1 << ExponentLenOf<Float>) - 1;
		const auto floatBits = z_D::utilBitCast<TUnsignedIntMin<sizeof(Float)>>(x);
		return int(floatBits >> NumMantissaExplicitBitsOf<Float>) & exponentMask;
	}
	return ExponentBiasOf<Float> + z_D::ILog2_(x);
}

template<typename Float> requires CFloatingPoint<Float>
[[nodiscard]] constexpr int ExtractExponent(Float x) noexcept
{
	return ExtractBiasedExponent(x) - ExponentBiasOf<Float>;
}

template<typename Float> requires CFloatingPoint<Float>
constexpr TUnsignedIntMin<sizeof(Float)> ExtractMantissa(Float x) noexcept
{
	using UInt = TUnsignedIntMin<sizeof(Float)>;
	if(!IsConstantEvaluated(x) || INTRAZ_D_BITCAST_CONSTEXPR)
		return z_D::utilBitCast<UInt>(x) & UInt(IntegerRangeMax<Float> - 1);
	if(x == Infinity || x == -Infinity) return 0;
	if(x < 0) x = -x;
	x = z_D::Scalbn(x, -z_D::ILog2_(x)) - 1;
	return UInt(x*(UInt(1) << NumMantissaExplicitBitsOf<Float>));
}

template<typename Float> requires CFloatingPoint<Float>
constexpr Float ComposeFloat(TUnsignedIntMin<sizeof(Float)> mantissa, int exponent, bool negative) noexcept
{
	using UInt = decltype(mantissa);
	if(!IsConstantEvaluated(mantissa, exponent, negative) || INTRAZ_D_BITCAST_CONSTEXPR)
		return z_D::utilBitCast<Float>(mantissa |
			(UInt(ExponentBiasOf<Float> + exponent) << NumMantissaExplicitBitsOf<Float>) |
			(UInt(negative) << (sizeof(Float)*8-1)));

	Float res = 1 + mantissa*(Float(1) / (UInt(1) << NumMantissaExplicitBitsOf<Float>));
	res = z_D::Scalbn(res, exponent);
	return negative? -res: res;
}
#undef INTRAZ_D_BITCAST_CONSTEXPR


// A floating decimal representing Mantissa * 10^Exponent.
template<typename MantissaT> struct DecimalFloat
{
	MantissaT Mantissa;
	int Exponent;
};

struct HalfFloat
{
	HalfFloat() = default;

	constexpr explicit HalfFloat(decltype(Construct), uint16 s): AsUint16(s) {}
	HalfFloat& operator=(const HalfFloat& rhs) = default;

	template<typename T> requires CReal<T> && (!CSame<TRemoveConstRef<T>, HalfFloat>)
	constexpr explicit HalfFloat(T f): AsUint16(fromFloat(float(f))) {}

	template<typename T> requires CReal<T>
	[[nodiscard]] constexpr operator T() const {return toFloat(AsUint16);}

	template<typename T> requires CReal<T> || CSame<T, HalfFloat>
	constexpr HalfFloat& operator+=(T rhs) {return *this = HalfFloat(*this + float(rhs));}

	template<typename T> requires CReal<T> || CSame<T, HalfFloat>
	constexpr HalfFloat& operator-=(T rhs) {return *this = HalfFloat(*this - float(rhs));}

	template<typename T> requires CReal<T> || CSame<T, HalfFloat>
	constexpr HalfFloat& operator*=(T rhs) {return *this = HalfFloat(*this * float(rhs));}

	template<typename T> requires CReal<T> || CSame<T, HalfFloat>
	constexpr HalfFloat& operator/=(T rhs) {return *this = HalfFloat(*this / float(rhs));}

	uint16 AsUint16 = 0;

private:
	static constexpr uint16 fromFloat(float f)
	{
		return uint16(
			(ExtractMantissa(f) >> (NumMantissaExplicitBitsOf<float> - 10)) |
			((ExtractExponent(f) + 15) << 10) |
			((f < 0) << 15)
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

template<typename T> requires CUnsignedIntegral<T>
[[nodiscard]] int Count1Bits(T mask)
{
	if constexpr(sizeof(T) == sizeof(long long)) return __builtin_popcountll(mask);
	else if constexpr(sizeof(T) > sizeof(int)) return __builtin_popcountl(mask);
	else return __builtin_popcount(mask);
}

template<typename T> requires CIntegral<T>
[[nodiscard]] int FindBitPosition(T mask)
{
	return Count1Bits((mask & (~mask + 1)) - 1);
}

template<typename T> requires CUnsignedIntegral<T>
[[nodiscard]] constexpr T NumBitsToMask(int bitCount)
{
	return T(-(bitCount != 0)) & (T(-1) >> ((sizeof(T) * 8) - bitCount));
	return bitCount >= sizeof(T)*8? T(-1): (1u << bitCount)-1u;
}

template<typename T> requires CIntegral<T>
[[nodiscard]] constexpr T RotateBitsLeft(TExplicitType<T> x, int n)
{
	const auto ux = TToUnsigned<T>(x);
	return (ux << n)|(ux >> (sizeof(T)*8 - n));
}

template<typename T> requires CIntegral<T>
[[nodiscard]] constexpr T RotateBitsRight(TExplicitType<T> x, int n)
{
	const auto ux = TToUnsigned<T>(x);
	return (ux >> n)|(ux << (sizeof(T)*8 - n));
}

template<typename T> requires CIntegral<T>
[[nodiscard]] constexpr T ReverseBits(TExplicitType<T> x)
{
#ifdef __clang__
	return T(__builtin_bitreverse64(x));
#else
	auto ux = TToUnsigned<T>(x);
	int bits = sizeof(x)*8; 
	auto mask = MaxValueOf<TToUnsigned<T>>;
	while(bits >>= 1)
	{
		mask ^= mask << bits;
		ux = TToUnsigned<T>(((ux & ~mask) >> bits) | ((ux & mask) << bits));
	}
	return T(ux);
#endif
}

#if INTRA_CONSTEXPR_TEST
static_assert(ExtractBiasedExponent(1/3.14f) == 125);
static_assert(ExtractBiasedExponent(3.14f) == 128);
static_assert(ExtractBiasedExponent(6.14f) == 129);
static_assert(ExtractBiasedExponent(3.14) == 1024);
static_assert(ExtractExponent(3.14f) == ExtractExponent(3.14));
static_assert(ReverseBits<uint8>(0b01100011) == 0b11000110);
static_assert(ReverseBits<uint32>(0b11000111101110101010101010001100) == 0b00110001010101010101110111100011);
#endif

INTRA_END
