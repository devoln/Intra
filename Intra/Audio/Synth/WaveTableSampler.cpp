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

static uint GetGoodSignalPeriod(float samplesPerPeriod, uint maxPeriods, float eps)
{
	const float fract = Math::Fract(samplesPerPeriod);
	if(fract <= eps/2) return 1;
	float minDeltaCnt = 1;
	uint minDeltaN = 0;
	for(uint n = 1; fract*float(n) < float(maxPeriods) || minDeltaCnt > eps; n++)
	{
		float delta = Math::Fract(fract*float(n));
		if(delta > 0.5f) delta = 1 - delta;
		if(minDeltaCnt > delta)
		{
			minDeltaCnt = delta;
			minDeltaN = n;
		}
	}
	return minDeltaN;
}

WaveTableSampler::WaveTableSampler(const void* params, WaveForm wave, uint octaves,
	float expCoeff, float volume, float freq, uint sampleRate,
	float vibratoFrequency, float vibratoValue, float smoothingFactor, const AdsrAttenuator& adsr):
	mRate(1), mLeftMultiplier(0.5f), mRightMultiplier(0.5f), mReverbMultiplier(0),
	mFreqOscillator(vibratoValue, 0, 2*float(Math::PI)*vibratoFrequency/float(sampleRate)),
	mADSR(adsr), mSmoothingFactor(smoothingFactor)
{
	INTRA_DEBUG_ASSERT(octaves >= 1);
	const float samplesPerPeriod = float(sampleRate)/freq;
	if(samplesPerPeriod < 1) return;
	const uint goodPeriod = mSmoothingFactor != 0? 1: GetGoodSignalPeriod(samplesPerPeriod*float(1 << (octaves - 1)), uint(Math::Round(1000/samplesPerPeriod)) + 1, 0.2f);
	const uint goodSignalPeriodSamples = uint(Math::Round(samplesPerPeriod*float(1 << (octaves - 1))*float(goodPeriod)));
	if(mSmoothingFactor == 0) freq = float((sampleRate*goodPeriod) << (octaves - 1)) / float(goodSignalPeriodSamples);
	else mRate = float(goodSignalPeriodSamples)/samplesPerPeriod;
	mSampleFragmentData.SetCountUninitialized(goodSignalPeriodSamples);
	mSampleFragment = mSampleFragmentData;

	if(octaves > 1)
	{
		volume *= 2.0f - 2.0f/float(1 << octaves);
		Array<float> buffer;
		buffer.SetCountUninitialized(goodSignalPeriodSamples);
		wave(params, octaves == 2? buffer.AsRange(): mSampleFragmentData.AsRange(), freq, volume, sampleRate);
		Span<float> src = octaves == 2? buffer.AsRange(): mSampleFragmentData.AsRange();
		GenOctaves(src, octaves > 2? buffer.AsRange(): mSampleFragmentData.AsRange(), octaves, 20);
	}
	else wave(params, mSampleFragmentData, freq, volume, sampleRate);

	Random::FastUniform<uint> rand(1436491347u ^ uint(mSampleFragment.Length()) ^ uint(freq*1000) ^ (octaves << 15));
	mFragmentOffset = float(rand(uint(mSampleFragment.Length())));

	mChannelDeltaSamples = 0;// (sampleRate >> 7) % mSampleFragment.Length();

	if(expCoeff <= 0.001f || mSmoothingFactor != 0)
	{
		mExpAtten.FactorStep = Math::Exp(-expCoeff/float(sampleRate));
		return;
	}

	const float ek = Math::Exp(-expCoeff/float(sampleRate));
	Span<float> dst = mSampleFragmentData;
	CSpan<float> src = mSampleFragmentData;
	const float startAtten = Math::Exp(expCoeff*mFragmentOffset/float(sampleRate));
	float atten = startAtten;
	ExponentialAttenuate(dst, src, atten, ek);
	mExpAtten.FactorStep = atten/startAtten;
}

