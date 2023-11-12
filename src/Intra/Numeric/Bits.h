#pragma once

#include <Intra/Core.h>
#include <Intra/Platform/Toolchain.h>

namespace Intra { INTRA_BEGIN

template<CBasicUnsignedIntegral T> constexpr auto UnsignedBitWidth = [](T x)
{
	if(x == 0) return 0;
#if defined(__GNUC__) || defined(__clang__)
	if constexpr(sizeof(T) <= sizeof(unsigned)) return SizeofInBits<T> - __builtin_clz(x);
	else return SizeofInBits<T> - __builtin_clzll(x);
#else
	if(IsConstantEvaluated())
	{
		int i = 0;
		for(int k = SizeofInBits<T> / 2; k != 0; k /= 2)
			if(x >= (T(1) << k)) i += k, x >>= k;
		return i;
	}
	int resOffset = 0;
	uint32 v32 = uint32(x);
	if constexpr(CSameSize<T, uint64>)
	{
	#if defined(__amd64__) || defined(__aarch64__)
		unsigned long index;
		z_D::_BitScanReverse64(&index, x);
		return int(1 + index);
	#else
		if(x >> 32)
		{
			resOffset = 32;
			v32 = uint32(x >> 32);
		}
	#endif
	}
	unsigned long index;
	z_D::_BitScanReverse(&index, v32);
	return int(1 + resOffset + index);
#endif
};

template<CBasicSignedIntegral T> constexpr auto SignedBitWidth = [](T x)
{
	return 1 + UnsignedBitWidth<TToUnsigned<T>>(TToUnsigned<T>(x < 0? -x: x));
};

template<CBasicIntegral T> constexpr auto BitWidth = [](T x)
{
	if constexpr(CBasicSigned<T>) return SignedBitWidth<T>(x);
	else return UnsignedBitWidth<T>(x);
};

template<CBasicIntegral T> constexpr auto ByteWidth = [](T x) {return (BitWidth<T>(x) + 7) >> 3;};


template<CBasicUnsignedIntegral T> constexpr auto Count1Bits = [](T mask)
{
#if defined(__GNUC__) || defined(__clang__)
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
		v = (((v >> 2) & 0b00110011001100110011001100110011) + (v & 0b00110011001100110011001100110011));
		v = (((v >> 4) + v) & 0b00001111000011110000111100001111);
		v += (v >> 8);
		v += (v >> 16);
		return int(v & 0b111111);
	}
#endif
};

template<CBasicUnsignedIntegral T> constexpr auto CountLeadingZeros = [](T x) {return SizeofInBits<T> - BitWidth<T>(x);};

template<CBasicUnsignedIntegral T> constexpr auto CountTrailingZeros = [](T x)
{
	if(x == 0) return SizeofInBits<T>;
#if defined(__GNUC__) || defined(__clang__)
	if constexpr(sizeof(T) <= sizeof(unsigned)) return __builtin_ctz(x);
	else return __builtin_ctzll(x);
#else
	if constexpr(CSameSize<T, uint64>)
	{
	#if defined(__amd64__) || defined(__aarch64__)
		if(!IsConstantEvaluated())
		{
			unsigned long index;
			z_D::_BitScanForward64(&index, x);
			return int(index);
		}
		else
	#endif
		{
			if((x >> 32) == 0) return 32 + CountTrailingZeros<uint32>(uint32(x));
			else return CountTrailingZeros<uint32>(uint32(x >> 32));
		}
	}
	if(IsConstantEvaluated()) return Count1Bits<T>((x & (~x + 1)) - 1);
	unsigned long index;
	z_D::_BitScanForward(&index, x);
	return int(index);
#endif
};


template<CBasicIntegral T> constexpr auto NumBitsToMask = [](int bitCount)
{
	using UInt = TToUnsigned<T>;
	return unsigned(bitCount) >= SizeofInBits<T>? MaxValueOf<UInt>: UInt((UInt(1) << bitCount) - 1);
};

template<CBasicIntegral T> constexpr auto RotateBitsLeft = [](T x, int n)
{
	const auto ux = TToUnsigned<T>(x);
	return T((ux << n)|(ux >> (SizeofInBits<T> - n)));
};

template<CBasicIntegral T> constexpr auto RotateBitsRight = [](T x, int n)
{
	const auto ux = TToUnsigned<T>(x);
	return T((ux >> n)|(ux << (SizeofInBits<T> - n)));
};

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
