#pragma once

#include <Intra/Platform/Toolchain.h>
#include <Intra/Numeric/Traits.h>

namespace Intra { INTRA_BEGIN
INTRA_FORCEINLINE constexpr uint64 Mul64To128(uint64 x, uint64 y, Out<uint64> hi)
{
#ifdef __SIZEOF_INT128__
	const auto xy = uint128(x) * y;
	hi = uint64(xy >> 64);
	return uint64(xy);
#else
#if defined(_MSC_VER) && defined(__amd64__)
	if(!IsConstantEvaluated())
	{
		uint64 res = 0;
		z_D::_umul128(x, y, &hi.Ref);
		return res;
	}
#endif
	const uint64 xl_yl = uint64(uint32(x)) * uint32(y);
	const uint64 xh_yl = (x >> 32) * uint32(y);
	const uint64 xl_yh = uint32(x) * (y >> 32);
	const uint64 xh_yh = (x >> 32) * (y >> 32);
	const uint64 cross = (xl_yl >> 32) + uint32(xh_yl) + xl_yh;
	hi = (xh_yl >> 32) + (cross >> 32) + xh_yh;
	return (cross << 32) | uint32(xl_yl);
#endif
}

constexpr uint64 ShiftRight128UpTo64(uint64 lowPart, uint64 highPart, int shift)
{
	INTRA_PRECONDITION(0 < shift && shift < 64);
#ifdef __SIZEOF_INT128__
	const auto u128 = lowPart | (uint128(highPart) << 64);
	return uint64(u128 >> shift);
#else
#if defined(_MSC_VER) && defined(__amd64__)
	if(!IsConstantEvaluated()) return z_D::__shiftright128(lowPart, highPart, shift);
#endif
	return (lowPart >> shift) | (highPart << (64 - shift));
#endif
}

constexpr uint64 ShiftLeft128UpTo64(uint64 lowPart, uint64 highPart, int shift)
{
	INTRA_PRECONDITION(0 < shift && shift < 64);
#ifdef __SIZEOF_INT128__
	const auto u128 = lowPart | (uint128(highPart) << 64);
	return uint64(u128 << shift);
#else
#if defined(_MSC_VER) && defined(__amd64__)
	if(!IsConstantEvaluated()) return z_D::__shiftleft128(lowPart, highPart, shift);
#endif
	return (lowPart << shift) | (highPart >> (64 - shift));
#endif
}

constexpr uint64 MulHighPart(uint64 x, uint64 y)
{
#if defined(_MSC_VER) && (defined(__amd64__) || defined(__aarch64__))
	return z_D::__umulh(x, y);
#else
	uint64 res = 0;
	Mul64To128(x, y, Out(res));
	return res;
#endif
}

constexpr uint32 MulShift(uint32 m, uint64 factor, int shift)
{
	INTRA_PRECONDITION(shift > 32);
	const uint32 factorLo = uint32(factor & 0xFFFFFFFF);
	const uint32 factorHi = uint32(factor >> 32);
	const uint64 bits0 = uint64(m) * factorLo;
	const uint64 bits1 = uint64(m) * factorHi;
	if constexpr(sizeof(WidestFastInt) >= 8)
	{
		const uint64 sum = (bits0 >> 32) + bits1;
		const uint64 shiftedSum = sum >> (shift - 32);
		INTRA_POSTCONDITION(shiftedSum <= MaxValueOf<uint32>);
		return uint32(shiftedSum);
	}
	else // Avoid a 64-bit right shift since we only need the upper 32 bits of the result and the shift value is > 32.
	{
		const uint32 bits0Hi = uint32(bits0 >> 32);
		uint32 bits1Lo = uint32(bits1);
		uint32 bits1Hi = uint32(bits1 >> 32);
		bits1Lo += bits0Hi;
		bits1Hi += (bits1Lo < bits0Hi);
		const int s = shift - 32;
		return (bits1Hi << (32 - s)) | (bits1Lo >> s);
	}
}

constexpr uint64 MulShift64(uint64 m, uint64 factorLo, uint64 factorHi, int shift)
{
	INTRA_PRECONDITION(shift > 64);
	uint64 bits0Hi = 0, bits1Hi = 0;
	Mul64To128(m, factorLo, Out(bits0Hi));
	uint64 bits1Lo = Mul64To128(m, factorHi, Out(bits1Hi));
	bits1Lo += bits0Hi;
	bits1Hi += (bits1Lo < bits0Hi);
	const int s = shift - 64;
	return (bits1Hi << (64 - s)) | (bits1Lo >> s);
}

} INTRA_END
