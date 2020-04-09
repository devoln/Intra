#pragma once

#include "Sampler.h"
#include "Extra/Container/Utility/Blob.h"

class Instrument
{
public:
	virtual ~Instrument() {}
	virtual void MoveConstruct(void* dst) = 0;
	virtual Sampler& CreateSampler(float freq, float volume, unsigned sampleRate,
		SamplerContainer& dst, uint16* oIndex = null) const = 0;
};

typedef Intra::Container::DynamicBlob<Instrument, alignof(Instrument), uint16> InstrumentContainer;
