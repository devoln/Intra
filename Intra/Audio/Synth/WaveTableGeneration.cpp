#include "WaveTableGeneration.h"
#include "Audio/AudioProcessing.h"

#include "Random/FastUniform.h"

#include "Math/SineRange.h"

#include "Range/Mutation/Copy.h"
#include "Range/Mutation/Transform.h"
#include "Range/Reduction.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio { namespace Synth {

void AddSineHarmonic(Span<float> wavetableAmplitudes, float freqSampleRateRatio, float harmFreqMultiplier, float amplitude)
{
	const size_t index = size_t(Math::Round(freqSampleRateRatio*harmFreqMultiplier*float(2*wavetableAmplitudes.Length())));
	if(index >= wavetableAmplitudes.Length()) return;
	wavetableAmplitudes[index] += amplitude;
}

void AddSineHarmonicGaussianProfile(Span<float> wavetableAmplitudes, float freqSampleRateRatio,
	float harmFreqMultiplier, float harmBandwidthScale, float amplitude, float bandwidthCents)
{
	const size_t N = wavetableAmplitudes.Length()*2;

	//Эта переменная играет роль обратного нормирующего коэффициента sigma*sqrt(2*pi) в формуле нормального распределения.
	//Пропорционален ширине гармоники - корня дисперсии sigma нормального распределения.
	double bwi = (Math::Pow2(bandwidthCents/1200 - 1) - 0.5)*freqSampleRateRatio;

	//Ширина гармоники растёт с её номером в ряду
	bwi *= Math::Pow(double(harmFreqMultiplier), double(harmBandwidthScale));

	//Избегаем проблем с точностью при делении на очень маленькое число
	if(bwi < 0.0000000001)
	{
		AddSineHarmonic(wavetableAmplitudes, freqSampleRateRatio, harmFreqMultiplier, amplitude);
		return;
	}

	//Эта переменная играет роль (x-a) / (sigma*sqrt(2)) в формуле нормального распределения
	double rw = -freqSampleRateRatio*harmFreqMultiplier/bwi;

	//Эта переменная играет роль приращения rw за семпл, что соответствует приращению x в формуле нормального распределения
	const double rdw = 1.0/(double(N)*bwi);
	
	double range = 2;
	if(rdw > 1) range = 3*rdw;
	if(-range > rw)
	{
		const double elementsToSkip = Math::Floor((-range - rw) / rdw);
		wavetableAmplitudes.PopFirstN(size_t(elementsToSkip));
		rw += elementsToSkip*rdw;
	}
	if(rw < range) wavetableAmplitudes = wavetableAmplitudes.Take(size_t(Math::Ceil((range - rw) / rdw)));

	const double A = amplitude*(Math::SqrtPI/2);
	double erf = Math::Erf(rw);
	while(!wavetableAmplitudes.Empty())
	{
		rw += rdw;
		const double erfNext = Math::Erf(rw);
		wavetableAmplitudes.Next() += float(A * (erfNext - erf));
		erf = erfNext;
	}
}


