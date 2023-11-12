#pragma once

#include <Intra/Core.h>
#include <Intra/Meta.h>

namespace Intra { INTRA_BEGIN

template<typename T> constexpr auto MaxFiniteValueOf = Undefined;
template<CBasicArithmetic T> constexpr T MaxFiniteValueOf<T> = [] {
	if constexpr(CConst<T> || CVolatile<T>) return MaxFiniteValueOf<TUnqual<T>>;
	else if constexpr(CBasicIntegral<T>) return TUnqual<T>(~0ULL >> (64 - SizeofInBits<T> + CSigned<T>));
	else if constexpr(CSame<T, float>) return
#ifdef __FLT_MAX__
		__FLT_MAX__;
#else
		3.402823466e+38f;
#endif
	else if constexpr(CSame<T, double>) return
#ifdef __DBL_MAX__
		__DBL_MAX__;
#else
		1.7976931348623157e+308;
#endif
	else if constexpr(CSame<T, long double>) return
#ifdef __LDBL_MAX__
		__LDBL_MAX__;
#else
		1.7976931348623157e+308L;
#endif
}();

template<typename T> constexpr auto MinFiniteValueOf = Undefined;
template<CBasicArithmetic T> constexpr T MinFiniteValueOf<T> = [] {
	if constexpr(CConst<T> || CVolatile<T>) return MinFiniteValueOf<TUnqual<T>>;
	else if constexpr(CBasicUnsignedIntegral<T>) return T(0);
	else return T(-MaxFiniteValueOf<T> - CBasicSignedIntegral<T>);
}();

template<typename T> constexpr auto MaxValueOf = Undefined;
template<typename T> constexpr auto MinValueOf = Undefined;
template<CNumber T> constexpr T MaxValueOf<T> = CFloatingPoint<T>? T(Infinity): MaxFiniteValueOf<T>;
template<CNumber T> constexpr T MinValueOf<T> = CFloatingPoint<T>? T(-Infinity): MinFiniteValueOf<T>;

template<typename T> constexpr auto SignBitMaskOf = Undefined;
template<CNumber T> constexpr auto SignBitMaskOf<T> = [] {
	if constexpr(MinValueOf<T> >= 0) return 0;
	else return TUnsignedIntOfSizeAtLeast<sizeof(T)>(1) << (SizeofInBits<T> - 1);
}();

template<typename T> constexpr auto MinNormPositiveValueOf = Undefined;
template<CBasicArithmetic T> constexpr T MinNormPositiveValueOf<T> = [] {
	if constexpr(CBasicIntegral<T>) return 1;
	else if constexpr(CSame<T, float>) return
#ifdef __FLT_MIN__
		__FLT_MIN__;
#else
		1.175494351e-38f;
#endif
	else if constexpr(CSame<T, double>) return
#ifdef __DBL_MIN__
		__DBL_MIN__;
#else
		2.2250738585072014e-308;
#endif
	else if constexpr(CSame<T, long double>) return
#ifdef __LDBL_MIN__
		__LDBL_MIN__;
#else
		2.2250738585072014e-308l;
#endif
}();

/// Get the number of mantissa bits in floating point representation. Equals to 0 for other types.
template<typename T> constexpr auto NumMantissaExplicitBitsOf = Undefined;
template<CNumber T> constexpr auto NumMantissaExplicitBitsOf<T> = [] {
	if constexpr(CBasicFloatingPoint<T>) return sizeof(T) == sizeof(float)? 23: 52;
	else return 0;
}();

/// Get mantissa length in bits including implicit 1.
/// Value is valid for basic arithmetic types and Fixed.
template<typename T> constexpr auto MantissaLenOf = Undefined;
template<CNumber T> constexpr auto MantissaLenOf<T> = [] {
	if constexpr(CBasicFloatingPoint<T>) return NumMantissaExplicitBitsOf<T> + 1;
	else if constexpr(CBasicIntegral<T>) return SizeofInBits<T> - CBasicSigned<T>;
}();

template<typename T> constexpr auto ExponentLenOf = Undefined;
template<CNumber T> constexpr auto ExponentLenOf<T> = [] {
	if constexpr(CBasicFloatingPoint<T>) return sizeof(T) == sizeof(float)? 8: 11;
	else if constexpr(CBasicIntegral<T> || CFixedPoint<T>) return 0;
	else if constexpr(CValueWrapper<T>) return ExponentLenOf<TWrappedType<T>>;
}();

template<typename T> constexpr auto ExponentBiasOf = Undefined;
template<CNumber T> constexpr auto ExponentBiasOf<T> = [] {
	if constexpr(CBasicFloatingPoint<T>) return sizeof(T) == sizeof(float)? 127: 1023;
	else if constexpr(CBasicIntegral<T>) return 0;
}();

/// The difference between 1.0 and the next representable value of the given type
template<typename T, auto RangeMin = 0, auto RangeMax = 1> constexpr auto MaxStepInRange = Undefined;
template<CNumber T, CNumber auto RangeMin, CNumber auto RangeMax> requires(RangeMin < RangeMax)
constexpr T MaxStepInRange<T, RangeMin, RangeMax> = [] {
	if constexpr(CBasicIntegral<T>) return 1;
	else if constexpr(CFixedPoint<T>) return T(Construct, 1);
	else if constexpr(CBasicFloatingPoint<T>)
	{
		constexpr T eps1 = [] {
			if constexpr(CSame<T, float>) return
#ifdef __FLT_EPSILON__
				__FLT_EPSILON__;
#else
				1.192092896e-7f;
#endif
			else if constexpr(CSame<T, double>) return
#ifdef __DBL_EPSILON__
				__DBL_EPSILON__;
#else
				2.2204460492503131e-16;
#endif
			else if constexpr(CSame<T, long double>) return
#ifdef __DBL_EPSILON__
				__LDBL_EPSILON__;
#else
				2.2204460492503131e-16L;
#endif
		}();
		constexpr auto absMin = RangeMin < 0? -RangeMin: RangeMin;
		constexpr auto absMax = RangeMax < 0? -RangeMax: RangeMax;
		constexpr auto abs = Max(absMin, absMax);
		return T(eps1 / 2 * CeilToPow2(abs));
	}
}();

template<typename T> constexpr auto IntegerRangeMax = Undefined;
template<CNumber T> constexpr auto IntegerRangeMax<T> = [] {
	if constexpr(CBasicIntegral<T>) return MaxValueOf<T>;
	else if constexpr(CBasicFloatingPoint<T>) return 1LL << MantissaLenOf<T>;
	else return 0;
}();

template<typename T> constexpr auto IntegerRangeMin = Undefined;
template<CNumber T> constexpr auto IntegerRangeMin<T> = [] {
	if constexpr(CBasicIntegral<T>) return MinValueOf<T>;
	else if constexpr(CBasicFloatingPoint<T>) return -IntegerRangeMax<T>;
	else return 0;
}();

#if INTRA_CONSTEXPR_TEST
#ifdef __DBL_MAX__
static_assert(MaxFiniteValueOf<double> == __DBL_MAX__);
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
	if constexpr(!CConvertibleTo<From, To>) return false;
	else if constexpr(CNumber<From> && CNumber<To>)
		return SafeCompare(MaxValueOf<From>, MaxValueOf<To>) <= 0 &&
			SafeCompare(MinValueOf<From>, MinValueOf<To>) >= 0;
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
	if constexpr(CBasicUnsignedIntegral<R> || CBasicUnsignedIntegral<T>) return R(0);
	else if constexpr(sizeof(R) >= sizeof(T)) return R(MinValueOf<T>);
	else return MinValueOf<R>;
}();
template<CBasicIntegral R, CBasicIntegral T> constexpr R CommonMaxValue = [] {
	if constexpr(sizeof(R) < sizeof(T)) return MaxValueOf<R>;
	else if constexpr(sizeof(R) > sizeof(T) || CSigned<R> == CSigned<T>) return R(MaxValueOf<T>);
	else if constexpr(CSigned<R>) return MaxValueOf<R>;
	else return R(MaxValueOf<T>);
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
using TBasicNumberWithRange = typename z_D::TBasicNumberWithRange_<Min, Max>::_;

template<CNumber auto Min, CNumber auto Max> requires CBasicIntegral<TBasicNumberWithRange<Min, Max>>
using TBasicIntegerWithRange = TBasicNumberWithRange<Min, Max>;

enum class OverflowMode: uint8 {Ignore, Wrap, Checked, Saturate};
enum class RoundMode: uint8 {Truncate, Floor, Ceil, Nearest, Illegal};

} INTRA_END
