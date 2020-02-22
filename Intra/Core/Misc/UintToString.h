#pragma once

#include "Core/Numeric.h"
#include "Core/Misc/RawMemory.h"
#include "Core/Misc/MathTricks.h"

/** This header file contains highly optimized unsigned integer to string conversion utilities for common special cases.

  These functions have unsafe interface so prefer using Core/Range package which uses this header under the hood.
*/

INTRA_BEGIN
namespace Misc {
constexpr inline index_t UintToString100(uint x, char* dst)
{
	if(x <= 9)
	{
		*dst = char((x | 0x30) & 0xFF);
		return 1;
	}
	else if(x <= 99)
	{
		uint low = x;
		uint ll = ((uint32(low) * 103) >> 9) & 0x1E;
		low += ll * 3;
		ll = ((low & 0xF0) >> 4) | ((low & 0x0F) << 8);
		ll |= 0x3030;
		Misc::BinarySerializeLE(ushort(ll), dst);
		return 2;
	}
	return 0;
}

constexpr inline index_t UintToString10000(uint x, char* dst)
{
	if(x <= 99) return UintToString100(x, dst);

	const index_t digits = (x > 999) ? 4 : 3;

	uint32 low = x;
	uint32 ll = ((low * 5243) >> 19) & 0xFF; //divide by 100 (compilers don't always do this optimization themselves)
	low -= ll * 100;

	low = (low << 16) | ll;

	// Two divisions by 10 (14 bits needed)
	ll = ((low * 103) >> 9) & 0x1E001E;
	low += ll * 3;

	// move digits into correct spot
	ll = ((low & 0x00F000F0) >> 4) | ((low & 0x000F000F) << 8);

	// convert from decimal digits to ASCII number digit range
	ll |= 0x30303030;

	if(digits == 4) Misc::BinarySerializeLE(ll, dst);
	else
	{
		Misc::BinarySerializeLE(ushort((ll >> 8) & 0xFFFF), dst);
		dst[2] = char(byte(ll >> 24));
	}
	return digits;
}

constexpr inline index_t UintToString100m(uint32 x, char* dst, index_t digits)
{
	const uint32 ll0 = uint32((uint64(x) * 109951163) >> 40); //divide by 10000 (compilers don't always do this optimization themselves)
	uint64 low = (x - ll0 * 10000) | (uint64(ll0) << 32);

	// Four divisions and remainders by 100
	uint64 ll = ((low * 5243) >> 19) & 0x000000FF000000FF;
	low -= ll * 100;
	low = (low << 16) | ll;

	// Eight divisions by 10 (14 bits needed)
	ll = ((low * 103) >> 9) & 0x001E001E001E001E;
	low += ll * 3;

	// move digits into correct spot
	ll = ((low & 0x00F000F000F000F0) >> 4) | (low & 0x000F000F000F000F) << 8;
	ll = (ll >> 32) | (ll << 32);

	// convert from decimal digits to ASCII number digit range
	ll |= 0x3030303030303030;

	if(digits >= 8)
	{
		Misc::BinarySerializeLE(ll, dst);
		return digits;
	}
	ll >>= (8 - digits)*8; //skip leading zeros
	int d = int(digits);
	if(d >= 4)
	{
		Misc::BinarySerializeLE(uint32(ll & 0xFFFFFFFF), dst);
		dst += 4;
		ll >>= 32;
		d -= 4;
	}
	if(d >= 2)
	{
		Misc::BinarySerializeLE(ushort(ll & 0xFFFF), dst);
		dst += 2;
		ll >>= 16;
		d -= 2;
	}
	if(d > 0) *dst = char(byte(ll & 0xFF));
}

constexpr inline index_t UintDecimalLength9(uint32 x)
{
	INTRA_DEBUG_ASSERT(x < 1000000000);
	if(x >= 100000000) return 9;
	if(x >= 10000000) return 8;
	if(x >= 1000000) return 7;
	if(x >= 100000) return 6;
	if(x >= 10000) return 5;
	if(x >= 1000) return 4;
	if(x >= 100) return 3;
	if(x >= 10) return 2;
	return 1;
}

constexpr inline index_t UintDecimalLength(uint32 x)
{
	return x >= 1000000000? 10: UintDecimalLength9(x);
}

constexpr inline index_t UintToString(uint32 x, char* dst)
{
	if(x <= 9999) return UintToString10000(x, dst);
	index_t digits = 0;

	if(x < 100000000)
	{
		if(x > 999999) digits = x > 9999999? 8: 7;
		else digits = x > 99999? 6: 5;
	}
	else
	{
		// Divide by 100000000. Compilers can sometimes do this optimization themselves
		// but for some platforms (e.g. wasm) or some optimization flags (GCC -Os, /O1s MSVC) they don't.
		const uint high = uint((uint64(x) * 0x55E63B89) >> 57);
		x -= uint32(high) * 100000000;
		digits = UintToString100(high, dst); // two digit version since `high` <= 42
		dst += digits;
		digits += 8;
	}

	UintToString100m(x, dst, digits);
	return digits;
}

constexpr inline index_t UintToString(uint64 x, char* dst)
{
	if(x <= 0xFFFFFFFF) return UintToString(uint32(x), dst); //boil down to 32-bit version without 64-bit divisions that are slow on 32-bit platforms
	const uint64 xdiv100m = Div100M(x);
	const uint64 xmod100m = x - xdiv100m * 100000000;
	const index_t d = UintToString(xdiv100m, dst); //recursion: print highest part first
	return d + UintToString100m(xmod100m, dst + d, 8); //this function allows fixed number width = 8
}

constexpr inline void UIntToHexString(uint32 num, char* dst, bool lowerAlpha)
{
	uint64 x = num;

	// isolate each hex-digit into its own byte and prepare them for little-endian copy
	// Ex: 0x1234FACE => 0x0E0C0A0F04030201
	x = ((x & 0xFFFF) << 32) | ((x & 0xFFFF0000) >> 16);
	x = ((x & 0x0000FF000000FF00) >> 8) | (x & 0x000000FF000000FF) << 16;
	x = ((x & 0x00F000F000F000F0) >> 4) | (x & 0x000F000F000F000F) << 8;

	// Create bitmask of bytes containing alpha hex digits
	// - add 6 to each digit
	// - if the digit is a high alpha hex digit, then the addition
	//   will overflow to the high nibble of the byte
	// - shift the high nibble down to the low nibble and mask
	//   to create the relevant bitmask
	//
	// Using above example:
	// 0x0E0C0A0F04030201 + 0x0606060606060606 = 0x141210150a090807
	// >> 4 == 0x0141210150a09080 & 0x0101010101010101
	// == 0x0101010100000000
	const uint64 mask = ((x + 0x0606060606060606) >> 4) & 0x0101010101010101;

	// convert to ASCII numeral characters
	x |= 0x3030303030303030;

	// if there are high hexadecimal characters, need to adjust
	// for uppercase alpha hex digits, need to add 0x07
	//   to move 0x3A-0x3F to 0x41-0x46 (A-F)
	// for lowercase alpha hex digits, need to add 0x27
	//   to move 0x3A-0x3F to 0x61-0x66 (a-f)
	// it's actually more expensive to test if mask non-null
	//   and then run the following stmt
	x += (lowerAlpha? 0x27: 0x07) * mask;

	Misc::BinarySerializeLE(x, dst);
}

constexpr inline void UIntToHexString(uint64 num, char* dst, bool lowerAlpha)
{
	UIntToHexString(uint32(num >> 32), dst, lowerAlpha);
	UIntToHexString(uint32(num & 0xFFFFFFFF), dst+8, lowerAlpha);
}
}
INTRA_END
