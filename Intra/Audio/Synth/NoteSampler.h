#pragma once

#include "Cpp/Fundamental.h"
#include "Cpp/Warnings.h"

#include "Container/Sequential/Array.h"

#include "Audio/MusicNote.h"

#include "WaveTableSampler.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio { namespace Synth {

class NoteSampler
{
	Array<WaveTableSampler> PeriodicSamplers;
	Array<GenericSampler> GenericSamplers;
	Array<GenericModifier> Modifiers;

	Span<float> operator()(Span<float> dst, bool add);

private:
	void fill(Span<float> dst, bool add);
};

}}}

INTRA_WARNING_POP
