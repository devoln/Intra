#pragma once

#include "Cpp/Warnings.h"
#include "Utils/Span.h"
#include "Container/Sequential/Array.h"
#include "Types.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio { namespace Synth {

struct RecordedSampler
{
	CSpan<float> Data;
	float Volume;
	float PlaybackRate;

	Span<float> operator()(Span<float> dst, bool add);
};

struct CachedDrumInstrument
{
	mutable Array<float> Data;
	mutable GenericSampler DataSampler;
	mutable uint SampleRate = 0;
	float VolumeScale;

	CachedDrumInstrument(GenericSampler sampler, size_t sampleCount = 44100, float volumeScale = 1):
		DataSampler(Cpp::Move(sampler)), VolumeScale(volumeScale) {Data.SetCountUninitialized(sampleCount);}

	RecordedSampler operator()(float volume, uint sampleRate) const;

	void Preload(uint sampleRate = 44100) const {operator()(1, sampleRate);}
};

}}}

INTRA_WARNING_POP
