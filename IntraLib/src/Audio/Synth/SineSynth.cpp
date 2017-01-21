#include "Audio/Synth/SineSynth.h"
#include "Audio/Synth/PeriodicSynth.h"
#include "Platform/CppWarnings.h"
#include "Math/MathRanges.h"
#include "Range/ArrayRange.h"
#include "Algo/Mutation/Copy.h"
#include "Algo/Mutation/Transform.h"
#include "Algo/Mutation/Fill.h"
#include "Math/Random.h"
#include "Containers/Array.h"

namespace Intra { namespace Audio { namespace Synth {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

struct SineParams
{
	float scale;
	ushort harmonics;
	float freqMultiplyer;
};

void PerfectSine(float volume, float freq, uint sampleRate, ArrayRange<float> inOutSamples, bool add)
{
	Math::SineRange<float> sineRange(volume, 0, float(2*Math::PI*freq/sampleRate));
	if(!add) Algo::CopyAdvanceToAdvance(sineRange, inOutSamples.Length(), inOutSamples);
	else Algo::Add(inOutSamples, sineRange);
}

void FastSine(float volume, float freq, uint sampleRate, ArrayRange<float> inOutSamples, bool add)
{
	const double samplesPerPeriod = float(sampleRate)/freq;
	uint count = GetGoodSignalPeriod(samplesPerPeriod, Math::Max(uint(freq/50), 5u));

	//Генерируем фрагмент, который будем повторять, пока не заполним буфер целиком
	Array<float> sineFragment;
	const auto sampleCount = uint(Math::Round(samplesPerPeriod*count));
	sineFragment.SetCountUninitialized(sampleCount);
	PerfectSine(volume, freq, sampleRate, sineFragment, false);

	RepeatFragmentInBuffer(sineFragment, inOutSamples, add);
}

static void SineSynthPassFunction(const SineParams& params,
	float freq, float volume, ArrayRange<float> inOutSamples, uint sampleRate, bool add)
{
	if(inOutSamples==null) return;
	const float newFreq = freq*params.freqMultiplyer;
	float maxValue = 2.0f-2.0f/float(1 << params.harmonics);
	float newVolume = volume*params.scale/maxValue;

	FastSine(newVolume, newFreq, sampleRate, inOutSamples, add);

	Math::Random<float> frandom(1278328923);
	for(ushort h=1; h<params.harmonics; h++)
	{
		newVolume/=2;
		float frequency = newFreq*float(1 << h);
		frequency += frandom()*frequency*0.002f;
		FastSine(newVolume, frequency, sampleRate, inOutSamples.Drop(Math::Random<ushort>::Global(20)), true);
	}
}

SynthPass CreateSineSynthPass(float scale, ushort harmonics, float freqMultiplyer)
{return SynthPass(SineSynthPassFunction, SineParams{scale, harmonics, freqMultiplyer});}


struct MultiSineParams
{
	byte len;
	SineHarmonic harmonics[20];
};

static void MultiSineSynthPassFunction(const MultiSineParams& params,
		float freq, float volume, ArrayRange<float> inOutSamples, uint sampleRate, bool add)
{
	if(inOutSamples==null) return;
	size_t start = Math::Random<ushort>::Global(20);
	if(!add) Algo::FillZeros(inOutSamples.Take(start));
	for(ushort h=0; h<params.len; h++)
	{
		auto& harm = params.harmonics[h];
		FastSine(volume*float(harm.Scale),
			freq*float(harm.FreqMultiplyer), sampleRate,
			inOutSamples.Drop(start),
			add || h>0);
	}
}

SynthPass CreateMultiSineSynthPass(ArrayRange<const SineHarmonic> harmonics)
{
	MultiSineParams params;
	auto src = harmonics.Take(Meta::NumOf(params.harmonics));
	params.len = byte(src.Length());
	Algo::CopyTo(src, params.harmonics);
	return SynthPass(MultiSineSynthPassFunction, params);
}

INTRA_WARNING_POP

}}}
