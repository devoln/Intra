#pragma once

#include <Cpp/Warnings.h>
#include <Cpp/Features.h>

#include <Utils/Span.h>
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

	void operator()(Span<float> inOutSamples, uint sampleRate) const;
};

struct FilterDrive
{
	float K;
	FilterDrive(float k): K(k) {}

	void operator()(Span<float> inOutSamples, uint sampleRate) const;
};

struct FilterHP
{
	float K;
	FilterHP(float k): K(k) {}

	static FilterHP FromCutoff(float cutoffFreqSampleRateRatio)
	{
		return {1.0f / (2*float(Math::PI)*cutoffFreqSampleRateRatio + 1)};
	}

	static FilterHP FromCutoff(float cutoffFreq, uint sampleRate)
	{
		return FromCutoff(cutoffFreq / float(sampleRate));
	}

	void operator()(Span<float> inOutSamples, uint sampleRate) const;
};


struct FilterQ
{
	float F, K;
	float P, S;
	FilterQ(float f, float k): F(f), K(k), P(0), S(0) {}

	forceinline float operator()(float sample)
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
	FilterQ operator()(uint sampleRate) {return FilterQ(float(ResFreq*2*Math::PI/float(sampleRate)), K);}
};

struct Fade
{
	uint FadeIn, FadeOut;

	Fade(uint fadeIn=0, uint fadeOut=0):
		FadeIn(fadeIn), FadeOut(fadeOut) {}

	void operator()(Span<float> inOutSamples, uint sampleRate) const;
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
	HallReverb(null_t=null) {}
	HallReverb(size_t delayLength, size_t numDelays, float reverbVolume=1, float k=0.5f);
	void ProcessSample(float* ioL, float* ioR, float reverbSample);
	void operator()(Span<float> dstLeft, Span<float> dstRight, CSpan<float> reverbBuffer);
	forceinline bool operator==(null_t) const noexcept {return mAccum.Empty();}
	forceinline bool operator!=(null_t) const noexcept {return !operator==(null);}
	forceinline operator bool() const noexcept {return operator!=(null);}

};

}

INTRA_WARNING_POP
