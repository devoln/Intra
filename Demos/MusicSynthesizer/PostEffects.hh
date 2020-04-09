#pragma once

#include <Core/Warnings.h>
#include <Core/Features.h>

#include <Core/Span.h>
#include <Utils/FixedArray.h>

#include <Math/SineRange.h>

#include "Types.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace PostEffects {

struct Echo
{
	float Delay;
	float MainVolume, SecondaryVolume;

	Echo(float delay=0.03f, float mainVolume=0.5f, float secondaryVolume=0.5f):
		Delay(delay), MainVolume(mainVolume), SecondaryVolume(secondaryVolume) {}

	void operator()(Span<float> inOutSamples, unsigned sampleRate) const;
};

struct FilterDrive
{
	float K;
	FilterDrive(float k): K(k) {}

	void operator()(Span<float> inOutSamples, unsigned sampleRate) const;
};

struct FilterHP
{
	float K;
	FilterHP(float k): K(k) {}

	static FilterHP FromCutoff(float cutoffFreqSampleRateRatio)
	{
		return {1.0f / (2*float(PI)*cutoffFreqSampleRateRatio + 1)};
	}

	static FilterHP FromCutoff(float cutoffFreq, unsigned sampleRate)
	{
		return FromCutoff(cutoffFreq / float(sampleRate));
	}

	void operator()(Span<float> inOutSamples, unsigned sampleRate) const;
};


struct FilterQ
{
	float F, K;
	float P, S;
	FilterQ(float f, float k): F(f), K(k), P(0), S(0) {}

	INTRA_FORCEINLINE float operator()(float sample)
	{
		P += S*F + sample;
		S = (S - P*F)*K;
		return P;
	}

	void operator()(Span<float> samples)
	{
		for(float& sample: samples)
			sample = operator()(sample);
	}
};

struct FilterQFactory
{
	float ResFreq, K;
	FilterQ operator()(unsigned sampleRate) {return FilterQ(float(ResFreq*2*PI/float(sampleRate)), K);}
};

struct Fade
{
	unsigned FadeIn, FadeOut;

	Fade(unsigned fadeIn=0, unsigned fadeOut=0):
		FadeIn(fadeIn), FadeOut(fadeOut) {}

	void operator()(Span<float> inOutSamples, unsigned sampleRate) const;
};

class HallReverb
{
	struct Delay
	{
		size_t Offset;
		float LeftVolume, RightVolume;
	};
	FixedArray<Delay> mD;
	FixedArray<float> mAccum;
	float mK = 0;
	float mS = 0;
	float mRF = 0;
	size_t mAccumIndex = 0;
	size_t mMaxDelay = 0;
	size_t mBufferedReverbSamples = 0;
public:
	HallReverb(decltype(null)=null) {}
	HallReverb(size_t delayLength, size_t numDelays, float reverbVolume=1, float k=0.5f);
	void ProcessSample(float* ioL, float* ioR, float reverbSample);
	void operator()(Span<float> dstLeft, Span<float> dstRight, CSpan<float> reverbBuffer);
	INTRA_FORCEINLINE bool operator==(decltype(null)) const noexcept {return mAccum.Empty();}
	INTRA_FORCEINLINE bool operator!=(decltype(null)) const noexcept {return !operator==(null);}
	INTRA_FORCEINLINE operator bool() const noexcept {return operator!=(null);}

};

}

INTRA_WARNING_POP
