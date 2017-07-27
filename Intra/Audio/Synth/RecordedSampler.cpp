#include "RecordedSampler.h"

#include "Range/Mutation/Copy.h"
#include "Range/Mutation/Transform.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio { namespace Synth {

Span<float> RecordedSampler::operator()(Span<float> dst, bool add)
{
	if(!add) MultiplyAdvance(dst, Data, Volume);
	else AddMultipliedAdvance(dst, Data, Volume);
	return dst;
}

RecordedSampler CachedDrumInstrument::operator()(float volume, uint sampleRate) const
{
	if(SampleRate == 0)
	{
		SampleRate = sampleRate;
		DataSampler(Data, false);
		float u = 1;
		LinearMultiply(Data.Tail(300), u, -0.00333f);
	}
	return {Data, volume*VolumeScale, float(SampleRate)/float(sampleRate)};
}


}}}

INTRA_WARNING_POP
