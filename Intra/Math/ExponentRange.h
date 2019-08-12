#pragma once

#include "Core/Core.h"
#include "Core/Type.h"
#include "Math/Math.h"

INTRA_BEGIN
inline namespace Math {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename T> struct ExponentRange
{
	enum: bool {RangeIsInfinite = true};

	constexpr ExponentRange(T scale=0, double step=0, T k=0):
		mEkSr(T(Math::Exp(-k*step))), mExponent(scale/mEkSr) {}

	INTRA_NODISCARD constexpr forceinline bool Empty() const {return false;}
	INTRA_CONSTEXPR2 forceinline void PopFirst() {mExponent *= mEkSr;}
	INTRA_NODISCARD constexpr forceinline T First() const {return mExponent;}

private:
	T mEkSr, mExponent;
};

}
INTRA_END
