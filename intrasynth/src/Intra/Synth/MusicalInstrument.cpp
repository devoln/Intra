#include "MusicalInstrument.h"
#include "Intra/Range/ForEach.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

NoteSampler MusicalInstrument::BuildNoteSampler(float freq, float volume, unsigned sampleRate) const
{
	NoteSampler result;

	for(auto& wave: Waves) result.WaveFormSamplers.AddLast(wave(freq, volume, sampleRate));
	for(auto& wave: WaveTables) result.WaveTableSamplers.AddLast(wave(freq, volume, sampleRate));
	if(WhiteNoise) result.WhiteNoiseSamplers.AddLast(WhiteNoise(freq, volume, sampleRate));
	for(auto& instrument: GenericInstruments) result.GenericSamplers.AddLast(instrument(freq, volume, sampleRate));

	if(ExponentAttenuation) result.Modifiers.AddLast(ExponentAttenuation(freq, volume, sampleRate));
	if(Envelope) result.ADSR = AdsrAttenuator(Envelope(sampleRate));
	if(Chorus) result.Modifiers.AddLast(Chorus(freq, volume, sampleRate));
	for(auto& mod: GenericModifiers) result.Modifiers.AddLast(mod(freq, volume, sampleRate));

	return result;
}

Sampler& MusicalInstrument::CreateSampler(float freq, float volume, unsigned sampleRate,
	SamplerContainer& dst, uint16* oIndex) const
{
	auto& stored = dst.Add<NoteSampler>(BuildNoteSampler(freq, volume, sampleRate));
	if(oIndex) *oIndex = uint16(dst.Length() - 1);
	return stored;
}

INTRA_WARNING_POP
