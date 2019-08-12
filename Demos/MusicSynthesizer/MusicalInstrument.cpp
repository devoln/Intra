#include "MusicalInstrument.h"
#include "Core/Range/ForEach.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

NoteSampler MusicalInstrument::operator()(float freq, float volume, uint sampleRate) const
{
	NoteSampler result;

	for(auto& wave: Waves) result.WaveTableSamplers.AddLast(wave(freq, volume, sampleRate));
	for(auto& wave: WaveTables) result.WaveTableSamplers.AddLast(wave(freq, volume, sampleRate));
	if(WhiteNoise) result.WhiteNoiseSamplers.AddLast(WhiteNoise(freq, volume, sampleRate));
	for(auto& instrument: GenericInstruments) result.GenericSamplers.AddLast(instrument(freq, volume, sampleRate));

	if(ExponentAttenuation) result.Modifiers.AddLast(ExponentAttenuation(freq, volume, sampleRate));
	if(ADSR) result.ADSR = ADSR(freq, volume, sampleRate);
	if(Chorus) result.Modifiers.AddLast(Chorus(freq, volume, sampleRate));
	for(auto& mod: GenericModifiers) result.Modifiers.AddLast(mod(freq, volume, sampleRate));

	return result;
}

INTRA_WARNING_POP
