#pragma once

#include "SamplerTask.h"
#include "Envelope.h"

class NormalRateTask: public SamplerTask
{
public:
	Span<const float> Source;
	EnvelopeSegment Attenuator;
	size_t ChannelIndex;

	INTRA_FORCEINLINE NormalRateTask(size_t channelIndex, size_t offset, Span<const float> source, EnvelopeSegment attenuator):
		SamplerTask(offset, source.Length()), ChannelIndex(channelIndex), Source(source), Attenuator(attenuator) {}

	void MoveConstruct(void* dst) override {new(dst) NormalRateTask(Move(*this));}

	void operator()(SamplerTaskContext& stc) override
	{
		stc.UsedChannels |= (1 << ChannelIndex);
		Attenuator(stc.Channels[ChannelIndex].Drop(OffsetInSamples).Take(Source.Length()), Source);
	}
};
