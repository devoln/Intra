#pragma once

#include "Intra/Misc/RawMemory.h"

INTRA_BEGIN
struct HalfFloat
{
	HalfFloat() = default;

	constexpr explicit HalfFloat(uint16 s): AsUShort(s) {}
	INTRA_FORCEINLINE explicit HalfFloat(float f): AsUShort(fromFloat(f)) {}
	INTRA_FORCEINLINE explicit HalfFloat(double d): AsUShort(fromFloat(float(d))) {}

	[[nodiscard]] INTRA_FORCEINLINE operator float() const {return toFloat(AsUShort);}
	[[nodiscard]] INTRA_FORCEINLINE operator double() const {return toFloat(AsUShort);}

	INTRA_FORCEINLINE HalfFloat& operator=(float rhs) {AsUShort = fromFloat(rhs); return *this;}
	INTRA_FORCEINLINE HalfFloat& operator=(double rhs) {AsUShort = fromFloat(float(rhs)); return *this;}
	INTRA_FORCEINLINE HalfFloat& operator=(const HalfFloat& rhs) = default;

	INTRA_FORCEINLINE HalfFloat& operator+=(float rhs) {AsUShort = fromFloat(toFloat(AsUShort) + rhs); return *this;}
	INTRA_FORCEINLINE HalfFloat& operator+=(double rhs) {AsUShort = fromFloat(toFloat(AsUShort) + float(rhs)); return *this;}
	INTRA_FORCEINLINE HalfFloat& operator+=(const HalfFloat& rhs) {AsUShort = fromFloat(toFloat(AsUShort) + float(rhs)); return *this;}

	INTRA_FORCEINLINE HalfFloat& operator-=(float rhs) {AsUShort = fromFloat(toFloat(AsUShort) - rhs); return *this;}
	INTRA_FORCEINLINE HalfFloat& operator-=(double rhs) {AsUShort = fromFloat(toFloat(AsUShort) - float(rhs)); return *this;}
	INTRA_FORCEINLINE HalfFloat& operator-=(const HalfFloat& rhs) {AsUShort = fromFloat(toFloat(AsUShort) - float(rhs)); return *this;}

	INTRA_FORCEINLINE HalfFloat& operator*=(float rhs) {AsUShort = fromFloat(toFloat(AsUShort) * rhs); return *this;}
	INTRA_FORCEINLINE HalfFloat& operator*=(double rhs) {AsUShort = fromFloat(toFloat(AsUShort) * float(rhs)); return *this;}
	INTRA_FORCEINLINE HalfFloat& operator*=(const HalfFloat& rhs) {AsUShort = fromFloat(toFloat(AsUShort) * float(rhs)); return *this;}

	INTRA_FORCEINLINE HalfFloat& operator/=(float rhs) {AsUShort = fromFloat(toFloat(AsUShort) / rhs); return *this;}
	INTRA_FORCEINLINE HalfFloat& operator/=(double rhs) {AsUShort = fromFloat(toFloat(AsUShort) / float(rhs)); return *this;}
	INTRA_FORCEINLINE HalfFloat& operator/=(const HalfFloat& rhs) {AsUShort = fromFloat(toFloat(AsUShort) / float(rhs)); return *this;}

	uint16 AsUShort;

private:
	static uint16 fromFloat(float f)
	{
		const uint32 i32 = Misc::BitCast<uint32>(f);
		return uint16( (((i32 & 0x7fffffffu) >> 13u) - (0x38000000u >> 13u)) | ((i32 & 0x80000000u) >> 16u) );
	}

	static float toFloat(uint16 h)
	{
		const uint32 i32 = uint32((h & 0x8000) << 16) | uint32( ((h & 0x7fff) << 13) + 0x38000000 );
		return Misc::BitCast<float>(i32);
	}
};
INTRA_END
