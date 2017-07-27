#pragma once

#include "Cpp/Warnings.h"
#include "Container/Sequential/Array.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio { namespace Synth {

class TubeSampler
{
	Array<float> mSamples;
	float mPhaseDelta, mDetuneFactor, mRandCoeff;
	float mFF, mA, mP, mS;
	uint mSeed;
public:
	TubeSampler(float dphi, float volume, float detuneVal = 1, float detuneFactor = 0.001f, float kRand = 1);

	Span<float> operator()(Span<float> dst, bool add);

	void MultiplyPitch(float freqMultiplier)
	{
		mPhaseDelta *= freqMultiplier;
		mFF *= freqMultiplier;
	}
};

struct TubeInstrument
{
	float Scale = 0;
	float DetuneValue = 1;
	float DetuneFactor = 0.001f;
	float RandomCoeff = 1;

	TubeSampler operator()(float freq, float volume, uint sampleRate) const
	{return {freq*2*float(Math::PI)/float(sampleRate), volume*Scale, DetuneValue, DetuneFactor, RandomCoeff};}
};

}}}

INTRA_WARNING_POP
