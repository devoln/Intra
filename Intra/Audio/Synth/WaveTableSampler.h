#pragma once

#include "Cpp/Warnings.h"

#include "Math/FixedPoint.h"
#include "Math/SineRange.h"

#include "Utils/Span.h"

#include "Container/Sequential/Array.h"

#include "Types.h"
#include "Filter.h"
#include "WaveTable.h"
#include "ADSR.h"

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

	Array<float> mSampleFragmentData;
	CSpan<float> mSampleFragment;
	float mFragmentOffset;
	float mRate;
	float mAttenuation;
	float mAttenuationStep;
	float mRightPanMultiplier;
	Math::SineRange<float> mFreqOscillator;
	AdsrAttenuator mADSR;
	float mChannelDeltaSamples;

	WaveTableSampler(const void* params, WaveForm wave, uint octaves,
		float attenuationPerSample, float volume,
		float freq, uint sampleRate, float vibratoFrequency, float vibratoValue, const AdsrAttenuator& adsr = null);

public:
	WaveTableSampler(null_t=null) {}

	WaveTableSampler(CSpan<float> periodicWave, float rate, float expCoeff,
		float volume, float vibratoDeltaPhase, float vibratoValue, const AdsrAttenuator& adsr, float channelDeltaSamples);

	template<typename F, typename = Meta::EnableIf<
		Meta::IsCallable<F, Span<float>, float, float, uint>::_
	>> forceinline WaveTableSampler(F wave, uint octaves,
		float expCoeff, float volume, float freq, uint sampleRate, float vibratoFrequency, float vibratoValue, const AdsrAttenuator& adsr = null):
		WaveTableSampler(&wave, WaveFormWrapper<F>, octaves,
			expCoeff, volume, freq, sampleRate, vibratoFrequency, vibratoValue, adsr) {}

	forceinline bool OwnDataArray() const noexcept {return !mSampleFragmentData.Empty();}

	static WaveTableSampler Sine(uint octaves,
		float expCoeff, float volume, float freq, uint sampleRate,
		float vibratoFrequency, float vibratoValue, const AdsrAttenuator& adsr = null);

	static WaveTableSampler Sawtooth(uint octaves,
		float updownRatio, float expCoeff, float volume,
		float freq, uint sampleRate,
		float vibratoFrequency, float vibratoValue, const AdsrAttenuator& adsr = null);

	static WaveTableSampler Square(uint octaves,
		float updownRatio, float expCoeff, float volume, float freq, uint sampleRate,
		float vibratoFrequency, float vibratoValue, const AdsrAttenuator& adsr = null);

	static WaveTableSampler WhiteNoise(uint octaves,
		float expCoeff, float volume, float freq, uint sampleRate,
		float vibratoFrequency, float vibratoValue, const AdsrAttenuator& adsr = null);

	static WaveTableSampler WavePeriod(uint octaves, CSpan<float> wave, float expCoeff, float volume,
		float freq, uint sampleRate,
		float vibratoFrequency, float vibratoValue, const AdsrAttenuator& adsr = null);

	Span<float> operator()(Span<float> dst, bool add);
	size_t operator()(Span<float> dstLeft, Span<float> dstRight, bool add);

	void MultiplyPitch(float freqMultiplier)
	{
		mRate *= freqMultiplier;
		if(Math::Abs(mRate - 1) < 0.0001f) mRate = 1;
	}

	void SetPan(float newPan)
	{
		mRightPanMultiplier = (newPan + 1) * 0.5f;
	}

	void NoteRelease()
	{
		mADSR.NoteRelease();
	}

private:
	void generateWithDefaultRate(Span<float> dst, bool add,
		float& fragmentOffset, float& attenuation, AdsrAttenuator& adsr);

	void generateWithVaryingRate(Span<float> dst, bool add,
		float& fragmentOffset, float& attenuation, Math::SineRange<float>& freqOscillator, AdsrAttenuator& adsr);
};

enum class WaveType: byte {Sine, Sawtooth, Square, WhiteNoise};

struct WaveInstrument
{
	WaveType Type = WaveType::Sine;
	float Scale = 0;
	float ExpCoeff = 0;
	float FreqMultiplier = 1;
	uint Octaves = 1;
	float VibratoFrequency = 0;
	float VibratoValue = 0;
	AdsrAttenuatorFactory ADSR;

	//Отношение времени нарастания к спаду в пилообразной волне.
	//Отношение времени значения 1 ко времени -1 в прямоугольном импульсе.
	float UpdownRatio = 1;

	WaveTableSampler operator()(float freq, float volume, uint sampleRate) const;
};

struct PeriodicInstrument
{
	Array<float> Table;
	float ExpCoeff = 0;
	uint Octaves = 1;
	float VibratoFrequency = 0;
	float VibratoValue = 0;
	AdsrAttenuatorFactory ADSR;

	WaveTableSampler operator()(float freq, float volume, uint sampleRate) const;
};

struct WaveTableCache
{
	typedef Delegate<WaveTable(float freq, uint sampleRate)> GeneratorType;
	mutable Array<WaveTable> Tables;
	GeneratorType Generator;
	bool AllowMipmaps = false;

	WaveTable& Get(float freq, uint sampleRate) const;

	WaveTableCache() {}
	WaveTableCache(const WaveTableCache&) = delete;
	WaveTableCache& operator=(const WaveTableCache&) = delete;
};

struct WaveTableInstrument
{
	WaveTableCache* Tables = null;
	float ExpCoeff = 0;
	float VolumeScale = 0;
	float VibratoFrequency = 0;
	float VibratoValue = 0;
	AdsrAttenuatorFactory ADSR;

	WaveTableSampler operator()(float freq, float volume, uint sampleRate) const;
};

}}}

INTRA_WARNING_POP
