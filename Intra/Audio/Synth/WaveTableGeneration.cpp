#include "WaveTableGeneration.h"
#include "Audio/AudioProcessing.h"

#include "Random/FastUniform.h"

#include "Math/SineRange.h"

#include "Range/Mutation/Copy.h"
#include "Range/Mutation/Transform.h"
#include "Range/Reduction.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio { namespace Synth {

void AddSineHarmonicGaussianProfile(Span<float> wavetableAmplitudes, float freqSampleRateRatio,
	float harmFreqMultiplier, float harmBandwidthScale, float amplitude, float bandwidthCents)
{
	const size_t N = wavetableAmplitudes.Length()*2;
	const float bwi = (Math::Pow2(bandwidthCents/1200 - 1) - 0.5f)*freqSampleRateRatio*Math::Pow(harmFreqMultiplier, harmBandwidthScale);
	float rw = -freqSampleRateRatio*harmFreqMultiplier/bwi, rdw = 1.0f/N/bwi;
	if(-2 > rw)
	{
		wavetableAmplitudes.PopFirstN(size_t((-2 - rw) / rdw));
		rw = -2;
	}
	amplitude /= bwi;
	if(rw < 2) wavetableAmplitudes = wavetableAmplitudes.Take(size_t((2 - rw) / rdw));
	while(!wavetableAmplitudes.Empty())
	{
		wavetableAmplitudes.Next() += amplitude * Math::Exp(-Math::Sqr(rw));
		rw += rdw;
	}
}


static void GenerateRandomPhases(Span<float> inOutRealAmplitudes, Span<float> outImagAmplitudes)
{
	INTRA_DEBUG_ASSERT(inOutRealAmplitudes.Length() <= outImagAmplitudes.Length());
	Random::FastUniform<ushort> rand(inOutRealAmplitudes.Length() ^ size_t(inOutRealAmplitudes.Begin));
	enum: uint {TABLE_SIZE = 1024};
	float sineTable[TABLE_SIZE];
	Math::SineRange<float> oscillator(1, 0, float(2*Math::PI/TABLE_SIZE));
	ReadTo(oscillator, sineTable);
	inOutRealAmplitudes.PopFirst();
	outImagAmplitudes.PopFirst();
	while(!inOutRealAmplitudes.Empty())
	{
		float& real = inOutRealAmplitudes.Next();
		float& imag = outImagAmplitudes.Next();
		const size_t phaseIndex = rand(TABLE_SIZE);
		const float cosine = sineTable[(phaseIndex + TABLE_SIZE/4) & (TABLE_SIZE-1)];
		const float sine = sineTable[phaseIndex];
		imag = real*sine;
		real *= cosine;
	}
}

static void ReflectComplexAmplitudes(Span<float> inOutRealAmplitudesX2, Span<float> inOutImagAmplitudesX2)
{
	inOutImagAmplitudesX2[inOutImagAmplitudesX2.Length() / 2] = 0;
	inOutRealAmplitudesX2.PopFirst();
	inOutImagAmplitudesX2.PopFirst();
	while(!inOutRealAmplitudesX2.Empty())
	{
		inOutRealAmplitudesX2.Last() = inOutRealAmplitudesX2.First();
		inOutImagAmplitudesX2.Last() = -inOutImagAmplitudesX2.First();
		inOutRealAmplitudesX2.PopFirst();
		inOutImagAmplitudesX2.PopFirst();
		if(inOutRealAmplitudesX2.Empty()) break;
		inOutRealAmplitudesX2.PopLast();
		inOutImagAmplitudesX2.PopLast();
	}
}

static void NormalizeSamples(Span<float> inOutSamples, float volume = 1)
{
	const auto minimax = MiniMax(inOutSamples.AsConstRange());
	const float multiplier = 2 / (minimax.second - minimax.first);
	const float add = -minimax.first*multiplier - 1;
	MulAdd(inOutSamples, multiplier*volume, add*volume);
}

void ConvertAmplutudesToSamples(Span<float> inAmplitudesX2OutSamples, Span<float> tempBuffer, float volume)
{
	INTRA_DEBUG_ASSERT(tempBuffer.Length() >= inAmplitudesX2OutSamples.Length());
	INTRA_DEBUG_ASSERT(inAmplitudesX2OutSamples.Length() % 2 == 0);
	GenerateRandomPhases(
		inAmplitudesX2OutSamples.Take(inAmplitudesX2OutSamples.Length() / 2),
		tempBuffer.Take(inAmplitudesX2OutSamples.Length() / 2));
	ReflectComplexAmplitudes(inAmplitudesX2OutSamples, tempBuffer.Take(inAmplitudesX2OutSamples.Length()));
	InplaceInverseFFTNonNormalized(inAmplitudesX2OutSamples, tempBuffer.Take(inAmplitudesX2OutSamples.Length()));
	NormalizeSamples(inAmplitudesX2OutSamples, volume);
}

void ConvertAmplitudesToSamples(WaveTable& table, float volume)
{
	INTRA_DEBUG_ASSERT(table.LevelCount == 1);
	INTRA_DEBUG_ASSERT(table.Data.Length() == table.BaseLevelLength/2); //Data содержит не семплы, а амплитуды частот, которых в 2 раза меньше
	table.Data.SetCount(table.BaseLevelLength * 2); //В первой половине будут размещены семплы, а вторая будет временным буфером для алгоритма
	auto amplitudesThenSamples = table.Data.Take(table.BaseLevelLength); //Половина этой части буфера содержит амлитуды частот, приследующем действии сюда запишутся семплы
	auto tempBuffer = table.Data.Drop(table.BaseLevelLength);
	ConvertAmplutudesToSamples(amplitudesThenSamples, tempBuffer, volume);
	table.Data.SetCount(table.BaseLevelLength); //Временный буфер убираем. Место, оставшееся зарезервированным, будет использоваться для хранения уровней детализации
	table.GenerateAllNextLevels();
}

}}}

INTRA_WARNING_POP
