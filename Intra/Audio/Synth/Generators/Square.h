#pragma once

#include "Cpp/Warnings.h"
#include "Cpp/Features.h"
#include "Cpp/Fundamental.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio { namespace Synth { namespace Generators {

struct Square
{
	Square(float frequency, uint sampleRate):
		mPhase(0), mDeltaPhase(frequency*2 / sampleRate) {}

	forceinline void PopFirst() { mPhase += mDeltaPhase; }
	forceinline float First() const { return float(int(mPhase) & 1)*2.0f - 1.0f; }
	forceinline bool Empty() const { return false; }

private:
	float mPhase, mDeltaPhase;
};

}}}}

INTRA_WARNING_POP
