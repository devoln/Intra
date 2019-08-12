#pragma once

#include "Sampler.h"
#include "Container/Utility/Blob.h"

class Instrument
{
public:
	virtual ~Instrument() {}
	virtual void MoveConstruct(void* dst) = 0;
	virtual Sampler& CreateSampler(float freq, float volume, uint sampleRate,
		SamplerContainer& dst, ushort* oIndex = null) const = 0;
};

typedef Intra::Container::DynamicBlob<Instrument, alignof(Instrument), ushort> InstrumentContainer;