WaveTableSampler::WaveTableSampler(CSpan<float> periodicWave, float rate,
	float attenuationPerSample, float volume, float vibratoDeltaPhase, float vibratoValue, const AdsrAttenuator& adsr, size_t channelDeltaSamples):
	mSampleFragment(periodicWave), mRate(rate), mLeftMultiplier(0.5f), mRightMultiplier(0.5f), mReverbMultiplier(0),
	mFreqOscillator(vibratoValue, 0, vibratoDeltaPhase), mADSR(adsr), mChannelDeltaSamples(channelDeltaSamples), mSmoothingFactor(0)
{
	mExpAtten.Factor = volume;
	mExpAtten.FactorStep = attenuationPerSample;
	Random::FastUniform<uint> rand(1436491347u ^ uint(periodicWave.Length()) ^ uint(rate*1537) ^ uint(volume * 349885300.0f));
	mFragmentOffset = 0;// float(rand(uint(mSampleFragment.Length())));
}

void WaveTableSampler::generateWithDefaultRate(Span<float> dstLeft, Span<float> dstRight, Span<float> dstReverb, bool add)
{
	if(dstRight == dstLeft) dstRight = null;
	auto rightAttenuation = mExpAtten;
	auto reverbAttenuation = mExpAtten;
	auto rightAdsr = mADSR;
	auto reverbAdsr = mADSR;
#if 0
	float rightFragmentOffset = mFragmentOffset - float(mChannelDeltaSamples);
	if(rightFragmentOffset < 0)
	{
		rightFragmentOffset += float(mSampleFragment.Length());
		if(OwnExponentialAttenuatedDataArray()) rightAttenuation.Factor /= rightAttenuation.FactorStep;
	}
#endif
	while(!dstLeft.Empty() || !dstRight.Empty() || !dstReverb.Empty())
	{
		const float rightFragmentOffset = mFragmentOffset;
		auto leftFragment = mSampleFragment.Drop(size_t(mFragmentOffset)).Take(dstLeft.Length());
		auto rightFragment = mSampleFragment.Drop(size_t(rightFragmentOffset)).Take(dstRight.Length());
		auto reverbFragment = mSampleFragment.Drop(size_t(mFragmentOffset)).Take(dstReverb.Length());
		const size_t leftSamplesToProcess = leftFragment.Length();
		const size_t rightSamplesToProcess = rightFragment.Length();

		// Нужно наложить экспоненциальное затухание и\или ADSR огибающую на генерируемый фрагмент,
		// записав или добавив результат в dst, не модифицируя сам фрагмент.
		// Здесь выбирается нужный алгоритм для нужной комбинации.
		if(!OwnDataArray() && mExpAtten.FactorStep != 1)
		{
			//Требуется накладывать экспоненциальное затухание, причём используется сторонний периодический семпл.
			//В этом случае невозможно применить трюк с предварительным наложением экспоненты на периодический семпл.
			//Поэтому честно накладываем экспоненту и ADSR. Класс ADSR умеет делать это всё за один проход.
			if(rightSamplesToProcess) rightAdsr(dstRight, rightFragment, add, mRightMultiplier, rightAttenuation);
			if(reverbFragment != null) reverbAdsr(dstReverb, reverbFragment, add, mReverbMultiplier, reverbAttenuation);
			mADSR(dstLeft, leftFragment, add, mLeftMultiplier, mExpAtten);
		}
		else
		{
			//Экспоненциальное затухание не требуется, либо имеется свой собственный периодический семпл, на который уже наложено экспоненциальное затухание.
			//Теперь экспонента сводится к умножению на постоянное в пределах данного фрагмента число (1 - если экспоненциальное затухание не требуется).
			//Остаётся только применить ADSR, указав эту константу в качестве громкости.
			if(rightSamplesToProcess) rightAdsr(dstRight, rightFragment, add, mExpAtten.Factor*mRightMultiplier);
			if(reverbFragment != null) reverbAdsr(dstReverb, reverbFragment, add, mExpAtten.Factor*mReverbMultiplier);
			mADSR(dstLeft, leftFragment, add, mExpAtten.Factor*mLeftMultiplier);
		}
		dstLeft.PopFirstExactly(leftFragment.Length());
		dstRight.PopFirstExactly(rightFragment.Length());
		dstReverb.PopFirstExactly(reverbFragment.Length());
		mFragmentOffset += float(leftSamplesToProcess);
		if(mFragmentOffset >= mSampleFragment.Length())
		{
			// Если этот объект имеет собственный фрагмент семплов, то на него уже наложено экспоненциальное затухание.
			// Достаточно уменьшить один общий множитель.
			if(OwnExponentialAttenuatedDataArray()) mExpAtten.Factor *= mExpAtten.FactorStep;
			mFragmentOffset = 0;
		}
#if 0
		rightFragmentOffset += float(rightSamplesToProcess);
		if(rightFragmentOffset >= mSampleFragment.Length())
		{
			// Если этот объект имеет собственный фрагмент семплов, то на него уже наложено экспоненциальное затухание.
			// Достаточно уменьшить один общий множитель.
			if(OwnExponentialAttenuatedDataArray()) rightAttenuation.Factor *= rightAttenuation.FactorStep;
			rightFragmentOffset = 0;
		}
#endif
	}
}

