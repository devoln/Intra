#pragma once

#include "Core/Assert.h"

INTRA_CORE_BEGIN
namespace Misc {
INTRA_CONSTEXPR2 inline uint64 MulHighPart(uint64 x, uint64 y)
{
#ifdef INTRA_INT128_SUPPORT
	return uint64((uint128(x) * y) >> 64);
//#elif defined(_MSC_VER) && defined(INTRA_PLATFORM_IS_64)
	//TODO: is there a way to optimize this on MSVC preserving constexpr? _umul128 and other intrinsics cannot be used in constexpr
#else
	const uint64 xh_yh = uint64(uint32(x >> 32)) * uint32(y >> 32);
	const uint64 xl_yh = uint64(uint32(x & 0xFFFFFFFF)) * uint32(y >> 32);
	const uint64 xh_yl = uint64(uint32(x >> 32)) * uint32(y & 0xFFFFFFFF);
	const uint64 xl_yl = uint64(uint32(x & 0xFFFFFFFF)) * uint32(y & 0xFFFFFFFF);
	const uint64 middle = xh_yl + (xl_yl >> 32) + (xl_yh & 0xFFFFFFFF);
	return xh_yh + (middle >> 32) + (xl_yh >> 32);
#endif
}

INTRA_OPTIMIZE_FUNCTION()
INTRA_CONSTEXPR2 forceinline uint64 Div100M(uint64 x)
{
#ifdef INTRA_PLATFORM_IS_64
	// Most compilers replace this with efficient 64-bit multiply trick on 64-bit platforms
	// We enforce this optimization on GCC and MSVC using INTRA_OPTIMIZE_FUNCTION macro
	return x / 100000000;
#else
	// divide by 100000000 using 64 bit multiply + shift trick. Most compilers cannot do this automatically on 32-bit platforms.
	// We cannot do this efficiently using 64-bit instructions on 64-bit platforms in C++ without __int128 support on MSVC
	return MulHighPart(x, 0xABCC77118461CEFD) >> 26;
#endif
}
INTRA_OPTIMIZE_FUNCTION_END

INTRA_CONSTEXPR2 inline uint32 Pow5Factor(uint32 x)
{
	INTRA_DEBUG_ASSERT(x != 0);
	for(uint32 i = 0; ; i++)
	{
		if(x % 5 != 0) return i;
		x = x / 5;
	}
}

INTRA_CONSTEXPR2 inline bool IsMultipleOfPowerOf5(uint32 x, uint32 powerOf5)
{
	return Pow5Factor(x) >= powerOf5;
}

INTRA_CONSTEXPR2 inline bool IsMultipleOfPowerOf2(uint32 x, uint32 powerOf2)
{
	// return __builtin_ctz(x) >= powerOf2; //TODO: check if it is faster
	return (x & ((1u << powerOf2) - 1)) == 0;
}

INTRA_CONSTEXPR2 inline uint32 MulShift(uint32 m, uint64 factor, int32 shift)
{
  INTRA_DEBUG_ASSERT(shift > 32);
  const uint32 factorLo = uint32(factor & 0xFFFFFFFF);
  const uint32 factorHi = uint32(factor >> 32);
  const uint64 bits0 = uint64(m) * factorLo;
  const uint64 bits1 = uint64(m) * factorHi;
#ifdef INTRA_PLATFORM_IS_64
	const uint64 sum = (bits0 >> 32) + bits1;
	const uint64 shiftedSum = sum >> (shift - 32);
	INTRA_DEBUG_ASSERT(shiftedSum <= LMaxOf(uint32()));
	return uint32(shiftedSum);
#else
	// On 32-bit platforms we can avoid a 64-bit shift-right since we only
	// need the upper 32 bits of the result and the shift value is > 32.
	const uint32 bits0Hi = uint32(bits0 >> 32);
	uint32 bits1Lo = uint32(bits1);
	uint32 bits1Hi = uint32(bits1 >> 32);
	bits1Lo += bits0Hi;
	bits1Hi += (bits1Lo < bits0Hi);
	const int32 s = shift - 32;
	return (bits1Hi << (32 - s)) | (bits1Lo >> s);
#endif
}

// Returns e == 0 ? 1 : ceil(log_2(5^e)).
inline int32 Pow5bits(int32 e)
{
	// This approximation works up to the point that the multiplication overflows at e = 3529.
	// If the multiplication were done in 64 bits, it would fail at 5^4004 which is just greater
	// than 2^9297.
	INTRA_DEBUG_ASSERT(0 <= e && e <= 3528);
	return int32(((uint32(e) * 1217359) >> 19) + 1);
}

// Returns floor(log_10(2^e)).
INTRA_CONSTEXPR2 uint32 Log10Pow2(int32 e)
{
	// The first value this approximation fails for is 2^1651 which is just greater than 10^297.
	INTRA_DEBUG_ASSERT(0 <= e && e <= 1650);
	return ((uint32(e)) * 78913) >> 18;
}

// Returns floor(log_10(5^e)).
INTRA_CONSTEXPR2 uint32 Log10Pow5(int32 e)
{
	// The first value this approximation fails for is 5^2621 which is just greater than 10^1832.
	INTRA_DEBUG_ASSERT(0 <= e && e <= 2620);
	return ((uint32(e)) * 732923) >> 20;
}

}
INTRA_CORE_END
