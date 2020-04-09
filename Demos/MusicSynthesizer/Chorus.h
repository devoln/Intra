#pragma once

#include <Core/Warnings.h>
#include <Core/Features.h>

#include <Core/Span.h>

#include <Container/Sequential/Array.h>

#include <Math/Math.h>
#include <Math/SineRange.h>

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

struct Chorus
{
	Array<float> DelayCircularBuffer;
	size_t CircularBufferOffset;
	float MainVolume, SecondaryVolume;
	SineRange<float> Oscillator;

	Chorus(size_t maxDelaySamples, float delayFreqPerSample, float mainVolume=0.5f, float secondaryVolume=0.5f):
		DelayCircularBuffer(maxDelaySamples), CircularBufferOffset(0),
		MainVolume(mainVolume), SecondaryVolume(secondaryVolume),
		Oscillator(float(maxDelaySamples)*0.5f, float(-PI/2), float(2*PI*delayFreqPerSample)) {}

	void operator()(Span<float> inOutSamples);
};

struct ChorusFactory
{
	float MaxDelay;
	float DelayFrequency;
	float MainVolume, SecondaryVolume;

	ChorusFactory(decltype(null)=null): MaxDelay(0), DelayFrequency(0), MainVolume(0), SecondaryVolume(0) {}

	ChorusFactory(float maxDelay, float delayFreq = 3,
		float mainVolume = 0.5f, float secondaryVolume = 0.5f):
		MaxDelay(maxDelay), DelayFrequency(delayFreq),
		MainVolume(mainVolume), SecondaryVolume(secondaryVolume) {}

	Chorus operator()(float freq, float volume, unsigned sampleRate) const
	{
		(void)freq; (void)volume;
		return Chorus(size_t(MaxDelay*float(sampleRate)), DelayFrequency/float(sampleRate), MainVolume, SecondaryVolume);
	}

	INTRA_FORCEINLINE bool operator==(decltype(null)) const noexcept {return MaxDelay == 0 || SecondaryVolume == 0;}
	INTRA_FORCEINLINE bool operator!=(decltype(null)) const noexcept {return !operator==(null);}
	INTRA_FORCEINLINE explicit operator bool() const noexcept {return operator!=(null);}
};

INTRA_WARNING_POP
