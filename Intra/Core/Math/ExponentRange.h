#pragma once

#include "Core/Type.h"
#include "Math/Math.h"

INTRA_BEGIN
template<typename T> struct ExponentRange
{
	enum: bool {RangeIsInfinite = true};

	constexpr ExponentRange(T scale=0, double step=0, T k=0):
		mEkSr(T(Exp(-k*step))), mExponent(scale/mEkSr) {}

	INTRA_NODISCARD constexpr forceinline bool Empty() const {return false;}
	constexpr forceinline void PopFirst() {mExponent *= mEkSr;}
	INTRA_NODISCARD constexpr forceinline T First() const {return mExponent;}

private:
	T mEkSr, mExponent;
};
INTRA_END
