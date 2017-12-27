#pragma once

#include "Utils/Span.h"

#include "Math/Math.h"

#include "WaveTable.h"
#include "WaveTableSampler.h"

namespace Intra { namespace Audio { namespace Synth {

#if INTRA_DISABLED
inline void AddSineHarmonic(Span<float> wavetableAmplitudes, float freqSampleRateRatio, float amplitude, float bandwidthCents)
{
	const size_t N = wavetableAmplitudes.Length()*2;
	float bwi = (Math::Pow2(bandwidthCents/1200 - 1) - 0.5f)*freqSampleRateRatio;
	float rw = -freqSampleRateRatio, rdw = 1.0f/N;
	while(!wavetableAmplitudes.Empty())
	{
		wavetableAmplitudes.Next() += amplitude * GaussianProfile(rw, bwi);
		rw += rdw;
	}
}
#endif

void AddSineHarmonicGaussianProfile(Span<float> wavetableAmplitudes, float freqSampleRateRatio,
	float harmFreqMultiplier, float harmBandwidthScale, float amplitude, float bandwidthCents);

//! Получает на вход массив inAmplitudesX2OutSamples, первая половина которого содержит амплитуды частот.
//! Присваивает каждой из них случайные фазы и заполняет весь inAmplitudesX2OutSamples семплами.
//! @param tempBuffer - временный буфер размера не меньше inAmplitudesX2OutSamples.Length(), в который будет производиться запись алгоримом.
void ConvertAmplutudesToSamples(Span<float> inAmplitudesX2OutSamples, Span<float> tempBuffer, float volume=1);

//! Принимает table, у которого Data содержит table.BaseLevelLength / 2 частот.
//! После работы этой функции table содержит table.BaseLevelLength семплов, соответствующих этим частотам со случайными фазами.
//! Кроме того генерирует все уровни детализации для полученного сигнала.
void ConvertAmplitudesToSamples(WaveTable& table, float volume=1, bool genMipmaps=false);


struct SineHarmonicWithBandwidthDesc
{
	float Amplitude;
	float FreqMultiplier;
	float Bandwidth;
};

WaveTableCache CreateWaveTablesFromHarmonics(CSpan<SineHarmonicWithBandwidthDesc> harmonics,
	float bandwidthScale, size_t tableSize, bool allowMipmaps);

//Первые numHarmonics слагаемых ряда гармоник с коэффициентами n^(-numHarmonics) и частотами (1+(n-1))*pi
Array<SineHarmonicWithBandwidthDesc> CreateHarmonicArray(float bandwidth, float bandwidthStep,
	float harmonicAttenuationPower, float freqMultStep, size_t numHarmonics, bool alternatingSigns);

//Гармоники обобщённой пилообразной волны c указанным соотношением времени нарастания к времени спада updownRatio
Array<SineHarmonicWithBandwidthDesc> CreateUpdownHarmonicArray(float bandwidth, float bandwidthStep, float updownRatio, size_t numHarmonics);

struct FormantDesc
{
	float Frequency;
	float Coeff;
	float Scale;
};

WaveTableCache CreateWaveTablesFromFormants(CSpan<FormantDesc> formants, uint numHarmonics,
	float harmonicAttenuationPower, float bandwidth, float bandwidthScale, size_t tableSize);


}}}
