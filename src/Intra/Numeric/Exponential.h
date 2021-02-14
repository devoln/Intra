#pragma once

#include <Intra/Core.h>

namespace Intra { INTRA_BEGIN

template<int ApproxOrder> constexpr auto Log2Approx = []<typename T>(const T& x) requires(CNumber<TScalarOf<T>>) {
	static_assert(2 <= ApproxOrder && ApproxOrder <= 5);

	using T = TRemoveConstRef<decltype(x)>;
	using S = TScalarOf<T>;
	using SInt = TToIntegral<S>;
	using TInt = TToIntegral<T>;
	constexpr auto exponentMask = (1 << ExponentLenOf<S>) - 1;
	constexpr auto mantissaMask = (SInt(1) << NumMantissaExplicitBitsOf<S>) - 1;

	T m = BitCastTo<T>((BitCastTo<TInt>(x) & mantissaMask) | 1);

	// Minimax polynomial fit of log2(x)/(x - 1), for x in range [1, 2]
	T p;
	if constexpr(ApproxOrder == 2)
	{
		p = S(-1.04913055217340124191) + S(0.204446009836232697516f)*m;
		p = S(2.28330284476918490682) + p*m;
	}
	else if constexpr(ApproxOrder == 3)
	{
		p = S(0.688243882994381274313) + S(-0.107254423828329604454)*m;
		p = S(-1.75647175389045657003) + p*m;
		p = S(2.61761038894603480148) + p*m;
	}
	else if constexpr(ApproxOrder == 4)
	{
		p = S(-0.465725644288844778798) + S(0.0596515482674574969533)*m;
		p = S(1.48116647521213171641) + p*m;
		p = S(-2.52074962577807006663) + p*m;
		p = S(2.8882704548164776201) + p*m;
	}
	else if constexpr(ApproxOrder == 5)
	{
		p = S(3.1821337e-1) + S(-3.4436006e-2)*m;
		p = S(-1.2315303) + p*m;
		p = S(2.5988452) + p*m;
		p = S(-3.324199) + p*m;
		p = S(3.1157899) + p*m;
	}

	T e = NumericCastTo<T>(((BitCastTo<TInt>(x) >> NumMantissaExplicitBitsOf<S>) & exponentMask) - ExponentBiasOf<S>);
	p *= m - 1; // This effectively increases the polynomial degree by one, but ensures that log2(1) == 0
	return p + e;
};

template<int ApproxOrder> constexpr auto LogApprox = []<typename T>(const T& x) requires(CReal<TScalarOf<T>>)
{
	return Log2Approx<ApproxOrder>(x) * TScalarOf<T>(Constants.LN2);
};

template<typename T> requires CSame<TScalarOf<T>, float>
INTRA_FORCEINLINE T INTRA_VECTORCALL Pow2(T x) noexcept
{
	const T fractionalPart = Fract(x);

	T factor = float(-8.94283890931273951763e-03) + fractionalPart*float(-1.89646052380707734290e-03);
	factor = float(-5.58662282412822480682e-02) + factor*fractionalPart;
	factor = float(-2.40139721982230797126e-01) + factor*fractionalPart;
	factor = float(3.06845249656632845792e-01) + factor*fractionalPart;
	factor = float(1.06823753710239477000e-07) + factor*fractionalPart;
	x -= factor;

	x *= float(1 << 23);
	x += float((1 << 23) * 127);

	return T(SimdCastTo<TToIntegral<T>>(x)); //TODO: not truncate, but round to int
}

template<typename T> requires CSame<TScalarOf<T>, float>
INTRA_FORCEINLINE T INTRA_VECTORCALL SimdExp(T x) noexcept //TODO: integrate with general Exp
{
	return Pow2(x * float(1/Constants.LN2));
}

constexpr auto Pow = [](const auto& x, const auto& power) noexcept {
	using T = TRemoveConstRef<decltype(x)>;
#if defined(__GNUC__) || defined(__clang__)
	if constexpr(CBasicIntegral<decltype(power)>)
	{
		if constexpr(CSame<T, float>) return __builtin_powif(x, int(power));
		else if constexpr(CSame<T, double>) return __builtin_powi(x, int(power));
		else if constexpr(CSame<T, long double>) return __builtin_powil(x, int(power));
	}
	else if constexpr(CSame<T, float>) return __builtin_powf(x, T(power));
	else if constexpr(CSame<T, double>) return __builtin_pow(x, T(power));
	else if constexpr(CSame<T, long double>) return __builtin_powl(x, T(power));
#else
	if constexpr(CBasicIntegral<decltype(power)>)
	{
		auto v = x;
		unsigned n = unsigned(Abs(y));
		for(T z = 1; ; v *= v)
		{
			if((n & 1) != 0) z *= v;
			if((n >>= 1) == 0) return (power < 0? T(1)/z: z);
		}
	}
	else if constexpr(CSame<T, float>) return z_D::powf(x, T(power));
	else if constexpr(CBasicFloatingPoint<T>) return T(z_D::pow(double(x), double(power)));
#endif
	else return Pow(TFloatOfSizeAtLeast<Min(sizeof(T), sizeof(double))>(x), T(power));
};

constexpr auto CeilToPow2 = []<CNumber T>(T x)
{
	if constexpr(CSigned<T>) if(x <= 0) return 0;
	if constexpr(CIntegral<T>)
	{
		auto v = TToUnsigned<T>(x);
		v--;
		v |= v >> 1;
		v |= v >> 2;
		v |= v >> 4;
		if constexpr(sizeof(v) >= sizeof(uint16))
		{
			v |= v >> 8;
			if constexpr(sizeof(v) >= sizeof(uint32))
			{
				v |= v >> 16;
				if constexpr(sizeof(v) >= sizeof(uint64)) v |= v >> 32;
			}
		}
		return T(++v);
	}
	else if constexpr(CFloatingPoint<T>)
	{
		//TODO:
		return ComposeFloat(ExtractMantissaImplicit1(x));
	}
};

template<unsigned PowBase> constexpr auto IsPowerOf = []<CBasicIntegral T>(T x)
{
	if constexpr(PowBase == 2) return x > 0 && ((x & (x - 1)) == 0);
	else
	{
		if(x <= 0) return false;
		auto v = TToUnsigned<T>(x);
		while(v > 1) v /= PowBase;
		return v == 1;
	}
};

constexpr auto FloorLog2 = []<CBasicIntegral T>(T x)
{
	INTRA_PRECONDITION(x > 0);
	return UnsignedBitWidth(TToUnsigned<T>(x)) - 1;
};

template<auto Base, size_t ArrSize> constexpr Array<decltype(Base), ArrSize> PowersOf = [] {
	Array<decltype(Base), ArrSize> res;
	T pow = 1;
	for(auto& x: res) x = pow, pow *= Base;
	return res;
}();

namespace z_D {
constexpr uint8 Log10GuessTable[64] = {
	19, 18,18,18,18, 17,17,17, 16,16,16, 15,15,15,15, 14,14,14, 13,13,13, 12,12,12,12, 11,11,11, 10,10,10, 9,
	9,9,9, 8,8,8, 7,7,7, 6,6,6,6, 5,5,5, 4,4,4, 3,3,3,3, 2,2,2, 1,1,1, 0,0,0
};
}

constexpr auto CeilLog10 = []<CBasicIntegral T>(T x)
{
	INTRA_PRECONDITION(x > 0);
	constexpr uint8* guess = z_D::Log10GuessTable + (64 - SizeofInBits<T>);
	int digits = guess[CountLeadingZeros(x)];
	return digits + (x >= PowersOf<T(10), z_D::Log10GuessTable[0] + 1>[digits]);
};

constexpr auto FloorLog10 = []<CBasicIntegral T>(T x)
{
	INTRA_PRECONDITION(x > 0);
	return CeilLog10(x + 1) - 1;
};

INTRA_OPTIMIZE_FUNCTION()
template<CBasicUnsignedIntegral UInt> INTRA_FORCEINLINE constexpr int PowFactor(UInt x, int base)
{
	INTRA_PRECONDITION(base >= 2);
	if(x == 0) return 0;
	if(base == 2) return CountTrailingZeros(x);
	for(int i = 0; ; i++)
	{
		auto div = x / base;
		if(unsigned(x) - base*unsigned(div) != 0) return i;
		x = div;
	}
}
INTRA_OPTIMIZE_FUNCTION_END

template<CBasicUnsignedIntegral UInt> INTRA_FORCEINLINE constexpr bool IsMultipleOfPowerOf(UInt x, int base, int exponent)
{
	if(base == 2) return x != 0 && (x & NumBitsToMask<UInt>(exponent)) == 0;
	return PowFactor(x, base) >= exponent;
}

/// @return e == 0? 1: ceil(log2(5^exponent)).
constexpr auto BitWidthOfPow5Approx32 = [](uint32 exponentOf5) {
	return 1 + int((exponentOf5 * 1217359) >> 19);
};
constexpr auto BitWidthOfPow5Approx64 = [](uint32 exponentOf5) {
	return 1 + int((exponentOf5 * 1217359ULL) >> 19);
};
INTRA_OPTIMIZE_FUNCTION(template<unsigned PowBase> constexpr auto BitWidthOfPow = [](uint32 exponentOfPowBase)) INTRA_FORCEINLINE_LAMBDA
{
	if constexpr(PowBase == 2) return 1 + exponentOfPowBase;
	else if constexpr(PowBase == 5)
	{
		if constexpr(sizeof(size_t) == sizeof(uint64))
			if(exponentOfPowBase <= 3528) return BitWidthOfPow5Approx32(exponentOfPowBase);
		if(exponentOfPowBase <= 4003) return BitWidthOfPow5Approx64(exponentOfPowBase);
	}
	INTRA_DEBUG_ASSERT(!"Generic case is not implemented yet");
};

#if INTRA_CONSTEXPR_TEST
static_assert(BitWidthOfPow<5>(0) == UnsignedBitWidth(0u));
static_assert(BitWidthOfPow<5>(1) == UnsignedBitWidth(5u));
static_assert(BitWidthOfPow<5>(2) == UnsignedBitWidth(25u));
#endif

constexpr auto FloorLog10OfPow2Approx = [](uint32 exponentOf2) {return (uint32(exponentOf2) * 78913) >> 18;};
constexpr auto FloorLog10OfPow5Approx = [](uint32 exponentOf5) {return (uint32(exponentOf5) * 732923) >> 20;};
INTRA_OPTIMIZE_FUNCTION(template<unsigned LogBase, unsigned PowBase> constexpr auto FloorLogOfPow = [](uint32 exponentOfPowBase)) INTRA_FORCEINLINE_LAMBDA
{
	if constexpr(LogBase == 10)
	{
		//The approximations are exact for small values
		if constexpr(PowBase == 2) if(exponentOfPowBase <= 1650) return FloorLog10OfPow2Approx(exponentOfPowBase);
		if constexpr(PowBase == 5) if(exponentOfPowBase <= 2620) return FloorLog10OfPow5Approx(exponentOfPowBase);
	}
	INTRA_DEBUG_ASSERT(!"Generic case is not implemented yet");
};
INTRA_OPTIMIZE_FUNCTION_END

#if INTRA_CONSTEXPR_TEST
static_assert(BitWidthOfPow<5>(0) == UnsignedBitWidth(0u));
static_assert(BitWidthOfPow<5>(1) == UnsignedBitWidth(5u));
static_assert(BitWidthOfPow<5>(2) == UnsignedBitWidth(25u));

static_assert(FloorLogOfPow<10, 2>(3) == 0);
static_assert(FloorLogOfPow<10, 2>(4) == 1);
static_assert(FloorLogOfPow<10, 2>(50) == 15);

static_assert(FloorLogOfPow<10, 5>(10) == 6);
static_assert(FloorLogOfPow<10, 5>(11) == 7);
static_assert(FloorLogOfPow<10, 5>(30) == 20);
#endif

template<unsigned Radix> constexpr auto RadixShiftLeft = [](auto x, int shift) INTRA_FORCEINLINE_LAMBDA
{
	static_assert(Radix >= 2);
	if constexpr(Radix == 2) return x << shift;
	while(shift--) x *= Radix;
	return x;
};

template<unsigned Radix> constexpr auto RadixShiftRight = [](auto x, int shift) INTRA_FORCEINLINE_LAMBDA
{
	static_assert(Radix >= 2);
	if constexpr(Radix == 2) return x >> shift;
	while(shift--) x = FastDiv<Radix>(x);
	return x;
};

constexpr auto RadixWidth = []<CBasicIntegral T>(T x, unsigned radix = 10) INTRA_FORCEINLINE_LAMBDA
{
	if(x == 0) return 0;
	if(radix == 2) return BitWidth(x);
	if(radix == 16) return (BitWidth(x) + 3) >> 2;
	if(radix == 256) return (BitWidth(x) + 7) >> 3;
	if(radix == 10) return 1 + FloorLog10(x);
	INTRA_DEBUG_ASSERT(!"Common case is not implemented yet");
};

} INTRA_END
