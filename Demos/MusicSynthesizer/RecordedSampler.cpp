#include "RecordedSampler.h"

#include <Range/Mutation/Copy.h>
#include <Range/Mutation/Transform.h>

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

size_t RecordedSampler::operator()(Span<float> dst)
{
	size_t len = dst.Length();
	AddMultipliedAdvance(dst, Data, Volume);
	return len - dst.Length();
}

RecordedSampler CachedDrumInstrument::operator()(float volume, uint sampleRate) const
{
	if(SampleRate == 0)
	{
		SampleRate = sampleRate;
		DataSampler->GenerateMono(Data, false);
		float u = 1;
		LinearMultiply(Data.Tail(300), u, -0.00333f);
	}
	return {Data, volume*VolumeScale, float(SampleRate)/float(sampleRate)};
}

INTRA_WARNING_POP
