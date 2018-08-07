#pragma once

#include <Cpp/Fundamental.h>
#include <Cpp/Warnings.h>

#include <Container/Sequential/Array.h>

#include <Audio/MusicNote.h>

#include "WaveTableSampler.h"
#include "WhiteNoiseSampler.h"
#include "ADSR.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

class NoteSampler
{
public:
	Array<WaveTableSampler> WaveTableSamplers;
	Array<WhiteNoiseSampler> WhiteNoiseSamplers;
	Array<GenericSamplerRef> GenericSamplers;
	Array<GenericModifier> Modifiers;
	AdsrAttenuator ADSR;
	float Pan = 0;
	float ReverbCoeff = 0;

	size_t GenerateMono(Span<float> ioDst);
	size_t GenerateStereo(Span<float> ioDstLeft, Span<float> ioDstRight, Span<float> ioDstReverb);

	void MultiplyPitch(float freqMultiplier);
	void NoteRelease();
	void SetPan(float pan);
	void MultiplyVolume(float volumeMultiplier);
	void SetReverbCoeff(float reverbCoeff);

	bool Empty() const noexcept {return (WaveTableSamplers.Empty() && GenericSamplers.Empty()) || ADSR.SamplesLeft() == 0;}

private:
	void fill(Span<float> ioDst);
	void fillStereo(Span<float> ioDstLeft, Span<float> ioDstRight, Span<float> ioDstReverb);
	void applyModifiers(Span<float> ioDst);
};


INTRA_WARNING_POP
