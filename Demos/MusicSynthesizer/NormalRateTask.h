#pragma once

#include "SamplerTask.h"
#include "Envelope.h"

class NormalRateTask: public SamplerTask
{
public:
	CSpan<float> Source;
	EnvelopeSegment Attenuator;
	size_t ChannelIndex;

	forceinline NormalRateTask(size_t channelIndex, size_t offset, CSpan<float> source, EnvelopeSegment attenuator):
		SamplerTask(offset, source.Length()), ChannelIndex(channelIndex), Source(source), Attenuator(attenuator) {}

	void MoveConstruct(void* dst) override {new(dst) NormalRateTask(Move(*this));}

	void operator()(SamplerTaskContext& stc) override {Attenuator(stc.Channels[ChannelIndex], Source);}
};
