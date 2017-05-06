#pragma once

#include "Cpp/Fundamental.h"
#include "Cpp/Warnings.h"
#include "Cpp/Features.h"

namespace Intra { namespace Math {

struct HalfFloat
{
	HalfFloat() = default;

	forceinline explicit HalfFloat(ushort s): AsUShort(s) {}
	forceinline explicit HalfFloat(float f): AsUShort(fromFloat(f)) {}
	forceinline explicit HalfFloat(double d): AsUShort(fromFloat(float(d))) {}

	forceinline operator float() const {return toFloat(AsUShort);}
	forceinline operator double() const {return toFloat(AsUShort);}

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
		union {float f32; uint i32;};
		f32 = f;
		return ushort( (((i32 & 0x7fffffffu) >> 13u) - (0x38000000u >> 13u)) | ((i32 & 0x80000000u) >> 16u) );
	}

	static float toFloat(ushort h)
	{
		union {float f32; int i32;};
		i32 = ((h & 0x8000) << 16) | ( ((h & 0x7fff) << 13) + 0x38000000 );
		return f32;
	}
};

}}