static float smoothFilterBuffer(Span<float> buffer, float prevSample, float smoothFactor, float attenuation)
{
	smoothFactor *= attenuation;
	const float invSmoothFactor = attenuation - smoothFactor;
	for(float& sample: buffer)
	{
		const float temp = sample;
		sample = invSmoothFactor*prevSample + smoothFactor*sample;
		prevSample = temp;
	}
	return prevSample;
}

template<bool Add, bool FreqOsc, bool Adsr>
void WaveTableSampler::generateWithVaryingRate(Span<float> dstLeft, Span<float> dstRight, Span<float> dstReverb)
{
	if(dstRight.Empty()) dstRight.Begin = dstRight.End = null;
	if(dstReverb.Empty()) dstReverb.Begin = dstReverb.End = null;
	const float rightMult = dstRight == null? 0.5f: mRightMultiplier;
	const float leftMult = dstRight == null? 0.5f: mLeftMultiplier;
	const float reverbMult = mReverbMultiplier;

	float adsrU = Adsr && mADSR? mADSR.U: 1;
	const float adsrDU = mADSR.DU;
	const size_t len = mSampleFragment.Length();
	const bool preattenuated = OwnExponentialAttenuatedDataArray();
	const float att = preattenuated? 1: mExpAtten.FactorStep;
	while(dstLeft.Begin != dstLeft.End)
	{
		const int ii = int(mFragmentOffset);
		size_t i = size_t(ii);
		const float totalAttenuation = mExpAtten.Factor * (Adsr? adsrU: 1);
		const float factor = mFragmentOffset - float(ii);
		size_t j = i + 1;
		const float dt = FreqOsc? mRate*(1+mFreqOscillator.Next()): mRate;
		mFragmentOffset += dt;
		float a;
		if(j >= len)
		{
			if(mSmoothingFactor != 0 && i < len) a = mLastFragmentSample = smoothFilterBuffer(mSampleFragmentData, mLastFragmentSample, mSmoothingFactor, 1);
			else a = mSampleFragment.Begin[i < len? i: i-len];
			if(i >= len)
			{
				mFragmentOffset -= float(len);
				if(preattenuated) mExpAtten.Factor *= mExpAtten.FactorStep;
				i -= len;
			}
			j -= len;
		}
		else a = mSampleFragment.Begin[i];
		mExpAtten.Factor *= att;

		const float b = mSampleFragment.Begin[j];
		float sample = (a + (b-a)*factor);
		sample *= totalAttenuation;

		if(!Add)
		{
			*dstLeft.Begin++ = sample*leftMult;
			if(dstRight.Begin) *dstRight.Begin++ = sample*rightMult;
			if(dstReverb.Begin) *dstReverb.Begin++ = sample*reverbMult;
		}
		else
		{
			*dstLeft.Begin++ += sample*leftMult;
			if(dstRight.Begin) *dstRight.Begin++ += sample*rightMult;
			if(dstReverb.Begin) *dstReverb.Begin++ += sample*reverbMult;
		}

		if(Adsr)
		{
			if(mADSR.Linear) adsrU += adsrDU;
			else adsrU *= adsrDU;
		}
	}
	
	if(Adsr) if(mADSR) mADSR.U = adsrU;
}

