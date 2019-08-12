#pragma once

#include "SamplerTask.h"
#include "Envelope.h"

class FixedRateTask: public SamplerTask
{
public:
	CSpan<float> Source;
	EnvelopeSegment Attenuator;
	size_t ChannelIndex;
	float Rate;

	forceinline FixedRateTask(size_t channelIndex, size_t offset, CSpan<float> source, EnvelopeSegment attenuator, float rate):
		SamplerTask(offset, source.Length()), ChannelIndex(channelIndex), Source(source), Attenuator(attenuator), Rate(rate) {}

	void MoveConstruct(void* dst) override {new(dst) FixedRateTask(Move(*this));}

	void operator()(SamplerTaskContext& stc) override {Attenuator(stc.Channels[ChannelIndex], Source);}
};
