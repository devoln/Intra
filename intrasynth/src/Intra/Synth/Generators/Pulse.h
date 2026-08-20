#pragma once

#include <Cpp/Warnings.h>
#include <Cpp/Features.h>
#include <Cpp/Fundamental.h>

#include <Math/Math.h>

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Generators {

struct Pulse
{
	Pulse(float updownRatio, float frequency, unsigned sampleRate):
		mUpdownPercent(updownRatio / (updownRatio + 1)), mPhase(0), mDeltaPhase(frequency*2 / float(sampleRate)) {}

	INTRA_FORCEINLINE void PopFirst() {mPhase += mDeltaPhase;}
	INTRA_FORCEINLINE float First() const {return Math::Fract(mPhase) > mUpdownPercent? 1.0f: - 1.0f;}
	INTRA_FORCEINLINE bool Empty() const {return false;}

private:
	float mUpdownPercent, mPhase, mDeltaPhase;
};

}

INTRA_WARNING_POP
