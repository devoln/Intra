#pragma once

#include "Core/Core.h"
#include "Core/Misc/RawMemory.h"

INTRA_BEGIN
inline namespace Math {

struct HalfFloat
{
	HalfFloat() = default;

	constexpr forceinline explicit HalfFloat(ushort s): AsUShort(s) {}
	forceinline explicit HalfFloat(float f): AsUShort(fromFloat(f)) {}
	forceinline explicit HalfFloat(double d): AsUShort(fromFloat(float(d))) {}

	INTRA_NODISCARD forceinline operator float() const {return toFloat(AsUShort);}
	INTRA_NODISCARD forceinline operator double() const {return toFloat(AsUShort);}

	forceinline HalfFloat& operator=(float rhs) {AsUShort = fromFloat(rhs); return *this;}
	forceinline HalfFloat& operator=(double rhs) {AsUShort = fromFloat(float(rhs)); return *this;}
	forceinline HalfFloat& operator=(const HalfFloat& rhs) = default;

	forceinline HalfFloat& operator+=(float rhs) {AsUShort = fromFloat(toFloat(AsUShort) + rhs); return *this;}
	forceinline HalfFloat& operator+=(double rhs) {AsUShort = fromFloat(toFloat(AsUShort) + float(rhs)); return *this;}
	forceinline HalfFloat& operator+=(const HalfFloat& rhs) {AsUShort = fromFloat(toFloat(AsUShort) + float(rhs)); return *this;}

	forceinline HalfFloat& operator-=(float rhs) {AsUShort = fromFloat(toFloat(AsUShort) - rhs); return *this;}
	forceinline HalfFloat& operator-=(double rhs) {AsUShort = fromFloat(toFloat(AsUShort) - float(rhs)); return *this;}
	forceinline HalfFloat& operator-=(const HalfFloat& rhs) {AsUShort = fromFloat(toFloat(AsUShort) - float(rhs)); return *this;}

	forceinline HalfFloat& operator*=(float rhs) {AsUShort = fromFloat(toFloat(AsUShort) * rhs); return *this;}
	forceinline HalfFloat& operator*=(double rhs) {AsUShort = fromFloat(toFloat(AsUShort) * float(rhs)); return *this;}
	forceinline HalfFloat& operator*=(const HalfFloat& rhs) {AsUShort = fromFloat(toFloat(AsUShort) * float(rhs)); return *this;}

	forceinline HalfFloat& operator/=(float rhs) {AsUShort = fromFloat(toFloat(AsUShort) / rhs); return *this;}
	forceinline HalfFloat& operator/=(double rhs) {AsUShort = fromFloat(toFloat(AsUShort) / float(rhs)); return *this;}
	forceinline HalfFloat& operator/=(const HalfFloat& rhs) {AsUShort = fromFloat(toFloat(AsUShort) / float(rhs)); return *this;}

	ushort AsUShort;

private:
	static ushort fromFloat(float f)
	{
		const uint32 i32 = Misc::BitCast<uint32>(f);
		return ushort( (((i32 & 0x7fffffffu) >> 13u) - (0x38000000u >> 13u)) | ((i32 & 0x80000000u) >> 16u) );
	}

	static float toFloat(ushort h)
	{
		const uint32 i32 = uint32((h & 0x8000) << 16) | uint32( ((h & 0x7fff) << 13) + 0x38000000 );
		return Misc::BitCast<float>(i32);
	}
};

}
INTRA_END
