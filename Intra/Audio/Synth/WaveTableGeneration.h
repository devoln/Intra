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

//! �������� �� ���� ������ inAmplitudesX2OutSamples, ������ �������� �������� �������� ��������� ������.
//! ����������� ������ �� ��� ��������� ���� � ��������� ���� inAmplitudesX2OutSamples ��������.
//! @param tempBuffer - ��������� ����� ������� �� ������ inAmplitudesX2OutSamples.Length(), � ������� ����� ������������� ������ ���������.
void ConvertAmplutudesToSamples(Span<float> inAmplitudesX2OutSamples, Span<float> tempBuffer, float volume=1);

//! ��������� table, � �������� Data �������� table.BaseLevelLength / 2 ������.
//! ����� ������ ���� ������� table �������� table.BaseLevelLength �������, ��������������� ���� �������� �� ���������� ������.
//! ����� ���� ���������� ��� ������ ����������� ��� ����������� �������.
void ConvertAmplitudesToSamples(WaveTable& table, float volume=1);

}}}
