#pragma once

#include "SamplerTask.h"
#include "Envelope.h"

class FixedRateTask: public SamplerTask
{
public:
	Span<const float> Source;
	EnvelopeSegment Attenuator;
	size_t ChannelIndex;
	float Rate;

	INTRA_FORCEINLINE FixedRateTask(size_t channelIndex, size_t offset, Span<const float> source, EnvelopeSegment attenuator, float rate):
		SamplerTask(offset, source.Length()), ChannelIndex(channelIndex), Source(source), Attenuator(attenuator), Rate(rate) {}

	void MoveConstruct(void* dst) override {new(dst) FixedRateTask(Move(*this));}

	void operator()(SamplerTaskContext& stc) override
	{
		stc.UsedChannels |= (1 << ChannelIndex);
		Attenuator(stc.Channels[ChannelIndex].Drop(OffsetInSamples).Take(Source.Length()), Source);
	}
};
