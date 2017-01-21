#pragma once

#include "Platform/CppWarnings.h"
#include "Platform/CppFeatures.h"
#include "Range/ForwardDecls.h"

namespace Intra { namespace Audio { namespace Synth { namespace PostEffects {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

struct Chorus
{
	float MaxDelay;
	float Frequency;
	float MainVolume, SecondaryVolume;

	Chorus(float maxDelay=0.03f, float frequency=3, float mainVolume=0.5f, float secondaryVolume=0.5f):
		MaxDelay(maxDelay), Frequency(frequency), MainVolume(mainVolume), SecondaryVolume(secondaryVolume) {}

	void operator()(ArrayRange<float> inOutSamples, uint sampleRate) const;
};

struct Echo
{
	float Delay;
	float MainVolume, SecondaryVolume;

	Echo(float delay=0.03f, float mainVolume=0.5f, float secondaryVolume=0.5f):
		Delay(delay), MainVolume(mainVolume), SecondaryVolume(secondaryVolume) {}

	void operator()(ArrayRange<float> inOutSamples, uint sampleRate) const;
};

struct FilterDrive
{
	float K;
	FilterDrive(float k): K(k) {}

	void operator()(ArrayRange<float> inOutSamples, uint sampleRate) const;
};

struct FilterHP
{
	float K;
	FilterHP(float k): K(k) {}

	void operator()(ArrayRange<float> inOutSamples, uint sampleRate) const;
};


struct FilterQ
{
	float Frq, K;
	FilterQ(float frq, float k): Frq(frq), K(k) {}

	void operator()(ArrayRange<float> samples, uint sampleRate) const;
};

struct Fade
{
	uint FadeIn, FadeOut;

	Fade(uint fadeIn=0, uint fadeOut=0):
		FadeIn(fadeIn), FadeOut(fadeOut) {}

	void operator()(ArrayRange<float> inOutSamples, uint sampleRate) const;
};

INTRA_WARNING_POP

}}}}
