#pragma once

#include "Core/Core.h"
#include "Core/Type.h"
#include "Math/Math.h"

INTRA_BEGIN
inline namespace Math {

template<typename T> struct SineRange
{
	enum: bool {RangeIsInfinite = true};

	SineRange() = default;

	INTRA_MATH_CONSTEXPR SineRange(T amplitude, T phi0, T dphi):
		mS1(amplitude*Sin(phi0)),
		mS2(amplitude*Sin(phi0 + dphi)),
		mK(2*Cos(dphi)) {}

	INTRA_NODISCARD constexpr forceinline bool Empty() const noexcept {return false;}
	INTRA_NODISCARD constexpr forceinline T First() const noexcept {return mS1;}

	INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline T Next() noexcept
	{
		const T result = mS1;
		PopFirst();
		return result;
	}
	
	INTRA_CONSTEXPR2 forceinline void PopFirst() noexcept
	{
		const T newS = mK*mS2 - mS1;
		mS1 = mS2;
		mS2 = newS;
	}

private:
	T mS1 = 0, mS2 = 0, mK = 2;
};

}

INTRA_END
