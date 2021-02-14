#include "IntraX/Unstable/Audio/AudioBuffer.h"
#include "Intra/Range/Reduce.h"
#include "Intra/Range/Span.h"
#include "Intra/Range/Map.h"
#include "Intra/Range/Mutation/Fill.h"
#include "Intra//Range/Mutation/Copy.h"

namespace Intra { INTRA_BEGIN
AudioBuffer::AudioBuffer(Index sampleCount, NonNegative<int> sampleRate, Span<const float> initData):
	SampleRate(sampleRate)
{
	if(!initData.Empty()) Samples.AddLastRange(initData);
	Samples.SetCount(sampleCount);
}

void AudioBuffer::ShiftSamples(index_t samplesToShift)
{
	if(samplesToShift == 0 || Samples.Empty()) return;
	if(Abs(samplesToShift) >= Samples.Count())
	{
		FillZeros(Samples);
		return;
	}
	if(samplesToShift < 0)
	{
		const auto numSamplesMoved = CopyTo(Samples.Drop(-samplesToShift), Samples);
		FillZeros(Samples.Drop(numSamplesMoved));
		return;
	}
	CopyTo(Samples, Samples.Drop(samplesToShift));
	FillZeros(Samples.Take(samplesToShift));
}
} INTRA_END
