#pragma once

#include <Math/Core.h>

namespace Intra { INTRA_BEGIN

template<CNumber T> constexpr T MaxFiniteValueOf = [] {
	if constexpr(CConst<T> || CVolatile<T>) return MaxFiniteValueOf<TUnqual<T>>;
	else if constexpr(CScalar<T>) return VMapByType<TUnqual<T>,
		CBasicSigned<char>? char(127): char(255),
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
	#ifdef __FLT_MAX__
		float(__FLT_MAX__),
		double(__DBL_MAX__),
		static_cast<long double>(__LDBL_MAX__)
	#else
		3.402823466e+38f,
		1.7976931348623158e+308,
		1.7976931348623158e+308l
	#endif
	>;
}();

template<CNumber T> constexpr T MinFiniteValueOf = [] {
	if constexpr(CConst<T> || CVolatile<T>) return MinValueOf<TUnqual<T>>;
	else if constexpr(CBasicUnsignedIntegral<T>) return T(0);
	else if constexpr(CBasicArithmetic<T>) return VMapByType<TUnqual<T>,
		char(-128),
		int8(-128),
		int16(-32768),
		int32(-2147483648),
		int64(-9223372036854775807-1),
		-MaxFiniteValueOf<float>,
		-MaxFiniteValueOf<double>,
		-MaxFiniteValueOf<long double>
	>;
}();

template<CNumber T> constexpr auto SignBitMaskOf = [] {
	if constexpr(MinValueOf<T> >= 0) return 0;
	else return TUnsignedIntOfSizeAtLeast<sizeof(T)>(1) << (SizeofInBits<T> - 1);
}();

template<CNumber T> constexpr T MaxValueOf = CBasicFloatingPoint<T>? T(Infinity): MaxFiniteValueOf<T>;
template<CNumber T> constexpr T MinValueOf = CBasicFloatingPoint<T>? T(-Infinity): MinFiniteValueOf<T>;

template<CNumber T> constexpr T MinNormPositiveValueOf = [] {
	if constexpr(CBasicIntegral<T>) return 1;
	else if constexpr(CBasicFloatingPoint<T>) return VMapByType<TUnqual<T>,
	#ifdef __FLT_MIN__
		float(__FLT_MIN__),
		double(__DBL_MIN__),
		static_cast<long double>(__LDBL_MIN__)
	#else
		1.175494351e-38f,
		2.2250738585072014e-308,
		2.2250738585072014e-308l
	#endif
	>;
}();

/// Get the number of mantissa bits in floating point representation. Equals to 0 for other types.
template<CNumber T> constexpr auto NumMantissaExplicitBitsOf = [] {
	if constexpr(CBasicFloatingPoint<T>) return sizeof(T) == sizeof(float)? 23: 52;
	else return 0;
}();

/// Get mantissa length in bits including implicit 1.
/// Value is valid for basic arithmetic types and Fixed.
template<CNumber T> constexpr auto MantissaLenOf = [] {
	if constexpr(CBasicFloatingPoint<T>) return NumMantissaExplicitBitsOf<T> + 1;
	else if constexpr(CBasicIntegral<T>) return SizeofInBits<T> - CBasicSigned<T>;
}();

template<CNumber T> constexpr auto ExponentLenOf = [] {
	if constexpr(CBasicFloatingPoint<T>) return sizeof(T) == sizeof(float)? 8: 11;
	else if constexpr(CBasicIntegral<T> || CFixedPoint<T>) return 0;
	else if constexpr(CValueWrapper<T>) return ExponentLenOf<TWrappedType<T>>;
}();

template<CNumber T> constexpr auto ExponentBiasOf = [] {
	if constexpr(CBasicFloatingPoint<T>) return sizeof(T) == sizeof(float)? 127: 1023;
	else if constexpr(CBasicIntegral<T>) return 0;
}();

/// The difference between 1.0 and the next representable value of the given type 
template<CNumber T, CNumber auto RangeMin = 0, CNumber auto RangeMax = 1> requires(RangeMin < RangeMax)
constexpr T MaxStepInRange = [] {
	if constexpr(CBasicIntegral<T>) return 1;
	else if constexpr(CBasicFloatingPoint<T>)
	{
		constexpr T eps1 = VMapByType<TUnqual<T>,
		#ifdef __FLT_EPSILON__
			float(__FLT_EPSILON__),
			double(__DBL_EPSILON__),
			static_cast<long double>(__LDBL_EPSILON__)
		#else
			1.192092896e-7f,
			2.2204460492503131e-16,
			2.2204460492503131e-16l
		#endif
		>;
		constexpr auto absMin = RangeMin < 0? -RangeMin: RangeMin;
		constexpr auto absMax = RangeMax < 0? -RangeMax: RangeMax;
		constexpr auto abs = Max(absMin, absMax);
		return eps1/2*CeilToPow2(abs);
	}
}();

template<CNumber T> constexpr auto IntegerRangeMax = [] {
	if constexpr(CBasicIntegral<T>) return MaxValueOf<T>;
	else if constexpr(CBasicFloatingPoint<T>) return 1ll << MantissaLenOf<T>;
	else return 0;
}();
template<CNumber T> constexpr auto IntegerRangeMin = [] {
	if constexpr(CBasicIntegral<T>) return MinValueOf<T>;
	else if constexpr(CBasicFloatingPoint<T>) return -IntegerRangeMax<T>;
	else return 0;
}();

#if INTRA_CONSTEXPR_TEST
#ifdef __DBL_MAX__
static_assert(MaxValueOf<double> == __DBL_MAX__);
#endif
static_assert(IntegerRangeMax<int16> == 32767);
static_assert(IntegerRangeMin<int16> == -32768);
static_assert(IntegerRangeMax<float> == 16777216);
static_assert(IntegerRangeMin<float> == -16777216);
static_assert(IntegerRangeMax<double> == 9007199254740992 || sizeof(double) == sizeof(float));
#endif


template<class W> concept CNumberWrapper = CValueWrapper<W> && CNumber<TWrappedType<W>>;

template<CNumberWrapper T> constexpr T MaxFiniteValueOf<T> = T(MaxFiniteValueOf<TWrappedType<T>>);
template<CNumberWrapper T> constexpr T MinFiniteValueOf<T> = T(MinFiniteValueOf<TWrappedType<T>>);
template<CNumberWrapper T> constexpr T SignBitMaskOf<T> = T(SignBitMaskOf<TWrappedType<T>>);
template<CNumberWrapper T> constexpr T MaxValueOf<T> = T(MaxValueOf<TWrappedType<T>>);
template<CNumberWrapper T> constexpr T MinValueOf<T> = T(MinValueOf<TWrappedType<T>>);
template<CNumberWrapper T> constexpr T MinNormPositiveValueOf<T> = T(MinNormPositiveValueOf<TWrappedType<T>>);
template<CNumberWrapper T> constexpr auto NumMantissaExplicitBitsOf<T> = NumMantissaExplicitBitsOf<TWrappedType<T>>;
template<CNumberWrapper T> constexpr auto MantissaLenOf<T> = MantissaLenOf<TWrappedType<T>>;
template<CNumberWrapper T> constexpr auto ExponentLenOf<T> = ExponentLenOf<TWrappedType<T>>;
template<CNumberWrapper T> constexpr auto ExponentBiasOf<T> = ExponentBiasOf<TWrappedType<T>>;

template<CNumberWrapper T, CNumber auto RangeMin, CNumber auto RangeMax> requires(RangeMin < RangeMax)
constexpr T MaxStepInRange<T, RangeMin, RangeMax> = T(MaxStepInRange<TWrappedType<T>, RangeMin, RangeMax>);

template<CNumberWrapper T> constexpr auto IntegerRangeMax<T> = IntegerRangeMax<TWrappedType<T>>;
template<CNumberWrapper T> constexpr auto IntegerRangeMin<T> = IntegerRangeMin<TWrappedType<T>>;

constexpr auto SafeCompare = []<CNumber T1, CNumber T2>(T1 t1, T2 t2) {
	//TODO: not implemented, implement to make it actually safe
	return Cmp(t1, t2);
};

template<typename From, typename To> concept CNoWrapConvertible = [] {
	if constexpr(!CConvertible<From, To>) return false;
	else if constexpr(CNumber<From> && CNumber<To>)
		return SafeCompare(MaxValueOfWithInfinity<From>, MaxValueOfWithInfinity<To>) <= 0 &&
			SafeCompare(MinValueOfWithInfinity<From>, MinValueOfWithInfinity<To>) >= 0;
	else return CSameUnqualRef<From, To>;
}();

template<typename From, typename To> concept CLosslessConvertible = [] {
	if constexpr(CBasicIntegral<From>)
		return SafeCompare(IntegerRangeMax<From>, IntegerRangeMax<To>) <= 0 &&
			SafeCompare(IntegerRangeMin<From>, IntegerRangeMin<To>) >= 0;
	if constexpr(CBasicFloatingPoint<From>)
		return CBasicFloatingPoint<To> &&
			SafeCompare(MaxValueOf<From>, MaxValueOf<To>) <= 0 &&
			SafeCompare(MinValueOf<From>, MinValueOf<To>) >= 0 &&
			SafeCompare(MaxStepInRange<From>, MaxStepInRange<To>) <= 0;
	return false;
}();

template<typename T> concept CDivisibleBy0 = CBasicFloatingPoint<T> || requires {T::TagDivisibleBy0::True;};

//TODO: modify these concepts using requires expressions to support any custom type
template<typename T> concept CSupportsIntegers = IntegerRangeMin<T> != IntegerRangeMax<T>;
template<typename T> concept CSupportsSignedIntegers = CSupportsIntegers<T> && IntegerRangeMin<T> < 0;
template<typename T> concept CIntegralNumber = CNumber<T> && MaxStepInRange<T> == T(uint64(MaxStepInRange<T>));

template<CBasicIntegral R, CBasicIntegral T> constexpr R CommonMinValue = [] {
	if constexpr(CBasicUnsignedIntegral<R> || CBasicUnsignedIntegral<T>) return 0;
	else if constexpr(sizeof(R) >= sizeof(T)) return R(MinValueOf<T>);
	else return MinValueOf<R>;
}();
template<CBasicIntegral R, CBasicIntegral T> constexpr R CommonMaxValue = [] {
	constexpr bool isNarrowingCast = sizeof(R) < sizeof(T) ||
		sizeof(R) == sizeof(T) && CBasicUnsignedIntegral<R> != CBasicIntegral<T>;
	if constexpr(!isNarrowingCast || sizeof(R) > sizeof(T) || CBasicIntegral<R>) return R(MaxValueOf<T>);
	else return MaxValueOf<R>;
}();

template<CNumber auto Max> requires(SafeCompare(Max, MaxValueOf<uint64>) <= 0)
using TBasicUnsignedIntegerWithRange = TUnsignedIntOfSizeAtLeast<
	SafeCompare(Max, MaxValueOf<uint8>) <= 0? sizeof(uint8):
	SafeCompare(Max, MaxValueOf<uint16>) <= 0? sizeof(uint16):
	SafeCompare(Max, MaxValueOf<uint32>) <= 0? sizeof(uint32):
	sizeof(uint64)>;

template<CNumber auto Min, CNumber auto Max> requires(
	SafeCompare(MinValueOf<int64>, Min) <= 0 &&
	SafeCompare(Max, MaxValueOf<int64>) <= 0 &&
	SafeCompare(Min, Max) <= 0
)
using TBasicSignedIntegerWithRange = TSignedIntOfSizeAtLeast<
	SafeCompare(Max, MaxValueOf<int8>) <= 0? sizeof(int8):
	SafeCompare(Max, MaxValueOf<int16>) <= 0? sizeof(int16):
	SafeCompare(Max, MaxValueOf<int32>) <= 0? sizeof(int32):
	sizeof(int64)>;

namespace z_D {
template<auto Min, auto Max> struct TBasicNumberWithRange_: TType<double> {};
template<auto Min, auto Max> requires(Min >= 0 && Max <= MaxValueOf<uint64>)
struct TBasicNumberWithRange_<Min, Max>: TType<TBasicUnsignedIntegerWithRange<Max>> {};
template<auto Min, auto Max> requires(Min < 0 && Max <= MaxValueOf<int64>)
struct TBasicNumberWithRange_<Min, Max>: TType<TBasicSignedIntegerWithRange<Min, Max>> {};
}
template<CNumber auto Min, CNumber auto Max> requires(Min <= Max)
using TBasicNumberWithRange = typename TBasicNumberWithRange_<Min, Max>::_;

template<CNumber auto Min, CNumber auto Max> requires CBasicIntegral<TBasicNumberWithRange<Min, Max>>
using TBasicIntegerWithRange = TBasicNumberWithRange<Min, Max>;

enum class OverflowMode: uint8 {Ignore, Wrap, Checked, Saturate};
enum class RoundMode: uint8 {Truncate, Floor, Ceil, Nearest, Illegal};

} INTRA_END
