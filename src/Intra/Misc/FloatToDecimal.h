#pragma once

#include "Intra/Numeric.h"
#include "Intra/Float.h"
#include "Intra/Misc/MathTricks.h"

INTRA_BEGIN
namespace Misc {

// This code was adapted from Ryu implementation https://github.com/ulfjack/ryu

namespace z_D {
enum {FLOAT_POW5_INV_BITCOUNT = 59, FLOAT_POW5_BITCOUNT = 61};

constexpr inline uint32 MulPow5InvDivPow2(uint32 m, uint32 q, int32 j)
{
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
	return MulShift(m, FLOAT_POW5_INV_SPLIT[q], j);
}

constexpr inline uint32 MulPow5divPow2(uint32 m, uint32 i, int32 j)
{
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
	return MulShift(m, FLOAT_POW5_SPLIT[i], j);
}
}

constexpr DecimalFloat<uint32> f2d(uint32 ieeeMantissa, uint32 ieeeExponent)
{
	// We subtract 2 so that the bounds computation has 2 additional bits.
	int32 e2 = int32(ieeeExponent) - ExponentBiasOf(float()) - NumMantissaBitsOf(float()) - 2;
	uint32 m2 = ieeeMantissa;
	if(ieeeExponent == 0) e2++;
	else m2 |= 1u << NumMantissaBitsOf(float());
	const bool even = (m2 & 1) == 0;
	const bool acceptBounds = even;

#ifdef RYU_DEBUG
	printf("-> %u * 2^%d\n", m2, e2 + 2);
#endif

	// Step 2: Determine the interval of valid decimal representations.
	const uint32 mv = 4 * m2;
	const uint32 mp = 4 * m2 + 2;
	// Implicit bool -> int conversion. True is 1, false is 0.
	const uint32 mmShift = ieeeMantissa != 0 || ieeeExponent <= 1;
	const uint32 mm = 4 * m2 - 1 - mmShift;

	// Step 3: Convert to a decimal power base using 64-bit arithmetic.
	uint32 vr = 0, vp = 0, vm = 0;
	int32 e10 = 0;
	bool vmIsTrailingZeros = false;
	bool vrIsTrailingZeros = false;
	byte lastRemovedDigit = 0;
	if(e2 >= 0)
	{
		const uint32 q = Log10Pow2(e2);
		e10 = int32(q);
		const int32 k = z_D::FLOAT_POW5_INV_BITCOUNT + Pow5bits(int32(q)) - 1;
		const int32 i = -e2 + int32(q) + k;
		vr = z_D::MulPow5InvDivPow2(mv, q, i);
		vp = z_D::MulPow5InvDivPow2(mp, q, i);
		vm = z_D::MulPow5InvDivPow2(mm, q, i);
		if(q != 0 && (vp - 1) / 10 <= vm / 10) {
			// We need to know one removed digit even if we are not going to loop below. We could use
			// q = X - 1 above, except that would require 33 bits for the result, and we've found that
			// 32-bit arithmetic is faster even on 64-bit machines.
			const int32 l = z_D::FLOAT_POW5_INV_BITCOUNT + Pow5bits(int32(q - 1)) - 1;
			lastRemovedDigit = byte(z_D::MulPow5InvDivPow2(mv, q - 1, -e2 + int32(q) - 1 + l) % 10);
		}
		if(q <= 9) {
			// The largest power of 5 that fits in 24 bits is 5^10, but q <= 9 seems to be safe as well.
			// Only one of mp, mv, and mm can be a multiple of 5, if any.
			if(mv % 5 == 0) vrIsTrailingZeros = IsMultipleOfPowerOf5(mv, q);
			else if(acceptBounds) vmIsTrailingZeros = IsMultipleOfPowerOf5(mm, q);
			else vp -= IsMultipleOfPowerOf5(mp, q);
		}
	}
	else
	{
		const uint32 q = Log10Pow5(-e2);
		e10 = int32(q) + e2;
		const int32 i = -e2 - int32(q);
		const int32 k = Pow5bits(i) - z_D::FLOAT_POW5_BITCOUNT;
		int32 j = int32(q) - k;
		vr = z_D::MulPow5divPow2(mv, uint32(i), j);
		vp = z_D::MulPow5divPow2(mp, uint32(i), j);
		vm = z_D::MulPow5divPow2(mm, uint32(i), j);
		if(q != 0 && (vp - 1) / 10 <= vm / 10)
		{
			j = int32(q) - 1 - Pow5bits(i + 1) - z_D::FLOAT_POW5_BITCOUNT;
			lastRemovedDigit = byte(z_D::MulPow5divPow2(mv, uint32(i + 1), j) % 10);
		}
		if(q <= 1)
		{
			// {vr,vp,vm} is trailing zeros if {mv,mp,mm} has at least q trailing 0 bits.
			// mv = 4 * m2, so it always has at least two trailing 0 bits.
			vrIsTrailingZeros = true;
			if(acceptBounds) vmIsTrailingZeros = mmShift == 1; // mm = mv - 1 - mmShift, so it has 1 trailing 0 bit iff mmShift == 1.
			else --vp; // mp = mv + 2, so it always has at least one trailing 0 bit.
		}
		else if(q < 31)
		{ // TODO(ulfjack): Use a tighter bound here.
			vrIsTrailingZeros = IsMultipleOfPowerOf2(mv, q - 1);
		}
	}

	// Step 4: Find the shortest decimal representation in the interval of valid representations.
	int32 removed = 0;
	uint32 output = 0;
	if(vmIsTrailingZeros || vrIsTrailingZeros)
	{
		// General case, which happens rarely (~4.0%).
		while(vp / 10 > vm / 10)
		{
			// The compiler does not realize that vm % 10 can be computed from vm / 10
			// as vm - (vm / 10) * 10.
			vmIsTrailingZeros &= vm - (vm / 10) * 10 == 0;
			vrIsTrailingZeros &= lastRemovedDigit == 0;
			lastRemovedDigit = byte(vr % 10);
			vr /= 10;
			vp /= 10;
			vm /= 10;
			++removed;
		}
		if(vmIsTrailingZeros)
		{
			while(vm % 10 == 0)
			{
				vrIsTrailingZeros &= lastRemovedDigit == 0;
				lastRemovedDigit = byte(vr % 10);
				vr /= 10;
				vp /= 10;
				vm /= 10;
				++removed;
			}
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
	else
	{
		// Specialized for the common case (~96.0%). Percentages below are relative to this.
		// Loop iterations below (approximately):
		// 0: 13.6%, 1: 70.7%, 2: 14.1%, 3: 1.39%, 4: 0.14%, 5+: 0.01%
		while(vp / 10 > vm / 10)
		{
			lastRemovedDigit = byte(vr % 10);
			vr /= 10;
			vp /= 10;
			vm /= 10;
			++removed;
		}
		// We need to take vr + 1 if vr is outside bounds or we need to round up.
		output = vr;
		if(vr == vm || lastRemovedDigit >= 5) output++;
	}
	const int exp = e10 + removed;
	return {output, exp};
}



}
INTRA_END
