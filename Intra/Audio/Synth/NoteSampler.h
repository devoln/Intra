#pragma once

#include "Cpp/Fundamental.h"
#include "Cpp/Warnings.h"

#include "Container/Sequential/Array.h"

#include "Audio/MusicNote.h"

#include "WaveTableSampler.h"
#include "ADSR.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio { namespace Synth {


class NoteSampler
{
public:
	Array<WaveTableSampler> WaveTableSamplers;
	Array<GenericSampler> GenericSamplers;
	Array<GenericModifier> Modifiers;
	AdsrAttenuator ADSR;
	float Pan = 0;

	Span<float> operator()(Span<float> dst, bool add);
	size_t operator()(Span<float> dstLeft, Span<float> dstRight, bool add);

	void MultiplyPitch(float freqMultiplier);
	void NoteRelease();

	bool Empty() const noexcept {return WaveTableSamplers.Empty() && GenericSamplers.Empty() || ADSR.SamplesLeft() == 0;}

private:
	void fill(Span<float> dst, bool add);
	void applyModifiers(Span<float> dst);
};


}}}

INTRA_WARNING_POP
