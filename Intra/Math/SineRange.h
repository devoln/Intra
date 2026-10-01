#pragma once

#include "Cpp/Features.h"
#include "Cpp/Warnings.h"

#include "Meta/Type.h"

#include "Math/Math.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Range {

/// Two-term Chebyshev recursion, s(n+1) = 2*cos(dphi)*s(n) - s(n-1), emitting
/// sine samples one by one. Meant for audio-rate sine partials.
///
/// WORKING RANGE (float32, measured with this build's cosf at 44.1/48 kHz):
/// at audio rates the produced rate matches the requested one almost exactly
/// (0.01 % at 100 Hz, 0.0001 % at 1 kHz). Below ~20 Hz the rounding of
/// 2*cos(dphi) becomes visible (0.4 % at 20 Hz, 1.3 % at 10 Hz, ~12 % at
/// 3 Hz), and once dphi²/2 falls below half an ulp of 1.0 (dphi² < 2⁻²⁴)
/// the coefficient rounds to exactly 2: the output stops oscillating and
/// becomes a linear ramp that grows without bound (measured threshold 1.87 Hz
/// at 48 kHz, 1.71 Hz at 44.1 kHz). A modulation-rate consumer must therefore
/// not step this recursion once per sample: step it once per 16-64 samples —
/// the threshold above scales down by the same factor — and interpolate
/// between the steps (see SteppedSineRange in
/// intrasynth/src/Intra/Synth/Chorus.h).

template<typename T> struct SineRange
{
	enum: bool {RangeIsInfinite = true};

	SineRange(null_t=null):
		mS1(0), mS2(0), mK(2) {}

	SineRange(T amplitude, T phi0, T dphi):
		mS1(amplitude*Math::Sin(phi0)),
		mS2(amplitude*Math::Sin(phi0 + dphi)),
		mK(2*Math::Cos(dphi)) {}

	forceinline bool Empty() const noexcept {return false;}
	forceinline T First() const noexcept {return mS1;}

	forceinline T Next() noexcept
	{
		const T result = mS1;
		PopFirst();
		return result;
	}
	
	forceinline void PopFirst() noexcept
	{
		const T newS = mK*mS2 - mS1;
		mS1 = mS2;
		mS2 = newS;
	}

	forceinline bool operator==(null_t) const noexcept {return mS1 == 0 && mS2 == 0 && mK == 2;}
	forceinline bool operator!=(null_t) const noexcept {return !operator==(null);}

private:
	T mS1, mS2, mK;
};

}

namespace Math {
using Range::SineRange;
}

}

INTRA_WARNING_POP