void WaveTableSampler::generateWithVaryingRate(Span<float> dstLeft, Span<float> dstRight, Span<float> dstReverb, bool add)
{
	const bool noAdsr = mADSR.U == 1 && ((mADSR.Linear && mADSR.DU == 0) || mADSR.DU == 1);
	if(!add)
	{
		if(mFreqOscillator == null)
		{
			if(noAdsr) generateWithVaryingRate<false, false, false>(dstLeft, dstRight, dstReverb);
			else generateWithVaryingRate<false, false, true>(dstLeft, dstRight, dstReverb);
		}
		else
		{
			if(noAdsr) generateWithVaryingRate<false, true, false>(dstLeft, dstRight, dstReverb);
			else generateWithVaryingRate<false, true, true>(dstLeft, dstRight, dstReverb);
		}
		return;
	}
	if(mFreqOscillator == null)
	{
		if(noAdsr) generateWithVaryingRate<true, false, false>(dstLeft, dstRight, dstReverb);
		else generateWithVaryingRate<true, false, true>(dstLeft, dstRight, dstReverb);
	}
	else
	{
		if(noAdsr) generateWithVaryingRate<true, true, false>(dstLeft, dstRight, dstReverb);
		else generateWithVaryingRate<true, true, true>(dstLeft, dstRight, dstReverb);
	}
}

Span<float> WaveTableSampler::operator()(Span<float> dst, bool add)
{
	return dst.Drop(operator()(dst, dst, add));
}

size_t WaveTableSampler::operator()(Span<float> dstLeft, Span<float> dstRight, bool add)
{
	return operator()(dstLeft, dstRight, null, add);
}

size_t WaveTableSampler::operator()(Span<float> dstLeft, Span<float> dstRight, Span<float> dstReverb, bool add)
{
	INTRA_DEBUG_ASSERT(dstLeft.Length() == dstRight.Length() && (dstLeft.Length() == dstReverb.Length() || dstReverb.Empty()));
	if(mSampleFragment.Empty()) return 0;
	
	auto dstLeftCopy = dstLeft.Take(mADSR.SamplesLeft());
	auto dstRightCopy = dstRight.Take(mADSR.SamplesLeft());
	auto dstReverbCopy = dstReverb.Take(mADSR.SamplesLeft());
	const size_t samplesToProcess = dstLeftCopy.Length();

	if(mRate == 1 && mFreqOscillator == null && mSmoothingFactor == 0)
	{
		generateWithDefaultRate(dstLeftCopy, dstRightCopy, dstReverbCopy, add);
	}
	else while(!dstLeftCopy.Full())
	{
		auto dstLeftPartToProcess = dstLeftCopy.TakeAdvance(mADSR.CurrentStateSamplesLeft());
		auto dstRightPartToProcess = dstRightCopy.TakeAdvance(dstLeftPartToProcess.Length());
		auto dstReverbPartToProcess = dstReverbCopy.TakeAdvance(dstLeftPartToProcess.Length());
		generateWithVaryingRate(dstLeftPartToProcess, dstRightPartToProcess, dstReverbPartToProcess, add);
		mADSR.SamplesProcessedExternally(dstLeftPartToProcess.Length());
	}

	if(samplesToProcess < dstLeft.Length()) return samplesToProcess;
	return mExpAtten.Factor < 0.00001f? samplesToProcess - 1: samplesToProcess;
}

void SineWaveForm::operator()(Span<float> dst, float freq, float volume, uint sampleRate) const
{
	Math::SineRange<float> sine(volume, 0, float(2*Math::PI*freq/float(sampleRate)));
	ReadTo(sine, dst);
}

