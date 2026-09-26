#include "RecordedSampler.h"

#include <Math/Math.h>

#include <Range/Mutation/Copy.h>
#include <Range/Mutation/Fill.h>
#include <Range/Mutation/Transform.h>

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

size_t RecordedSampler::operator()(Span<float> dst)
{
	size_t len = dst.Length();
	AddMultipliedAdvance(dst, Data, Volume);
	return len - dst.Length();
}

GenericSamplerRef CachedDrumInstrument::operator()(float volume, unsigned sampleRate) const
{
	return Truncated(volume, sampleRate, SampleCount);
}

GenericSamplerRef CachedDrumInstrument::Truncated(float volume, unsigned sampleRate, size_t sampleCount44100) const
{
	if(SampleRate != sampleRate)
	{
		SampleRate = sampleRate;
		// Always build the full physical source once. Shorter drum variants are
		// prefix views of this shared cache and never rerun the source model.
		const size_t fullCount = Math::Max(size_t(1),
			size_t(double(SampleCount)*sampleRate/44100.0 + 0.5));
		Data.SetCountUninitialized(fullCount);
		FillZeros(Data.AsRange());
		DataSampler->GenerateMono(Data);
		float u = 1;
		LinearMultiply(Data.Tail(300), u, -0.00333f);
	}
	const size_t targetCount = Math::Min(Data.Length(), Math::Max(size_t(1),
		size_t(double(sampleCount44100)*sampleRate/44100.0 + 0.5)));
	return new RecordedSampler{Data.AsRange().Take(targetCount), volume*VolumeScale, 1};
}

INTRA_WARNING_POP
