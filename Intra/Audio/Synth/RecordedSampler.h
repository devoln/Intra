#pragma once

#include "Cpp/Warnings.h"
#include "Utils/Span.h"
#include "Container/Sequential/Array.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio { namespace Synth {

struct RecordedSampler
{
	CSpan<float> Data;
	float Volume;

	Span<float> operator()(Span<float> dst, bool add);
};

struct RecordedDrumInstrument
{
	Array<float> Data;
	float VolumeScale = 1;

	RecordedSampler operator()(float volume, uint sampleRate) {return {Data, volume*VolumeScale};}

};

}}}

INTRA_WARNING_POP
