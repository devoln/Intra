#pragma once

#include "Cpp/Warnings.h"

#include "Math/FixedPoint.h"
#include "Math/SineRange.h"

#include "Utils/Span.h"

#include "Container/Sequential/Array.h"

#include "Types.h"
#include "Filter.h"
#include "WaveTable.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio { namespace Synth {

//! Подобрать целое количество периодов размером samplesPerPeriod так,
//! чтобы их было не очень много, но конец переходил в начало с минимальным швом.
//! @return Количество повторений периода.
uint GetGoodSignalPeriod(double samplesPerPeriod, uint maxPeriods, double eps = 0.1);

class WaveTableSampler
{
	typedef void(*WaveForm)(const void* params, Span<float> dst, float freq, float volume, uint sampleRate);

	template<typename F> static void WaveFormWrapper(const void* params,
		Span<float> dst, float freq, float volume, uint sampleRate)
	{(*static_cast<const F*>(params))(dst, freq, volume, sampleRate);}

	Array<float> mSampleFragment;
	float mFragmentOffset;
	float mRate;
	float mAttenuation;
	float mAttenuationStep;

	WaveTableSampler(const void* params, WaveForm wave, uint octaves,
		float attenuationPerSample, float volume,
		float freq, uint sampleRate);

public:
	WaveTableSampler(null_t=null) {}

	WaveTableSampler(CSpan<float> periodicWave, float rate, float expCoeff, float volume);

	template<typename F, typename = Meta::EnableIf<
		Meta::IsCallable<F, Span<float>, float, float, uint>::_
	>> forceinline WaveTableSampler(F wave, uint octaves,
		float expCoeff, float volume, float freq, uint sampleRate):
		WaveTableSampler(&wave, WaveFormWrapper<F>, octaves,
			expCoeff, volume,
			freq, sampleRate) {}

	static WaveTableSampler Sine(uint octaves,
		float expCoeff, float volume, float freq, uint sampleRate);

	static WaveTableSampler Sawtooth(uint octaves,
		float updownRatio, float expCoeff, float volume,
		float freq, uint sampleRate);

	static WaveTableSampler Square(uint octaves,
		float updownRatio, float expCoeff, float volume, float freq, uint sampleRate);

	static WaveTableSampler WhiteNoise(uint octaves,
		float expCoeff, float volume, float freq, uint sampleRate);

	static WaveTableSampler WavePeriod(uint octaves,
		CSpan<float> wave, float expCoeff, float volume, float freq, uint sampleRate);

	Span<float> operator()(Span<float> dst, bool add);
	Span<float> operator()(Span<float> dstLeft, Span<float> dstRight, bool add);

	void MultiplyPitch(float freqMultiplier)
	{
		mRate *= freqMultiplier;
		if(Math::Abs(mRate - 1) < 0.0001f) mRate = 1;
	}

private:
	void generateWithDefaultRate(Span<float> dst, bool add);
	void generateWithVaryingRate(Span<float> dst, bool add);
};

enum class WaveType: byte {Sine, Sawtooth, Square, WhiteNoise};

struct WaveInstrument
{
	WaveType Type = WaveType::Sine;
	float Scale = 0;
	float ExpCoeff = 0;
	float FreqMultiplier = 1;
	uint Octaves = 1;
	float RateAcceleration = 0;

	//Отношение времени нарастания к спаду в пилообразной волне.
	//Отношение времени значения 1 ко времени -1 в прямоугольном импульсе.
	float UpdownRatio = 1;

	WaveTableSampler operator()(float freq, float volume, uint sampleRate) const;
};

struct PeriodicInstrument
{
	Array<float> Table;
	float ExpCoeff;
	uint Octaves;

	WaveTableSampler operator()(float freq, float volume, uint sampleRate) const;
};

struct WaveTableCache
{
	typedef Delegate<WaveTable(float freq, uint sampleRate)> GeneratorType;
	mutable Array<WaveTable> Tables;
	GeneratorType Generator;
	float MaxRateDistance = 1;

	WaveTable& Get(float freq, uint sampleRate) const;

	WaveTableCache() {}
	WaveTableCache(const WaveTableCache&) = delete;
	WaveTableCache& operator=(const WaveTableCache&) = delete;
};

struct WaveTableInstrument
{
	WaveTableCache* Tables;
	float ExpCoeff;
	float VolumeScale;

	WaveTableSampler operator()(float freq, float volume, uint sampleRate) const;
};

}}}

INTRA_WARNING_POP
