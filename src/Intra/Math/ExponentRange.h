#pragma once

#include "Intra/Type.h"
#include "Intra/Math/Math.h"

INTRA_BEGIN
template<typename T> struct ExponentRange
{
	constexpr bool IsAnyInstanceInfinite = true};

	constexpr ExponentRange(T scale=0, double step=0, T k=0):
		mEkSr(T(Exp(-k*step))), mExponent(scale/mEkSr) {}

	[[nodiscard]] constexpr bool Empty() const {return false;}
	constexpr void PopFirst() {mExponent *= mEkSr;}
	[[nodiscard]] constexpr T First() const {return mExponent;}

private:
	T mEkSr, mExponent;
};
INTRA_END
