#pragma once

#include "Utils/Span.h"

#include "Math/Math.h"

#include "WaveTable.h"
#include "WaveTableSampler.h"

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

//Первые numHarmonics слагаемых ряда гармоник с коэффициентами sin(alpha*n) / n^harmonicAttenuationPower и частотами freqMult+freqMultStep*(n-1)
Array<SineHarmonicWithBandwidthDesc> CreateHarmonicArray(float bandwidth, float bandwidthStep,
	float harmonicAttenuationStep, float harmonicAttenuationPower,
	float freqMult, float freqMultStep, size_t numHarmonics, float scale, float alpha=0, float omega=0);

//Гармоники обобщённой пилообразной волны c указанным соотношением времени нарастания к времени спада updownRatio
Array<SineHarmonicWithBandwidthDesc> CreateUpdownHarmonicArray(float bandwidth, float bandwidthStep,
	float updownRatio, size_t numHarmonics=16, float scale=1, float freqMult=1, float harmonicAttenuationPower=2);

struct FormantDesc
{
	float Frequency;
	float Coeff;
	float Scale;
};

WaveTableCache CreateWaveTablesFromFormants(CSpan<FormantDesc> formants, uint numHarmonics,
	float harmonicAttenuationPower, float bandwidth, float bandwidthScale, size_t tableSize);

