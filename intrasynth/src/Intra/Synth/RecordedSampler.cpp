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
	if(SampleRate != sampleRate)
	{
		SampleRate = sampleRate;
		// Масштабируем длину сэмпла под фактический sample rate, чтобы один и
		// тот же удар звучал одинаково на любой частоте дискретизации.
		const size_t targetCount = Math::Max(size_t(1),
			size_t(double(SampleCount)*sampleRate/44100.0 + 0.5));
		Data.SetCountUninitialized(targetCount);
		FillZeros(Data.AsRange());
		DataSampler->GenerateMono(Data, nullptr);
		float u = 1;
		LinearMultiply(Data.Tail(300), u, -0.00333f);
	}
	return new RecordedSampler{Data, volume*VolumeScale, 1};
}

INTRA_WARNING_POP
