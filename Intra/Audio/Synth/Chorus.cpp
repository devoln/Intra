#include "Chorus.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio { namespace Synth {

void Chorus::operator()(Span<float> inOutSamples)
{
	const float oscillatorOffset = DelayCircularBuffer.Length()*0.5f + 0.5f;
	for(float& sample: inOutSamples)
	{
		DelayCircularBuffer[CircularBufferOffset++] = sample;
		if(CircularBufferOffset == DelayCircularBuffer.Length()) CircularBufferOffset = 0;

		const size_t delayInSamples = size_t(Oscillator.Next() + oscillatorOffset);
		size_t sampleIndex = CircularBufferOffset;
		if(sampleIndex > delayInSamples) sampleIndex += DelayCircularBuffer.Length();
		sampleIndex -= delayInSamples;

		sample *= MainVolume;
		sample += DelayCircularBuffer[sampleIndex]*SecondaryVolume;
	}
}

}}}

INTRA_WARNING_POP
