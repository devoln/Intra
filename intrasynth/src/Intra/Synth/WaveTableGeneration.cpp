#include "WaveTableGeneration.h"

#include <Audio/AudioProcessing.h>

#include <Random/FastUniform.h>

#include <Math/SineRange.h>

#include <Range/Mutation/Copy.h>
#include <Range/Mutation/Transform.h>
#include <Range/Reduction.h>

using namespace Audio;

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

void AddSineHarmonic(Span<float> wavetableAmplitudes, float freqSampleRateRatio, float harmFreqMultiplier, float amplitude)
{
	const size_t index = size_t(Round(freqSampleRateRatio*harmFreqMultiplier*float(2*wavetableAmplitudes.Length())));
	if(index >= wavetableAmplitudes.Length()) return;
	wavetableAmplitudes[index] += amplitude;
}

void AddSineHarmonicGaussianProfile(Span<float> wavetableAmplitudes, float freqSampleRateRatio,
	float harmFreqMultiplier, float harmBandwidthScale, float amplitude, float bandwidthCents)
{
	const size_t N = wavetableAmplitudes.Length()*2;

	//Это обратный нормирующий коэффициент sigma*sqrt(2*pi) в формуле нормального распределения.
	//Пропорционален ширине гармоники - корня дисперсии sigma нормального распределения.
	double bwi = (Pow2(bandwidthCents/1200 - 1) - 0.5)*freqSampleRateRatio;

	//Ширина гармоники растёт с её номером в ряду
	bwi *= Pow(double(harmFreqMultiplier), double(harmBandwidthScale));

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
		const double elementsToSkip = Floor((-range - rw) / rdw);
		wavetableAmplitudes.PopFirstN(size_t(elementsToSkip));
		rw += elementsToSkip*rdw;
	}
	if(rw < range) wavetableAmplitudes = wavetableAmplitudes.Take(size_t(Ceil((range - rw) / rdw)));

	//TODO: Для широких гауссиан можно использовать приближение exp(-x^2) сплайном
	//{1 - 1.5*(x*0.859741)^2 + 0.75*|x*0.859741|^3, |x*0.859741|<=1}
	//{0.5 - 0.25*|x*0.859741|^3, 1<|x*0.859741|<=2}
	const double A = amplitude/bwi*(SqrtPI/2);
	double erf = Erf(rw);
	while(!wavetableAmplitudes.Empty())
	{
		rw += rdw;
		const double erfNext = Erf(rw);
		wavetableAmplitudes.Next() += float(A * (erfNext - erf));
		erf = erfNext;
	}
}


static void GenerateRandomPhases(Span<float> inOutRealAmplitudes, Span<float> outImagAmplitudes, unsigned phaseSalt = 0)
{
	INTRA_DEBUG_ASSERT(inOutRealAmplitudes.Length() <= outImagAmplitudes.Length());
	// phaseSalt 0 keeps the canonical per-table phase fingerprint (length ^ 1633529523);
	// a non-zero salt selects a different phase realization for the same amplitudes,
	// which changes the envelope beating of near-degenerate lines (SynthVoice 54).
	Random::FastUniform<unsigned> rand(unsigned(inOutRealAmplitudes.Length()^1633529523u)^phaseSalt);

	enum: unsigned {TABLE_SIZE = 64, TABLE_MASK = TABLE_SIZE-1,
		INTERPOLATION_BITS = 25, INTERPOLATION_DIVISOR = 1 << INTERPOLATION_BITS, INTERPOLATION_MASK = INTERPOLATION_DIVISOR-1};
	float sineTable[TABLE_SIZE];
	SineRange<float> oscillator(1, 0, float(2*PI/TABLE_SIZE));
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
		const float cosine = LinearMix(cosine1, cosine2, factor);

		const float sine1 = sineTable[(sinPhaseIndex >> INTERPOLATION_BITS) & TABLE_MASK];
		const float sine2 = sineTable[((sinPhaseIndex >> INTERPOLATION_BITS) + 1) & TABLE_MASK];
		const float sine = LinearMix(sine1, sine2, factor);

		float& real = inOutRealAmplitudes.Next();
		float& imag = outImagAmplitudes.Next();
		imag = real*sine;
		real *= cosine;
	}
}

