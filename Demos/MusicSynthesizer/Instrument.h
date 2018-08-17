#pragma once

#include "Sampler.h"

//! Инструмент-наследник должен быть тривиально перемещаемым,
//! то есть он и его члены не должны иметь указателей и ссылок на свои поля
//! или иметь какую-то другую логику перемещения
class Instrument
{
public:
	virtual ~Instrument() {}

	virtual Sampler& CreateSampler(SamplerContainer& dst,
		float freq, float volume, uint sampleRate, ushort* oIndex = null) = 0;
};

class InstrumentContainer
{
	Array<byte> mInstrumentRawData;
	Array<uint> mInstrumentOffsets;

public:

};
