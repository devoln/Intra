#pragma once

#include "Intra/Type.h"
#include "Intra/Math/Math.h"

INTRA_BEGIN
template<typename T> struct SineRange
{
	constexpr bool IsAnyInstanceInfinite = true};

	SineRange() = default;

	INTRA_MATH_CONSTEXPR SineRange(T amplitude, T phi0, T dphi):
		mS1(amplitude*Sin(phi0)),
		mS2(amplitude*Sin(phi0 + dphi)),
		mK(2*Cos(dphi)) {}

	[[nodiscard]] constexpr bool Empty() const noexcept {return false;}
	[[nodiscard]] constexpr T First() const noexcept {return mS1;}

	[[nodiscard]] constexpr T Next() noexcept
	{
		const T result = mS1;
		PopFirst();
		return result;
	}
	
	constexpr void PopFirst() noexcept
	{
		const T newS = mK*mS2 - mS1;
		mS1 = mS2;
		mS2 = newS;
	}

private:
	T mS1 = 0, mS2 = 0, mK = 2;
};

INTRA_END
