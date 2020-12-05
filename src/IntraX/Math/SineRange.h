#pragma once

#include "Intra/Type.h"
#include "Intra/Math.h"
#include "Intra/Range/Decorators.h"

INTRA_BEGIN
template<typename T> [[nodiscard]] INTRA_MATH_CONSTEXPR auto SineRange(T amplitude, T phi0, T dphi)
{
	return Recurrence([K = 2*Cos(dphi)](T a, T b) {return K*b - a;},
		amplitude*Sin(phi0), amplitude*Sin(phi0 + dphi));
}
INTRA_END
