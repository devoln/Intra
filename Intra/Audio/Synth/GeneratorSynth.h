#pragma once

#include "Cpp/Warnings.h"
#include "Audio/Synth/Generators/WhiteNoise.h"
#include "Utils/Span.h"
#include "Random/FastUniform.h"

namespace Intra { namespace Audio { namespace Synth {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace D {

template<typename T> struct SamplerPassParams
{
	SamplerPassParams(const T& generator, ushort harmonics, float scale, float freqMultiplyer):
		Generator(generator), Harmonics(harmonics), Scale(scale), FreqMultiplyer(freqMultiplyer) {}

	SamplerPassParams(T&& generator, ushort harmonics, float scale, float freqMultiplyer):
		Generator(Cpp::Move(generator)), Harmonics(harmonics), Scale(scale), FreqMultiplyer(freqMultiplyer) {}

	T Generator;
	ushort Harmonics;
	float Scale;
	float FreqMultiplyer;
};

template<typename T> void GeneratorSynthPassFunction(const SamplerPassParams<T>& params,
	float freq, float volume, Span<float> outSamples, uint sampleRate, bool add)
{
	const float dt = 1.0f/float(sampleRate);

	//Считаем громкость основной гармоники так, чтобы суммарная громкость по всем гармоникам не превышала scale
	float maxValue = 2.0f-2.0f/float(1 << params.Harmonics);
	float newVolume = volume*params.Scale/maxValue;

	float frequency = freq*params.FreqMultiplyer;

	Random::FastUniform<float> frandom(988959283);
	for(ushort h=0; h<params.Harmonics; h++)
	{
		auto samplerCopy = params.Generator;
		samplerCopy.SetParams(frequency, newVolume, dt);
		if(h==0 && !add) for(auto& sample: outSamples)
			sample = samplerCopy.NextSample();
		else for(auto& sample: outSamples)
			sample += samplerCopy.NextSample();
		newVolume /= 2;
		frequency *= 2+frandom(0.0005f);
	}
}

struct WhiteNoisePassParams
{
	ushort Harmonics;
	float Scale;
	float FreqMultiplyer;
};

void WhiteNoiseSynthPassFunction(const WhiteNoisePassParams& params,
	float freq, float volume, Span<float> outSamples, uint sampleRate, bool add)
{
	SamplerPassParams<Generators::WhiteNoise> p(
		Generators::WhiteNoise(), params.Harmonics, params.Scale, params.FreqMultiplyer);
	GeneratorSynthPassFunction(p, freq, volume, outSamples, sampleRate, add);
}

}

template<typename T> SynthPass CreateGeneratorSynthPass(
	T generator, float scale=1, ushort harmonics=1, float freqMultiplyer=1)
{
	D::SamplerPassParams<T> params = {generator, harmonics, scale, freqMultiplyer};
	return SynthPass(D::GeneratorSynthPassFunction<T>, params);
}

SynthPass CreateWhiteNoiseSynthPass(float scale=1, ushort harmonics=1, float freqMultiplyer=1)
{
	D::WhiteNoisePassParams params = {harmonics, scale, freqMultiplyer};
	return SynthPass(D::WhiteNoiseSynthPassFunction, params);
}

INTRA_WARNING_POP

}}}
