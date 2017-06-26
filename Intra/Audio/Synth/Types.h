#pragma once

#include "Utils/Delegate.h"
#include "Audio/MusicNote.h"
#include "Utils/Span.h"

namespace Intra { namespace Audio {

struct MusicTrack;
namespace Synth {

class IMusicalInstrument
{
public:
	virtual ~IMusicalInstrument() {}

	virtual void GetNoteSamples(Span<float> dst,
		MusicNote note, float tempo, float volume=1, uint sampleRate=44100, bool add=false) const = 0;

	virtual uint GetNoteSampleCount(MusicNote note, float tempo, uint sampleRate=44100) const = 0;
	virtual void PrepareToPlay(const MusicTrack& /*track*/, uint /*sampleRate*/) const {}
};

typedef Utils::Delegate<void(
	float freq, float volume, Span<float> outSamples, uint sampleRate, bool add
)> SynthPass;

typedef Utils::Delegate<void(
	float freq, Span<float> inOutSamples, uint sampleRate
)> ModifierPass;

typedef Utils::Delegate<void(
	float noteDuration, Span<float> inOutSamples, uint sampleRate
)> AttenuationPass;

typedef Utils::Delegate<void(
	Span<float> inOutSamples, uint sampleRate
)> PostEffectPass;

}}}
