﻿#include "Audio/Synth/SawtoothSynth.h"
#include "Audio/Synth/Generators/Sawtooth.h"
#include "Audio/Synth/PeriodicSynth.h"
#include "Range/Generators/ArrayRange.h"
#include "Platform/CppWarnings.h"
#include "Algo/Mutation/Copy.h"
#include "Algo/Mutation/Transform.h"
#include "Containers/Array.h"
#include "Math/Random.h"

namespace Intra { namespace Audio { namespace Synth {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

void PerfectSawtooth(double upPercent, float volume,
	float freq, uint sampleRate, ArrayRange<float> inOutSamples, bool add)
{
	Generators::Sawtooth saw(float(upPercent/(1.0-upPercent)));
	saw.SetParams(freq, volume, 1.0/sampleRate);
	if(!add) Algo::CopyAdvanceToAdvance(saw, inOutSamples);
	else Algo::Add(inOutSamples, saw);
}


void FastSawtooth(double upPercent, float volume, float freq,
	uint sampleRate, ArrayRange<float> inOutSamples, bool add)
{
	const double samplesPerPeriod = float(sampleRate)/freq;
	uint count = GetGoodSignalPeriod(samplesPerPeriod, Math::Max(uint(freq/50), 5u));

	//Генерируем фрагмент, который будем повторять, пока не заполним буфер целиком
	Array<float> samples;
	const auto sampleCount = uint(Math::Round(samplesPerPeriod*count));
	samples.SetCountUninitialized(sampleCount);
	PerfectSawtooth(upPercent, volume, freq, sampleRate, samples, false);

	RepeatFragmentInBuffer(samples, inOutSamples, add);
}

struct SawtoothParams
{
	float UpdownRatio;
	ushort Harmonics;
	float Scale;
	float FreqMultiplyer;
};

void SawtoothSynthPassFunction(const SawtoothParams& params,
		float freq, float volume, ArrayRange<float> inOutSamples, uint sampleRate, bool add)
{
	if(inOutSamples==null) return;
	double updownPercent = params.UpdownRatio/(params.UpdownRatio+1);
	float newFreq=freq*params.FreqMultiplyer;
	float maxValue = 2.0f-2.0f/float(1 << params.Harmonics);
	float newVolume = volume*params.Scale/maxValue;

	FastSawtooth(updownPercent, newVolume, newFreq, sampleRate, inOutSamples, add);

	Math::Random<float> frandom(612651278);
	for(ushort h=1; h<params.Harmonics; h++)
	{
		newVolume/=2;
		float frequency = newFreq*float(1 << h);
		frequency += frandom()*frequency*0.002f;
		size_t randomSampleOffset = Math::Random<ushort>::Global(20);
		FastSawtooth(updownPercent, newVolume, frequency, sampleRate, inOutSamples.Drop(randomSampleOffset), true);
	}
}

SynthPass CreateSawtoothSynthPass(float updownRatio, float scale, ushort harmonics, float freqMultiplyer)
{return SynthPass(SawtoothSynthPassFunction, SawtoothParams{updownRatio, harmonics, scale, freqMultiplyer});}

INTRA_WARNING_POP

}}}