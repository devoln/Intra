#pragma once



#include "Intra/Range/Span.h"

#include "Random/FastUniformNoise.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

class WhiteNoiseSampler
{
	float mT, mDT;
	float mAmplitude;
public:
	WhiteNoiseSampler(float dt, float amplitude):
		mT(0), mDT(dt), mAmplitude(amplitude) {}

	size_t GenerateMono(Span<float> inOutSamples);
};

struct WhiteNoiseInstrument
{
	float VolumeScale;
	float FreqMultiplier;

	INTRA_FORCEINLINE WhiteNoiseInstrument(decltype(nullptr)=nullptr): VolumeScale(0), FreqMultiplier(1) {}
	INTRA_FORCEINLINE WhiteNoiseInstrument(float volumeScale, float freqMult = 1):
		VolumeScale(volumeScale), FreqMultiplier(freqMult) {}

	WhiteNoiseSampler operator()(float freq, float volume, unsigned sampleRate) const
	{return WhiteNoiseSampler(freq*FreqMultiplier/float(sampleRate), volume*VolumeScale);}

	INTRA_FORCEINLINE explicit operator bool() const noexcept {return VolumeScale > 0;}
	INTRA_FORCEINLINE bool operator==(decltype(nullptr)) const noexcept {return VolumeScale <= 0;}
	INTRA_FORCEINLINE bool operator!=(decltype(nullptr)) const noexcept {return !operator==(nullptr);}
};

INTRA_WARNING_POP