// Фаза, которую GenerateRandomPhases выдаёт БИНУ bin для таблицы tableSize
// с phaseSalt = 0: воспроизводим ТОТ ЖЕ поток rand() — каждый вызов берёт
// TABLE_SIZE << INTERPOLATION_BITS значений, как в теле GenerateRandomPhases.
// Возвращает фазу в радианах (аргумент синуса линии).
// Бин 0 (DC) вызов rand() не делает, бину 1 достаётся ПЕРВЫЙ вызов — поэтому
// для бина bin пропускаем bin-1 значений, а не bin.
float SustainPhaseOfBin(size_t bin, size_t tableSize)
{
	enum: unsigned {TABLE_SIZE = 64, INTERPOLATION_BITS = 25, INTERPOLATION_DIVISOR = 1 << INTERPOLATION_BITS};
	Random::FastUniform<unsigned> rand(unsigned(tableSize/2 ^ 1633529523u));
	for(size_t i = 0; i + 1 < bin; i++) (void)rand(TABLE_SIZE << INTERPOLATION_BITS);
	const size_t sinPhaseIndex = rand(TABLE_SIZE << INTERPOLATION_BITS);
	const size_t cosPhaseIndex = sinPhaseIndex + (TABLE_SIZE << (INTERPOLATION_BITS - 2));
	const float factor = float(cosPhaseIndex & (INTERPOLATION_DIVISOR - 1))/float(INTERPOLATION_DIVISOR);
	const size_t ci = (cosPhaseIndex >> INTERPOLATION_BITS) & (TABLE_SIZE - 1);
	const size_t si = (sinPhaseIndex >> INTERPOLATION_BITS) & (TABLE_SIZE - 1);
	// cos φ = interpolate(sineTable, cosPhaseIndex), sin φ = interpolate(..., sinPhaseIndex);
	// фаза в радианах: atan2(sin, cos).
	SineRange<float> osc(1.0f, 0.0f, float(2*PI/TABLE_SIZE));
	float sineTable[TABLE_SIZE];
	ReadTo(osc, sineTable);
	const float cosine = sineTable[ci] + (sineTable[(ci + 1) & (TABLE_SIZE - 1)] - sineTable[ci])*factor;
	const float sine = sineTable[si] + (sineTable[(si + 1) & (TABLE_SIZE - 1)] - sineTable[si])*factor;
	return sine >= 0.0f
		? (cosine >= 0.0f ? Math::Atan(sine/cosine) : float(PI) + Math::Atan(sine/cosine))
		: (cosine >= 0.0f ? Math::Atan(sine/cosine) : -float(PI) + Math::Atan(sine/cosine));
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
	ConvertAmplitudesToSamplesUnnormalized(inAmplitudesX2OutSamples, tempBuffer);
	NormalizeSamples(inAmplitudesX2OutSamples, volume);
}

void ConvertAmplitudesToSamplesUnnormalized(Span<float> inAmplitudesX2OutSamples, Span<float> tempBuffer, unsigned phaseSalt)
{
	INTRA_DEBUG_ASSERT(tempBuffer.Length() >= inAmplitudesX2OutSamples.Length());
	INTRA_DEBUG_ASSERT(inAmplitudesX2OutSamples.Length() % 2 == 0);
	GenerateRandomPhases(
		inAmplitudesX2OutSamples.Take(inAmplitudesX2OutSamples.Length() / 2),
		tempBuffer.Take(inAmplitudesX2OutSamples.Length() / 2), phaseSalt);
	ReflectComplexAmplitudes(inAmplitudesX2OutSamples, tempBuffer.Take(inAmplitudesX2OutSamples.Length()));
	InplaceInverseFFTNonNormalized(inAmplitudesX2OutSamples, tempBuffer.Take(inAmplitudesX2OutSamples.Length()));
}

