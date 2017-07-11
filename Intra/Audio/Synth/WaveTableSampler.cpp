#include "WaveTableSampler.h"
#include "ExponentialAttenuation.h"

#include "Generators/Sawtooth.h"
#include "Generators/Square.h"
#include "Generators/Pulse.h"
#include "Generators/WhiteNoise.h"

#include "Audio/AudioBuffer.h"
#include "Audio/Resample.h"
#include "Octaves.h"

#include "Utils/Span.h"

#include "Funal/Bind.h"

#include "Math/Math.h"

#include "Simd/Simd.h"

#include "Range/Mutation/Copy.h"
#include "Range/Mutation/Fill.h"
#include "Range/Mutation/Transform.h"

#include "Container/Sequential/Array.h"

#include "Random/FastUniform.h"


namespace Intra { namespace Audio { namespace Synth {

uint GetGoodSignalPeriod(double samplesPerPeriod, uint maxPeriods)
{
	if(Math::Fract(samplesPerPeriod) <= 0.05) return 1;
	const double fractionalCount = 1.0 / (Math::Floor(Math::Fract(samplesPerPeriod)*20)/20);
	double minDeltaCnt = 1;
	uint minDeltaN = 0;
	for(uint n = 1; fractionalCount*n < maxPeriods || minDeltaCnt > 0.1; n++)
	{
		double delta = Math::Fract(fractionalCount*n);
		if(delta > 0.5) delta = 1.0 - delta;
		delta /= n;
		if(minDeltaCnt > delta)
		{
			minDeltaCnt = delta;
			minDeltaN = n;
		}
	}
	return uint(Math::Round(fractionalCount*minDeltaN));
}

WaveTableSampler::WaveTableSampler(const void* params, WaveForm wave, uint octaves, float rateAcceleration,
	float expCoeff, float volume, float freq, uint sampleRate, size_t sampleCount, size_t delaySamples):
	mDelaySamples(delaySamples), mSamplesLeft(sampleCount),
	mRateAcceleration(rateAcceleration), mAttenuation(1), mAttenuationStep(1), mFragmentOffset(0)
{
	const float samplesPerPeriod = float(sampleRate)/freq;
	const uint goodPeriod = GetGoodSignalPeriod(samplesPerPeriod, Math::Max(uint(freq/50), 5u));
	const uint goodSignalPeriodSamples = uint(Math::Round(samplesPerPeriod*goodPeriod));
	mSampleFragment.SetCountUninitialized(goodSignalPeriodSamples);

	volume *= 2.0f - 2.0f/float(1 << octaves);
	Array<float> buffer;
	buffer.SetCountUninitialized(goodSignalPeriodSamples);
	wave(params, octaves == 2? buffer: mSampleFragment, freq, volume, sampleRate);
	if(octaves > 1)
	{
		Span<float> src = octaves == 2? buffer: mSampleFragment;
		GenOctaves(src, octaves != 2? buffer: mSampleFragment, octaves);
	}

	if(expCoeff <= 0.001f) return;

	const float ek = Math::Exp(-expCoeff/float(sampleRate));
	const float duration = Math::Log(0.001f / volume) / (-expCoeff);
	const size_t attenuationSamples = size_t(duration*sampleRate);
	if(mSamplesLeft > attenuationSamples) mSamplesLeft = attenuationSamples;
	Span<float> dst = mSampleFragment;
	ExponentialAttenuate(dst, mSampleFragment, mAttenuationStep, ek);
}

void WaveTableSampler::generateWithDefaultRate(Span<float> dst, bool add)
{
	while(!dst.Empty())
	{
		auto fragment = mSampleFragment.Drop(size_t(mFragmentOffset));
		size_t samplesProcessed;
		if(!add) samplesProcessed = Multiply(dst, fragment, mAttenuation);
		else samplesProcessed = AddMultiplied(dst, fragment, mAttenuation);
		dst.PopFirstExactly(samplesProcessed);
		mFragmentOffset += samplesProcessed;
		if(mFragmentOffset >= mSampleFragment.Length())
		{
			mAttenuation *= mAttenuationStep;
			mFragmentOffset = 0;
		}
	}
}

void WaveTableSampler::generateWithVaryingRate(Span<float> dst, bool add)
{
	while(!dst.Empty())
	{
		const size_t i = size_t(mFragmentOffset);
		const float factor = (mFragmentOffset - i)*mAttenuation;
		size_t j = i+1;
		mFragmentOffset += mRate;
		if(j == mSampleFragment.Length())
		{
			mFragmentOffset -= j;
			mAttenuation *= Math::Pow(mAttenuationStep, mRate);
			j = 0;
		}
		const float sample = mSampleFragment[i]*(mAttenuation - factor) + mSampleFragment[j]*factor;
		if(!add) *dst.Begin++ = sample;
		else *dst.Begin++ += sample;
	}
	mRate += mRateAcceleration*dst.Length();
}

Span<float> WaveTableSampler::operator()(Span<float> dst, bool add)
{
	const auto notProcessedPart = dst.Drop(mSamplesLeft);
	dst = dst.Take(mSamplesLeft);
	mSamplesLeft -= dst.Length();
	mDelaySamples -= dst.PopFirstN(mDelaySamples);
	if(mRate == 1 && mRateAcceleration == 0) generateWithDefaultRate(dst, add);
	else while(!dst.Full()) generateWithVaryingRate(dst.TakeAdvance(1024), add);
	return notProcessedPart;
}

WaveTableSampler WaveTableSampler::Sine(uint octaves, float rateAcceleration,
	float expCoeff, float volume, float freq,
	uint sampleRate, size_t sampleCount, size_t delaySamples)
{
	return WaveTableSampler(
		[](Span<float> dst, float freq, float volume, uint sampleRate)
	{
		Math::SineRange<float> sine(volume, 0, float(2*Math::PI*freq/sampleRate));
		ReadTo(sine, dst);
	}, octaves, rateAcceleration, expCoeff, volume, freq, sampleRate, sampleCount, delaySamples);
}

WaveTableSampler WaveTableSampler::Sawtooth(uint octaves, float rateAcceleration, float updownRatio, float expCoeff, float volume,
	float freq, uint sampleRate, size_t sampleCount, size_t delaySamples)
{
	return WaveTableSampler(
		[updownRatio](Span<float> dst, float freq, float volume, uint sampleRate)
	{
		Generators::Sawtooth saw(updownRatio, freq, volume, sampleRate);
		ReadTo(saw, dst);
	}, octaves, rateAcceleration, expCoeff, volume, freq, sampleRate, sampleCount, delaySamples);
}

WaveTableSampler WaveTableSampler::Square(uint octaves, float rateAcceleration, float updownRatio, float expCoeff, float volume,
	float freq, uint sampleRate, size_t sampleCount, size_t delaySamples)
{
	return WaveTableSampler(
		[updownRatio](Span<float> dst, float freq, float volume, uint sampleRate)
	{
		if(updownRatio == 1)
		{
			Generators::Square sqr(freq, sampleRate);
			ReadTo(sqr, dst);
		}
		else
		{
			Generators::Pulse rect(updownRatio, freq, sampleRate);
			ReadTo(rect, dst);
		}
		Multiply(dst, volume);
	}, octaves, rateAcceleration, expCoeff, volume, freq, sampleRate, sampleCount, delaySamples);
}

WaveTableSampler WaveTableSampler::WhiteNoise(uint octaves, float rateAcceleration,
	float expCoeff, float volume, float freq,
	uint sampleRate, size_t sampleCount, size_t delaySamples)
{
	return WaveTableSampler(
		[](Span<float> dst, float freq, float volume, uint sampleRate)
	{
		uint samplesPerPeriod = uint(Math::Round(float(sampleRate)/freq));
		if(samplesPerPeriod == 0) samplesPerPeriod = 1;
		Random::FastUniform<float> noise;
		auto samplePeriod = dst.Take(samplesPerPeriod);
		for(size_t i = 0; i < samplesPerPeriod; i++) dst.Put(noise.SignedNext()*volume);
		while(!dst.Full()) WriteTo(samplePeriod, dst);
	}, octaves, rateAcceleration, expCoeff, volume, freq, sampleRate, sampleCount, delaySamples);
}

WaveTableSampler WaveTableSampler::WaveTable(uint octaves, float rateAcceleration,
	CSpan<float> table, float expCoeff, float volume,
	float freq, uint sampleRate, size_t sampleCount, size_t delaySamples)
{
	return WaveTableSampler(
		[table](Span<float> dst, float freq, float volume, uint sampleRate)
	{
		uint samplesPerPeriod = uint(Math::Round(float(sampleRate)/freq));
		if(samplesPerPeriod == 0) samplesPerPeriod = 1;
		Random::FastUniform<float> noise;
		auto samplePeriod = dst.Take(samplesPerPeriod);
		ResampleLinear(table, dst);
		dst.PopFirstN(samplesPerPeriod);
		while(!dst.Full()) WriteTo(samplePeriod, dst);
	}, octaves, rateAcceleration, expCoeff, volume, freq, sampleRate, sampleCount, delaySamples);
}

WaveTableSampler WaveInstrument::operator()(float freq, float volume, uint sampleRate, size_t sampleCount) const
{
	Random::FastUniform<ushort> rand(3543212397u ^ (uint(Type) << 25) ^ uint(volume*1024) ^ (uint(freq) << 15) ^ uint(sampleCount));
	const size_t delay = rand(ushort(sampleRate >> 10));
	switch(Type)
	{
	case WaveType::Sine:
		return WaveTableSampler::Sine(Octaves, RateAcceleration, ExpCoeff, volume*Scale,
			freq*FreqMultiplyer, sampleRate, sampleCount+delay, delay);

	case WaveType::Sawtooth:
		return WaveTableSampler::Sawtooth(Octaves, RateAcceleration, UpdownRatio, ExpCoeff, volume*Scale,
			freq*FreqMultiplyer, sampleRate, sampleCount+delay, delay);

	case WaveType::Square:
		return WaveTableSampler::Square(Octaves, RateAcceleration, UpdownRatio, ExpCoeff, volume*Scale,
			freq*FreqMultiplyer, sampleRate, sampleCount+delay, delay);

	case WaveType::WhiteNoise:
		return WaveTableSampler::WhiteNoise(Octaves, RateAcceleration, ExpCoeff, volume*Scale,
			freq*FreqMultiplyer, sampleRate, sampleCount+delay, delay);
	}
}

WaveTableSampler WaveTableInstrument::operator()(float freq, float volume, uint sampleRate, size_t sampleCount) const
{
	Random::FastUniform<ushort> rand(3298519863u ^ uint(volume*100) ^ (uint(freq) << 15) ^ uint(sampleCount));
	const size_t delay = rand(ushort(sampleRate >> 10));
	return WaveTableSampler::WaveTable(Octaves, RateAcceleration, Table, ExpCoeff,
		volume, freq, sampleRate, sampleCount+delay, delay);
}

}}}