void SawtoothWaveForm::operator()(Span<float> dst, float freq, float volume, uint sampleRate) const
{
	Generators::Sawtooth saw(UpdownRatio, freq, volume, sampleRate);
	ReadTo(saw, dst);
}

void PulseWaveForm::operator()(Span<float> dst, float freq, float volume, uint sampleRate) const
{
	if(UpdownRatio == 1)
	{
		Generators::Square sqr(freq, sampleRate);
		ReadTo(sqr, dst);
	}
	else
	{
		Generators::Pulse rect(UpdownRatio, freq, sampleRate);
		ReadTo(rect, dst);
	}
	Multiply(dst, volume);
}

void WhiteNoiseWaveForm::operator()(Span<float> dst, float freq, float volume, uint sampleRate) const
{
	uint samplesPerPeriod = uint(Math::Round(float(sampleRate)/freq));
	if(samplesPerPeriod == 0) samplesPerPeriod = 1;
	Random::FastUniform<float> noise;
	auto samplePeriod = dst.Take(samplesPerPeriod);
	for(size_t i = 0; i < samplesPerPeriod; i++) dst.Put(noise.SignedNext()*volume);
	while(!dst.Full()) WriteTo(samplePeriod, dst);
}

void GuitarWaveForm::operator()(Span<float> dst, float freq, float volume, uint sampleRate) const
{
	uint samplesPerPeriod = dst.Length();
	Random::FastUniform<float> noise;
	auto samplePeriod = dst.Take(samplesPerPeriod);
		
	for(size_t i = 0; i < samplesPerPeriod; i++)
	{
		float sample = float(i) * (float(samplesPerPeriod) - float(i)) / float(Math::Sqr(samplesPerPeriod/2));
		sample = sample*(1 - sample)*sample*(float(samplesPerPeriod)/2 - float(i)) / float(samplesPerPeriod/2);
		sample += noise.SignedNext() / float(samplesPerPeriod*4) / (1.0f / float(samplesPerPeriod*2) + Demp);
		sample *= volume;
		dst.Put(sample);
	}

	while(!dst.Full()) WriteTo(samplePeriod, dst);
}

WaveTableSampler WaveInstrument::operator()(float freq, float volume, uint sampleRate) const
{
	const float vibratoFreq = (VibratoFrequency < 0? -freq: 1)*VibratoFrequency;
	return WaveTableSampler(Wave, Octaves, ExpCoeff,
		volume*Scale, freq*FreqMultiplier, sampleRate,
		vibratoFreq, VibratoValue, SmoothingFactor, ADSR(freq, volume, sampleRate));
}

WaveTableSampler WaveTableInstrument::operator()(float freq, float volume, uint sampleRate) const
{
	auto& table = Tables->Get(freq, sampleRate);
	const float ratio = freq / float(sampleRate);
	const size_t level = table.NearestLevelForRatio(ratio);
	const auto samples = table.LevelSamples(level);
	return WaveTableSampler(samples, ratio/table.LevelRatio(level),
		Math::Exp(-ExpCoeff/float(sampleRate)), volume*VolumeScale,
		2*float(Math::PI)*VibratoFrequency/float(sampleRate), VibratoValue,
		ADSR(freq, volume, sampleRate), /*(sampleRate >> 7) % samples.Length()*/0);
}


WaveTable& WaveTableCache::Get(float freq, uint sampleRate) const
{
	float freqSampleRateRatio = freq/float(sampleRate);
	for(size_t i = 0; i < Tables.Length(); i++)
	{
		float rate = freqSampleRateRatio / Tables[i].BaseLevelRatio;
		if(!AllowMipmaps)
		{
			if(0.9999f < rate && rate < 1.0001f) return Tables[i];
			continue;
		}
		float r = rate;
		while(r >= 0.9999f)
		{
			if(0.9999f < r && r < 1.0001f) return Tables[i];
			r *= 0.5f;
		}
	}
	return Tables.AddLast(Generator(freq, sampleRate));
}

}}}