void ConvertPhasedHarmonicsToSamples(Span<float> inAmplitudesX2OutSamples, Span<float> tempBuffer,
	float baseRatio, Span<const float> amplitudes)
{
	INTRA_DEBUG_ASSERT(tempBuffer.Length() >= inAmplitudesX2OutSamples.Length());
	INTRA_DEBUG_ASSERT(inAmplitudesX2OutSamples.Length() % 2 == 0);
	const size_t length = inAmplitudesX2OutSamples.Length();
	const size_t halfBins = length/2;
	// Нормировочный делитель — голая Σa амплитуд линий, РОВНО как эффективный
	// коэффициент BuildWaveTableCore: там Amplitude*volumeScale/(Σa·N) уходит в
	// AddSineHarmonic, который умножает обратно на N, то есть бин = a/Σa.
	// Раньше здесь стоял дополнительный /N (amplSum *= length) без обратного ×N —
	// таблица выходила на −84 дБ тише тела и онсета не было слышно вообще.
	float amplSum = 0;
	for(const auto& a: amplitudes) amplSum += a;
	if(Math::Abs(amplSum) < 0.000001f) return;
	for(size_t k = 0; k < amplitudes.Length(); k++)
	{
		const size_t bin = size_t(Round(baseRatio*float(k + 1)*float(2*halfBins)));
		if(bin >= halfBins) break;
		const float a = amplitudes[k]/amplSum;
		// Фаза линии — та же, что у сустейн-таблицы на ЭТОМ бине (у "Harms16"
		// ширина гармоники нулевая, поэтому линия занимает ровно один бин).
		const float phase = SustainPhaseOfBin(bin, length);
		// Соглашение как в GenerateRandomPhases: X = A·(cos φ + i·sin φ).
		// Другой знак мнимой части дал бы зеркальную фазу, и кроссфейд с телом
		// гасил бы гармоники — та же болезнь, что у блума (Update 223).
		inAmplitudesX2OutSamples[bin] += a*Math::Cos(phase);
		tempBuffer[bin] += a*Math::Sin(phase);
	}
	ReflectComplexAmplitudes(inAmplitudesX2OutSamples, tempBuffer.Take(length));
	InplaceInverseFFTNonNormalized(inAmplitudesX2OutSamples, tempBuffer.Take(length));
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

void ConvertAmplitudesToSamplesUnnormalized(WaveTable& table, bool genMipmaps, unsigned phaseSalt)
{
	INTRA_DEBUG_ASSERT(table.LevelCount == 1);
	INTRA_DEBUG_ASSERT(table.Data.Length() == table.BaseLevelLength/2);
	table.Data.SetCount(table.BaseLevelLength * 2);
	auto amplitudesThenSamples = table.Data.Take(table.BaseLevelLength);
	auto tempBuffer = table.Data.Drop(table.BaseLevelLength);
	ConvertAmplitudesToSamplesUnnormalized(amplitudesThenSamples, tempBuffer, phaseSalt);
	table.Data.SetCount(table.BaseLevelLength);
	if(genMipmaps) table.GenerateAllNextLevels();
	else table.Data.TrimExcessCapacity();
}

WaveTableCache CreateWaveTablesFromHarmonics(Span<const SineHarmonicWithBandwidthDesc> harmonics,
	float bandwidthScale, size_t tableSize, bool allowMipmaps)
{
	WaveTableCache result;
	Array<SineHarmonicWithBandwidthDesc> harmonicsArr = harmonics;
	result.Generator = [harmonicsArr, bandwidthScale, tableSize, allowMipmaps](float freq, unsigned sampleRate)
	{
		WaveTable tbl;
		tbl.BaseLevelLength = tableSize;
		tbl.Data.Reserve(tbl.BaseLevelLength * 2);
		tbl.Data.SetCount(tbl.BaseLevelLength / 2);
		tbl.BaseLevelRatio = freq/float(sampleRate);
		for(auto& harm: harmonicsArr)
		{
			//AddSineHarmonic(tbl.Data, tbl.BaseLevelRatio, harm.FreqMultiplier, harm.Amplitude);
			AddSineHarmonicGaussianProfile(tbl.Data, tbl.BaseLevelRatio,
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
		harm.Amplitude = scale*Cos(alpha)*Pow(harmonicAttenuation, -harmonicAttenuationPower);
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
	const float a = float(PI) / (1 + updownRatio);
	const float scaleNormalizeCoeff = scale * 2 / (a*(float(PI) - a));
	return CreateHarmonicArray(bandwidth, bandwidthStep, 1, harmonicAttenuationPower, freqMult, freqMult, numHarmonics, scaleNormalizeCoeff, a - float(PI/2), a);
}


WaveTableCache CreateWaveTablesFromFormants(Span<const FormantDesc> formants, unsigned numHarmonics,
	float harmonicAttenuationPower, float bandwidth, float bandwidthScale, size_t tableSize)
{
	WaveTableCache result;
	Array<FormantDesc> formantsArr = formants;
	result.Generator = [=](float freq, unsigned sampleRate)
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
			for(auto& formant: formantsArr) amplitude += formant.Scale*Exp(-Sqr((ifreq - formant.Frequency) * formant.Coeff));
			amplitude /= Pow(float(i), harmonicAttenuationPower);
			
			//AddSineHarmonic(tbl.Data, tbl.BaseLevelRatio, float(i), amplitude);
			AddSineHarmonicGaussianProfile(tbl.Data, tbl.BaseLevelRatio,
				float(i), bandwidthScale, amplitude, bandwidth);
		}
		ConvertAmplitudesToSamples(tbl, 1, false);
		return tbl;
	};
	result.AllowMipmaps = false;
	return result;
}


INTRA_WARNING_POP
