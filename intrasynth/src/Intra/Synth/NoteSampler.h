#pragma once

#include <Cpp/Fundamental.h>
#include <Cpp/Warnings.h>

#include <Container/Sequential/Array.h>

#include <Audio/MusicNote.h>

#include "WaveFormSampler.h"
#include "WaveTableSampler.h"
#include "WhiteNoiseSampler.h"
#include "ADSR.h"
#include "Sampler.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

class NoteSampler: public Sampler
{
public:
	Array<WaveFormSampler> WaveFormSamplers;
	Array<WaveTableSampler> WaveTableSamplers;
	Array<WhiteNoiseSampler> WhiteNoiseSamplers;
	Array<GenericSamplerRef> GenericSamplers;
	Array<GenericModifier> Modifiers;
	AdsrAttenuator ADSR;
	float Pan = 0;

	size_t GenerateMono(Span<float> ioDst);
	size_t GenerateStereo(Span<float> ioDstLeft, Span<float> ioDstRight);

	void MultiplyPitch(float freqMultiplier) override;
	void NoteRelease() override;
	void SetPan(float pan) override;
	void MultiplyVolume(float volumeMultiplier) override;
	void SetRenderParams(const RenderParams& params) override;

	bool Empty() const noexcept
	{
		return WaveFormSamplers.Empty() && WaveTableSamplers.Empty() &&
			WhiteNoiseSamplers.Empty() && GenericSamplers.Empty();
	}

	// Sampler (task-based) interface.
	void MoveConstruct(void* dst) override {new(dst) NoteSampler(Move(*this));}
	bool Generate(SamplerTaskContainer& dstTasks, size_t offsetInSamples, size_t numSamples) override;

private:
	void fill(Span<float> ioDst);
	void fillStereo(Span<float> ioDstLeft, Span<float> ioDstRight);
	void applyModifiers(Span<float> ioDst);
};

INTRA_WARNING_POP
