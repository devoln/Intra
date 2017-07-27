#pragma once

#include "Utils/Span.h"

#include "Math/Math.h"

#include "WaveTable.h"

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

//! ѕолучает на вход массив inAmplitudesX2OutSamples, перва€ половина которого содержит амплитуды частот.
//! ѕрисваивает каждой из них случайные фазы и заполн€ет весь inAmplitudesX2OutSamples семплами.
//! @param tempBuffer - временный буфер размера не меньше inAmplitudesX2OutSamples.Length(), в который будет производитьс€ запись алгоримом.
void ConvertAmplutudesToSamples(Span<float> inAmplitudesX2OutSamples, Span<float> tempBuffer, float volume=1);

//! ѕринимает table, у которого Data содержит table.BaseLevelLength / 2 частот.
//! ѕосле работы этой функции table содержит table.BaseLevelLength семплов, соответствующих этим частотам со случайными фазами.
//!  роме того генерирует все уровни детализации дл€ полученного сигнала.
void ConvertAmplitudesToSamples(WaveTable& table, float volume=1);

}}}
