﻿#pragma once

#include "Cpp/Warnings.h"

#include "Math/SineRange.h"

#include "Utils/Span.h"

#include "Container/Sequential/Array.h"

#include "Types.h"
#include "Filter.h"
#include "WaveTable.h"
#include "ADSR.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio { namespace Synth {

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
	size_t mChannelDeltaSamples;
	float mSmoothingFactor;

	WaveTableSampler(const void* params, WaveForm wave, uint octaves,
		float attenuationPerSample, float volume,
		float freq, uint sampleRate, float vibratoFrequency, float vibratoValue, float smoothingFactor, const AdsrAttenuator& adsr = null);

public:
	WaveTableSampler(null_t=null) {}

	WaveTableSampler(CSpan<float> periodicWave, float rate, float expCoeff,
		float volume, float vibratoDeltaPhase, float vibratoValue, const AdsrAttenuator& adsr, size_t channelDeltaSamples);

	template<typename F, typename = Meta::EnableIf<
		Meta::IsCallable<F, Span<float>, float, float, uint>::_
	>> forceinline WaveTableSampler(const F& wave, uint octaves,
		float expCoeff, float volume, float freq, uint sampleRate,
		float vibratoFrequency, float vibratoValue, float smoothingFactor, const AdsrAttenuator& adsr = null):
		WaveTableSampler(&wave, WaveFormWrapper<F>, octaves,
			expCoeff, volume, freq, sampleRate, vibratoFrequency, vibratoValue, smoothingFactor, adsr) {}

	forceinline bool OwnDataArray() const noexcept {return !mSampleFragmentData.Empty();}

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

	void generateKS(Span<float> dstLeft, Span<float> dstRight, bool add);
	void generateKS(Span<float> dst, bool add) {generateKS(dst, dst, add);}
};

typedef CopyableDelegate<void(Span<float> dst, float freq, float volume, uint sampleRate)> WaveForm;

struct SineWaveForm
{
	void operator()(Span<float> dst, float freq, float volume, uint sampleRate) const;
};

struct SawtoothWaveForm
{
	float UpdownRatio;
	void operator()(Span<float> dst, float freq, float volume, uint sampleRate) const;
};

struct PulseWaveForm
{
	float UpdownRatio;
	void operator()(Span<float> dst, float freq, float volume, uint sampleRate) const;
};

struct WhiteNoiseWaveForm
{
	void operator()(Span<float> dst, float freq, float volume, uint sampleRate) const;
};

struct GuitarWaveForm
{
	float Demp;
	void operator()(Span<float> dst, float freq, float volume, uint sampleRate) const;
};

struct WaveInstrument
{
	WaveForm Wave = SineWaveForm();
	float Scale = 0;
	float ExpCoeff = 0;
	float FreqMultiplier = 1;
	uint Octaves = 1;
	float VibratoFrequency = 0;
	float VibratoValue = 0;
	float SmoothingFactor = 0;
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
	WaveTableCache(WaveTableCache&&) = default;
	WaveTableCache& operator=(WaveTableCache&&) = default;
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