static void GenerateRandomPhases(Span<float> inOutRealAmplitudes, Span<float> outImagAmplitudes)
{
	INTRA_DEBUG_ASSERT(inOutRealAmplitudes.Length() <= outImagAmplitudes.Length());
	Random::FastUniform<uint> rand(uint(inOutRealAmplitudes.Length()^1633529523u));

	enum: uint {TABLE_SIZE = 64, TABLE_MASK = TABLE_SIZE-1,
		INTERPOLATION_BITS = 25, INTERPOLATION_DIVISOR = 1 << INTERPOLATION_BITS, INTERPOLATION_MASK = INTERPOLATION_DIVISOR-1};
	float sineTable[TABLE_SIZE];
	Math::SineRange<float> oscillator(1, 0, float(2*Math::PI/TABLE_SIZE));
	ReadTo(oscillator, sineTable);
	inOutRealAmplitudes.PopFirst();
	outImagAmplitudes.PopFirst();
	while(!inOutRealAmplitudes.Empty())
	{
		const size_t sinPhaseIndex = rand(TABLE_SIZE << INTERPOLATION_BITS);
		const size_t cosPhaseIndex = sinPhaseIndex + (TABLE_SIZE << (INTERPOLATION_BITS-2));
		const float factor = float(cosPhaseIndex & INTERPOLATION_MASK) / float(INTERPOLATION_DIVISOR);
		
		const float cosine1 = sineTable[(cosPhaseIndex >> INTERPOLATION_BITS) & TABLE_MASK];
		const float cosine2 = sineTable[((cosPhaseIndex >> INTERPOLATION_BITS) + 1) & TABLE_MASK];
		const float cosine = Math::LinearMix(cosine1, cosine2, factor);

		const float sine1 = sineTable[(sinPhaseIndex >> INTERPOLATION_BITS) & TABLE_MASK];
		const float sine2 = sineTable[((sinPhaseIndex >> INTERPOLATION_BITS) + 1) & TABLE_MASK];
		const float sine = Math::LinearMix(sine1, sine2, factor);

		float& real = inOutRealAmplitudes.Next();
		float& imag = outImagAmplitudes.Next();
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
	//InplaceInverseFFT(inAmplitudesX2OutSamples, tempBuffer.Take(inAmplitudesX2OutSamples.Length()));
	//Multiply(inAmplitudesX2OutSamples, volume);
}

void ConvertAmplitudesToSamples(WaveTable& table, float volume, bool genMipmaps)
{
	INTRA_DEBUG_ASSERT(table.LevelCount == 1);
	INTRA_DEBUG_ASSERT(table.Data.Length() == table.BaseLevelLength/2); //Data содержит не семплы, а амплитуды частот, которых в 2 раза меньше
	table.Data.SetCount(table.BaseLevelLength * 2); //В первой половине будут размещены семплы, а вторая будет временным буфером для алгоритма
	auto amplitudesThenSamples = table.Data.Take(table.BaseLevelLength); //Половина этой части буфера содержит амлитуды частот, приследующем действии сюда запишутся семплы
	auto tempBuffer = table.Data.Drop(table.BaseLevelLength);
	ConvertAmplutudesToSamples(amplitudesThenSamples, tempBuffer, volume);
	table.Data.SetCount(table.BaseLevelLength); //Временный буфер убираем. Место, оставшееся зарезервированным, будет использоваться для хранения уровней детализации
	if(genMipmaps) table.GenerateAllNextLevels();
	else table.Data.TrimExcessCapacity();
}

WaveTableCache CreateWaveTablesFromHarmonics(CSpan<SineHarmonicWithBandwidthDesc> harmonics,
	float bandwidthScale, size_t tableSize, bool allowMipmaps)
{
	WaveTableCache result;
	Array<SineHarmonicWithBandwidthDesc> harmonicsArr = harmonics;
	result.Generator = [harmonicsArr, bandwidthScale, tableSize, allowMipmaps](float freq, uint sampleRate)
	{
		WaveTable tbl;
		tbl.BaseLevelLength = tableSize;
		tbl.Data.Reserve(tbl.BaseLevelLength * 2);
		tbl.Data.SetCount(tbl.BaseLevelLength / 2);
		tbl.BaseLevelRatio = freq/float(sampleRate);
		for(auto& harm: harmonicsArr)
		{
			//Synth::AddSineHarmonic(tbl.Data, tbl.BaseLevelRatio, harm.FreqMultiplier, harm.Amplitude);
			Synth::AddSineHarmonicGaussianProfile(tbl.Data, tbl.BaseLevelRatio,
				harm.FreqMultiplier, bandwidthScale, harm.Amplitude, harm.Bandwidth);
		}
		ConvertAmplitudesToSamples(tbl, 1, allowMipmaps);
		return tbl;
	};
	result.AllowMipmaps = allowMipmaps;
	return result;
}

Array<SineHarmonicWithBandwidthDesc> CreateHarmonicArray(float bandwidth, float bandwidthStep,
	float harmonicAttenuationStep, float harmonicAttenuationPower, float freqMult, float freqMultStep, size_t numHarmonics, float scale, float alpha, float omega)
{
	Array<SineHarmonicWithBandwidthDesc> harmonics;
	harmonics.Reserve(numHarmonics);
	float harmonicAttenuation = 1;
	while(numHarmonics --> 0)
	{
		auto& harm = harmonics.EmplaceLast();
		harm.Amplitude = scale*Math::Cos(alpha)*Math::Pow(harmonicAttenuation, -harmonicAttenuationPower);
		harm.FreqMultiplier = freqMult;
		harm.Bandwidth = bandwidth;
		freqMult += freqMultStep;
		bandwidth += bandwidthStep;
		alpha += omega;
		harmonicAttenuation += harmonicAttenuationStep;
	}
	return harmonics;
}

Array<SineHarmonicWithBandwidthDesc> CreateUpdownHarmonicArray(float bandwidth, float bandwidthStep,
	float updownRatio, size_t numHarmonics, float scale, float freqMult, float harmonicAttenuationPower)
{
	const float a = float(Math::PI) / (1 + updownRatio);
	const float scaleNormalizeCoeff = scale * 2 / (a*(float(Math::PI) - a));
	return CreateHarmonicArray(bandwidth, bandwidthStep, 1, harmonicAttenuationPower, freqMult, freqMult, numHarmonics, scaleNormalizeCoeff, a - float(Math::PI/2), a);
}


WaveTableCache CreateWaveTablesFromFormants(CSpan<FormantDesc> formants, uint numHarmonics,
	float harmonicAttenuationPower, float bandwidth, float bandwidthScale, size_t tableSize)
{
	WaveTableCache result;
	Array<FormantDesc> formantsArr = formants;
	result.Generator = [=](float freq, uint sampleRate)
	{
		WaveTable tbl;
		tbl.BaseLevelLength = tableSize;
		tbl.Data.Reserve(tbl.BaseLevelLength * 2);
		tbl.Data.SetCount(tbl.BaseLevelLength / 2);
		tbl.BaseLevelRatio = freq/float(sampleRate);
		for(size_t i = 1; i <= numHarmonics; i++)
		{
			const float ifreq = float(i)*freq;
			float amplitude = 0;
			for(auto& formant: formantsArr) amplitude += formant.Scale*Math::Exp(-Math::Sqr((ifreq - formant.Frequency) * formant.Coeff));
			amplitude /= Math::Pow(float(i), harmonicAttenuationPower);
			
			//Synth::AddSineHarmonic(tbl.Data, tbl.BaseLevelRatio, float(i), amplitude);
			Synth::AddSineHarmonicGaussianProfile(tbl.Data, tbl.BaseLevelRatio,
				float(i), bandwidthScale, amplitude, bandwidth);
		}
		ConvertAmplitudesToSamples(tbl, 1, false);
		return tbl;
	};
	result.AllowMipmaps = false;
	return result;
}



}}}

INTRA_WARNING_POP
