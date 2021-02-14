#pragma once

#include <Intra/Core.h>

namespace Intra { INTRA_BEGIN

#if !defined(__GNUC__) && defined(_MSC_VER)
namespace z_D { extern "C" {
byte _BitScanReverse(unsigned long* Index, unsigned long Mask);
byte _BitScanReverse64(unsigned long* Index, uint64 Mask);
uint16 __popcnt16(uint16 value);
uint32 __popcnt(uint32 value);
uint64 __popcnt64(uint64 value);
}}
#endif

constexpr auto UnsignedBitWidth = []<CBasicUnsignedIntegral T>(T x)
{
	if(x == 0) return 0;
#ifdef __GNUC__
	if constexpr(sizeof(T) <= sizeof(unsigned)) return SizeofInBits<T> - __builtin_clz(x);
	else return SizeofInBits<T> - __builtin_clzll(x);
#else
	if(IsConstantEvaluated())
	{
		int i = 0;
		for(int k = 32; k != 0; k /= 2)
			if(x >= (T(1) << k)) i += k, x >>= k;
		return i;
	}
	int resOffset = 0;
	uint32 v32 = uint32(x);
	if constexpr(sizeof(T) == sizeof(uint64))
	{
	#if defined(__amd64__) || defined(__aarch64__)
		unsigned long index;
		_BitScanReverse64(&index, x);
		return 1 +  index;
	#else
		if(v >> 32)
		{
			resOffset = 32;
			v32 = uint32(x >> 32);
		}
	#endif
	}
	unsigned long index;
	_BitScanReverse(&index, v32);
	return 1 + resOffset + index;
#endif
};

constexpr auto SignedBitWidth = []<CBasicSignedIntegral T>(T x)
{
	return 1 + UnsignedBitWidth(TToUnsigned<T>(x < 0? -x: x));
};

constexpr auto BitWidth = []<CBasicIntegral T>(T x)
{
	if constexpr(CBasicSigned<T>) return SignedBitWidth(x);
	else return UnsignedBitWidth(x);
};

constexpr auto ByteWidth = []<CBasicIntegral T>(T x) {return (BitWidth(x) + 7) >> 3;};


constexpr auto Count1Bits = []<CBasicUnsignedIntegral T>(T mask)
{
#ifdef __GNUC__
	if constexpr(sizeof(T) <= sizeof(unsigned)) return int(__builtin_popcount(mask));
	else return int(__builtin_popcountll(mask));
#else
#if defined(_MSC_VER) && defined(__SSE4_2__)
	if(!IsConstantEvaluated())
	{
		if constexpr(sizeof(T) <= sizeof(uint32)) return int(z_D::__popcount(mask));
		else return int(z_D::__popcount64(mask));
	}
#endif
	if constexpr(CSameSize<T, uint64>)
	{
		uint64 v = mask;
		v -= (v >> 1) & 0b0101010101010101010101010101010101010101010101010101010101010101;
		v = (v        & 0b0011001100110011001100110011001100110011001100110011001100110011) +
			((v >> 2) & 0b0011001100110011001100110011001100110011001100110011001100110011);
		return int(((v + (v >> 4) & 0b1111000011110000111100001111000011110000111100001111000011110000) * 0x101010101010101) >> 56);
	}
	else
	{
		uint32 v = mask;
		v -= ((v >> 1) & 0b01010101010101010101010101010101);
		v = (((v >> 2) & 0b00110011001100110011001100110011) + (n & 0b00110011001100110011001100110011));
		v = (((v >> 4) + v) & 0b00001111000011110000111100001111);
		v += (v >> 8);
		v += (v >> 16);
		return int(v & 0b111111);
	}
#endif
};

template<CBasicUnsignedIntegral T> constexpr auto CountLeadingZeros = [](T x)
{
	return SizeofInBits<T> - BitWidth(x);
};

template<CBasicUnsignedIntegral T> constexpr auto CountTrailingZeros = [](T x)
{
	if(x == 0) return SizeofInBits<T>;
	#ifdef __GNUC__
		if constexpr(sizeof(T) <= sizeof(unsigned)) return __builtin_ctz(x);
		else return __builtin_ctzll(x);
	#else
		if constexpr(sizeof(T) == sizeof(uint64))
		{
		#if defined(__amd64__) || defined(__aarch64__)
			if(!IsConstantEvaluated())
			{
				unsigned long index;
				_BitScanForward64(&index, x);
				return index;
			}
			else
		#endif
			{
				if((x >> 32) == 0) return 32 + CountTrailingZeros(uint32(x));
				else return CountTrailingZeros(uint32(x >> 32));
			}
		}
		if(IsConstantEvaluated()) return Count1Bits((x & (~x + 1)) - 1);
		unsigned long index;
		_BitScanForward(&index, x);
		return index;
	#endif
};


template<CBasicIntegral T> constexpr auto NumBitsToMask = [](int bitCount)
{
	using UInt = TToUnsigned<T>;
	return unsigned(bitCount) >= SizeofInBits<T>? MaxValueOf<UInt>: UInt((UInt(1) << bitCount) - 1);
};

template<CBasicIntegral T> [[nodiscard]] constexpr T RotateBitsLeft(TExplicitType<T> x, int n)
{
	const auto ux = TToUnsigned<T>(x);
	return T((ux << n)|(ux >> (SizeofInBits<T> - n)));
}

template<CBasicIntegral T> [[nodiscard]] constexpr T RotateBitsRight(TExplicitType<T> x, int n)
{
	const auto ux = TToUnsigned<T>(x);
	return T((ux >> n)|(ux << (SizeofInBits<T> - n)));
}

template<CBasicIntegral T> constexpr auto ReverseBits = [](T x)
{
#ifdef __clang__
	if(!IsConstantEvaluated())
	{
		if constexpr(CSameSize<T, uint8>) return T(__builtin_bitreverse8(x));
		else if constexpr(CSameSize<T, uint16>) return T(__builtin_bitreverse16(x));
		else if constexpr(CSameSize<T, uint32>) return T(__builtin_bitreverse32(x));
		else if constexpr(CSameSize<T, uint64>) return T(__builtin_bitreverse64(x));
	}
#endif
	auto ux = TToUnsigned<T>(x);
	int bits = SizeofInBits<T>;
	auto mask = MaxValueOf<TToUnsigned<T>>;
	while(bits >>= 1)
	{
		mask ^= mask << bits;
		ux = TToUnsigned<T>(((ux & ~mask) >> bits) | ((ux & mask) << bits));
	}
	return T(ux);
};

#if INTRA_CONSTEXPR_TEST
static_assert(ReverseBits<uint8>(0b01100011) == 0b11000110);
static_assert(ReverseBits<uint32>(0b11000111101110101010101010001100) == 0b00110001010101010101110111100011);
#endif

} INTRA_END
