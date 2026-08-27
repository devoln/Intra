#pragma once

#include <Cpp/Warnings.h>
#include <Utils/Span.h>
#include <Container/Sequential/Array.h>

#include "Types.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

struct RecordedSampler: public IGenericSampler
{
	Span<const float> Data;
	float Volume;
	float PlaybackRate;

	RecordedSampler(CSpan<float> data, float volume, float playbackRate):
		Data(data), Volume(volume), PlaybackRate(playbackRate) {}

	size_t operator()(Span<float> dst);

	size_t GenerateMono(Span<float> ioDst) override
	{
		return operator()(ioDst);
	}

	size_t GenerateStereo(Span<float> ioDst, Span<float> ioDstRight) override
	{
		(void)ioDstRight;
		return operator()(ioDst);
	}
};

struct CachedDrumInstrument
{
	mutable Array<float> Data;
	mutable GenericSamplerRef DataSampler;
	mutable unsigned SampleRate = 0;
	/// Длина сэмпла на 44100 Гц; при другом sample rate масштабируется,
	/// чтобы удар звучал одинаково на любой частоте дискретизации.
	size_t SampleCount;
	float VolumeScale;

	CachedDrumInstrument(const CachedDrumInstrument&) = delete;
	CachedDrumInstrument& operator=(const CachedDrumInstrument&) = delete;
	CachedDrumInstrument(CachedDrumInstrument&&) = default;
	CachedDrumInstrument& operator=(CachedDrumInstrument&&) = default;

	CachedDrumInstrument(GenericSamplerRef sampler, size_t sampleCount = 44100, float volumeScale = 1):
		DataSampler(Move(sampler)), SampleCount(sampleCount), VolumeScale(volumeScale)
	{Data.SetCountUninitialized(sampleCount);}

	template<typename F> CachedDrumInstrument(F sampler, size_t sampleCount, float volumeScale):
		DataSampler(new FunctorGenericSampler<F>(Move(sampler))), SampleCount(sampleCount), VolumeScale(volumeScale)
	{Data.SetCountUninitialized(sampleCount);}

	GenericSamplerRef operator()(float volume, unsigned sampleRate) const;

	void Preload(unsigned sampleRate = 44100) const {operator()(1, sampleRate);}
};

INTRA_WARNING_POP
