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

uint GetGoodSignalPeriod(double samplesPerPeriod, uint maxPeriods, double eps)
{
	if(Math::Fract(samplesPerPeriod) <= eps/2) return 1;
	const double fractionalCount = 1.0 / (Math::Floor(Math::Fract(samplesPerPeriod)*20)/20);
	double minDeltaCnt = 1;
	uint minDeltaN = 0;
	for(uint n = 1; fractionalCount*n < maxPeriods || minDeltaCnt > eps; n++)
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
	float expCoeff, float volume, float freq, uint sampleRate):
	mRate(1), mRateAcceleration(rateAcceleration), mAttenuation(1), mAttenuationStep(1)
{
	INTRA_DEBUG_ASSERT(octaves >= 1);
	const float samplesPerPeriod = float(sampleRate)/freq;
	const uint goodPeriod = GetGoodSignalPeriod(samplesPerPeriod*(1 << (octaves - 1)), uint(Math::Round(1000/samplesPerPeriod))+1, 0.2f);
	const uint goodSignalPeriodSamples = uint(Math::Round(samplesPerPeriod*(1 << (octaves - 1))*goodPeriod));
	freq = sampleRate*(1 << (octaves - 1))*goodPeriod / float(goodSignalPeriodSamples);
	mSampleFragment.SetCountUninitialized(goodSignalPeriodSamples);
#ifdef INTRA_DEBUG
	Range::Fill(mSampleFragment, 1000000);
#endif

	if(octaves > 1)
	{
		volume *= 2.0f - 2.0f/float(1 << octaves);
		Array<float> buffer;
		buffer.SetCountUninitialized(goodSignalPeriodSamples);
#ifdef INTRA_DEBUG
		Range::Fill(buffer, 1000000);
#endif
		wave(params, octaves == 2? buffer: mSampleFragment, freq, volume, sampleRate);
		Span<float> src = octaves == 2? buffer: mSampleFragment;
		GenOctaves(src, octaves > 2? buffer: mSampleFragment, octaves, 20);
	}
	else wave(params, mSampleFragment, freq, volume, sampleRate);

	Random::FastUniform<uint> rand(1436491347u ^ mSampleFragment.Length() ^ uint(freq*1000) ^ (octaves << 15) ^ uint(size_t(this) >> 3));
	mFragmentOffset = float(rand(mSampleFragment.Length()));

	if(expCoeff <= 0.001f) return;

	const float ek = Math::Exp(-expCoeff/float(sampleRate));
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
		if(size_t(mFragmentOffset + 0.0001f) >= mSampleFragment.Length()) //Добавка 0.0001, чтобы обойти баг компилятора со сравнением
		{
			mFragmentOffset -= float(mSampleFragment.Length());
			mAttenuation *= Math::Pow(mAttenuationStep, mRate);
		}
		//Без добавки 0.0001 на некоторых конфигурациях проекта срабатывал ассерт
		//INTRA_ASSERT(size_t(mFragmentOffset) < mSampleFragment.Length());
		if(j >= mSampleFragment.Length()) j = 0;
		const float sample = mSampleFragment[i]*(mAttenuation - factor) + mSampleFragment[j]*factor;
		if(!add) *dst.Begin++ = sample;
		else *dst.Begin++ += sample;
		mRate += mRateAcceleration;
		if(mRate < 0.25f) mRate = 0.25f;
	}
}

Span<float> WaveTableSampler::operator()(Span<float> dst, bool add)
{
	if(mRate == 1 && mRateAcceleration == 0) generateWithDefaultRate(dst, add);
	else
	{
		auto dstCopy = dst;
		while(!dstCopy.Full()) generateWithVaryingRate(dstCopy.TakeAdvance(1024), add);
	}
	return mAttenuation < 0.0001f? dst.Tail(1): dst.Tail(0);
}

WaveTableSampler WaveTableSampler::Sine(uint octaves, float rateAcceleration,
	float expCoeff, float volume, float freq, uint sampleRate)
{
	return WaveTableSampler(
		[](Span<float> dst, float freq, float volume, uint sampleRate)
	{
		Math::SineRange<float> sine(volume, 0, float(2*Math::PI*freq/sampleRate));
		ReadTo(sine, dst);
	}, octaves, rateAcceleration/sampleRate, expCoeff, volume, freq, sampleRate);
}

WaveTableSampler WaveTableSampler::Sawtooth(uint octaves, float rateAcceleration, float updownRatio,
	float expCoeff, float volume, float freq, uint sampleRate)
{
	return WaveTableSampler(
		[updownRatio](Span<float> dst, float freq, float volume, uint sampleRate)
	{
		Generators::Sawtooth saw(updownRatio, freq, volume, sampleRate);
		ReadTo(saw, dst);
	}, octaves, rateAcceleration/sampleRate, expCoeff, volume, freq, sampleRate);
}

WaveTableSampler WaveTableSampler::Square(uint octaves, float rateAcceleration, float updownRatio,
	float expCoeff, float volume, float freq, uint sampleRate)
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
	}, octaves, rateAcceleration/sampleRate, expCoeff, volume, freq, sampleRate);
}

WaveTableSampler WaveTableSampler::WhiteNoise(uint octaves, float rateAcceleration,
	float expCoeff, float volume, float freq, uint sampleRate)
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
	}, octaves, rateAcceleration/sampleRate, expCoeff, volume, freq, sampleRate);
}

WaveTableSampler WaveTableSampler::WaveTable(uint octaves, float rateAcceleration,
	CSpan<float> table, float expCoeff, float volume, float freq, uint sampleRate)
{
	return WaveTableSampler(
		[table](Span<float> dst, float freq, float volume, uint sampleRate)
	{
		uint samplesPerPeriod = uint(Math::Round(float(sampleRate)/freq));
		if(samplesPerPeriod == 0) samplesPerPeriod = 1;
		auto samplePeriod = dst.Take(samplesPerPeriod);
		ResampleLinear(table, dst);
		Multiply(dst, volume);
		dst.PopFirstN(samplesPerPeriod);
		while(!dst.Full()) WriteTo(samplePeriod, dst);
	}, octaves, rateAcceleration/sampleRate, expCoeff, volume, freq, sampleRate);
}

WaveTableSampler WaveInstrument::operator()(float freq, float volume, uint sampleRate) const
{
	switch(Type)
	{
	case WaveType::Sine:
		return WaveTableSampler::Sine(Octaves, RateAcceleration, ExpCoeff,
			volume*Scale, freq*FreqMultiplier, sampleRate);

	case WaveType::Sawtooth:
		return WaveTableSampler::Sawtooth(Octaves, RateAcceleration, UpdownRatio, ExpCoeff,
			volume*Scale, freq*FreqMultiplier, sampleRate);

	case WaveType::Square:
		return WaveTableSampler::Square(Octaves, RateAcceleration, UpdownRatio, ExpCoeff,
			volume*Scale, freq*FreqMultiplier, sampleRate);

	case WaveType::WhiteNoise:
		return WaveTableSampler::WhiteNoise(Octaves, RateAcceleration, ExpCoeff,
			volume*Scale, freq*FreqMultiplier, sampleRate);
	}
	return null;
}

WaveTableSampler WaveTableInstrument::operator()(float freq, float volume, uint sampleRate) const
{
	return WaveTableSampler::WaveTable(Octaves, RateAcceleration, Table, ExpCoeff, volume, freq, sampleRate);
}

}}}
