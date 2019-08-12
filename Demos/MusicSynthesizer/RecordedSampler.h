#pragma once

#include <Core/Warnings.h>
#include <Core/Span.h>
#include <Container/Sequential/Array.h>

#include "Types.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

struct RecordedSampler
{
	CSpan<float> Data;
	float Volume;
	float PlaybackRate;

	size_t operator()(Span<float> dst);
};

struct CachedDrumInstrument
{
	mutable Array<float> Data;
	mutable GenericSamplerRef DataSampler;
	mutable uint SampleRate = 0;
	float VolumeScale;

	CachedDrumInstrument(const CachedDrumInstrument&) = delete;
	CachedDrumInstrument& operator=(const CachedDrumInstrument&) = delete;


	CachedDrumInstrument(GenericSamplerRef sampler, size_t sampleCount = 44100, float volumeScale = 1):
		DataSampler(Move(sampler)), VolumeScale(volumeScale) {Data.SetCountUninitialized(sampleCount);}

	RecordedSampler operator()(float volume, uint sampleRate) const;

	void Preload(uint sampleRate = 44100) const {operator()(1, sampleRate);}
};

INTRA_WARNING_POP
