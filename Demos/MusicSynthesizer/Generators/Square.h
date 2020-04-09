#pragma once

#include <Cpp/Warnings.h>
#include <Cpp/Features.h>
#include <Cpp/Fundamental.h>

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Generators {

struct Square
{
	Square(float frequency, unsigned sampleRate):
		mPhase(0), mDeltaPhase(frequency*2 / float(sampleRate)) {}

	INTRA_FORCEINLINE void PopFirst() {mPhase += mDeltaPhase;}
	INTRA_FORCEINLINE float First() const {return float(int(mPhase) & 1)*2.0f - 1.0f;}
	INTRA_FORCEINLINE bool Empty() const {return false;}

private:
	float mPhase, mDeltaPhase;
};

}

INTRA_WARNING_POP
