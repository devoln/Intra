#include "Audio/AudioBuffer.h"
#include "Core/Range/Reduce.h"
#include "Core/Range/Span.h"
#include "Core/Range/Map.h"
#include "Core/Range/Mutation/Fill.h"
#include "Core//Range/Mutation/Copy.h"

INTRA_BEGIN
AudioBuffer::AudioBuffer(size_t sampleCount,
	uint sampleRate, CSpan<float> initData):
	SampleRate(sampleRate), Samples()
{
	if(!initData.Empty()) Samples.AddLastRange(initData);
	Samples.SetCount(sampleCount);
}

void AudioBuffer::ShiftSamples(intptr samplesToShift)
{
	if(samplesToShift == 0 || Samples.Empty()) return;
	if(size_t(Abs(samplesToShift)) >= Samples.Count())
	{
		FillZeros(Samples);
		return;
	}
	if(samplesToShift < 0)
	{
		const auto numSamplesMoved = CopyTo(Samples.Drop(size_t(-samplesToShift)), Samples);
		FillZeros(Samples.Drop(numSamplesMoved));
		return;
	}
	CopyTo(Samples, Samples.Drop(size_t(samplesToShift)));
	FillZeros(Samples.Take(size_t(samplesToShift)));
}
INTRA_END
