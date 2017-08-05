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
	mRate(1), mAttenuation(1), mAttenuationStep(1), mRightPanMultiplier(0.5f),
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

	mChannelDeltaSamples = (sampleRate >> 7) % mSampleFragment.Length();

	if(expCoeff <= 0.001f) return;

	const float ek = Math::Exp(-expCoeff/float(sampleRate));
	Span<float> dst = mSampleFragmentData;
	const float startAtten = Math::Exp(expCoeff*mFragmentOffset/float(sampleRate));
	float atten = startAtten;
	ExponentialAttenuate(dst, mSampleFragmentData, atten, ek);
	mAttenuationStep = atten/startAtten;
}

WaveTableSampler::WaveTableSampler(CSpan<float> periodicWave, float rate,
	float attenuationPerSample, float volume, float vibratoDeltaPhase, float vibratoValue, const AdsrAttenuator& adsr, size_t channelDeltaSamples):
	mSampleFragment(periodicWave), mRate(rate), mAttenuation(volume), mAttenuationStep(attenuationPerSample), mRightPanMultiplier(0.5f),
	mFreqOscillator(vibratoValue, 0, vibratoDeltaPhase), mADSR(adsr), mChannelDeltaSamples(channelDeltaSamples), mSmoothingFactor(0)
{
	Random::FastUniform<uint> rand(1436491347u ^ uint(periodicWave.Length()) ^ uint(rate*1537) ^ uint(volume * 349885300.0f));
	mFragmentOffset = float(rand(uint(mSampleFragment.Length())));
}

void WaveTableSampler::generateWithDefaultRate(Span<float> dst, bool add,
	float& fragmentOffset, float& attenuation, AdsrAttenuator& adsr)
{
	while(!dst.Empty())
	{
		auto fragment = mSampleFragment.Drop(size_t(fragmentOffset)).Take(dst.Length());
		const size_t samplesProcessed = fragment.Length();

		// Нужно наложить экспоненциальное затухание и\или ADSR огибающую на генерируемый фрагмент,
		// записав или добавив результат в dst, не модифицируя сам фрагмент.
		// Здесь выбирается нужный алгоритм для нужной комбинации.
		if(!OwnDataArray() && mAttenuationStep != 1)
		{
			if(adsr.DU == 0)
			{
				if(adsr.U == 0)
				{
					auto dstFragmentToProcess = dst.TakeAdvance(fragment.Length());
					if(!add) FillZeros(dstFragmentToProcess);
				}
				else
				{
					float totalAttenuation = attenuation;
					if(adsr) totalAttenuation *= adsr.U;
					if(!add) ExponentialAttenuate(dst, fragment, totalAttenuation, mAttenuationStep);
					else ExponentialAttenuateAdd(dst, fragment, totalAttenuation, mAttenuationStep);
					attenuation = totalAttenuation;
					if(adsr) attenuation /= adsr.U;
				}
			}
			else while(!fragment.Empty())
			{
				auto fragmentToProcess = fragment.TakeAdvance(adsr.CurrentStateSamplesLeft());
				if(adsr.DU == 0)
				{
					if(attenuation == 0 || adsr.U == 0)
					{
						auto dstFragmentToProcess = dst.TakeAdvance(fragmentToProcess.Length());
						if(!add) FillZeros(dstFragmentToProcess);
					}
					else
					{
						attenuation *= adsr.U;
						if(!add) ExponentialAttenuate(dst, fragmentToProcess, attenuation, mAttenuationStep);
						else ExponentialAttenuateAdd(dst, fragmentToProcess, attenuation, mAttenuationStep);
						attenuation /= adsr.U;
					}
				}
				else
				{
					if(!add) ExponentialLinearAttenuate(dst, fragmentToProcess, attenuation, mAttenuationStep, adsr.U, adsr.DU);
					else ExponentialLinearAttenuateAdd(dst, fragmentToProcess, attenuation, mAttenuationStep, adsr.U, adsr.DU);
				}
				adsr.SamplesProcessedExternally(fragmentToProcess.Length());
			}
		}
		else
		{
			if(adsr.DU == 0)
			{
				const float totalAttenuation = adsr? attenuation*adsr.U: attenuation;
				if(!add) Multiply(dst, fragment, totalAttenuation);
				else AddMultiplied(dst, fragment, totalAttenuation);
				dst.PopFirstExactly(samplesProcessed);
			}
			else for(;;)
			{
				auto fragmentToProcess = fragment.TakeAdvance(adsr.CurrentStateSamplesLeft());
				if(fragmentToProcess.Empty()) break;
				if(adsr.DU == 0)
				{
					if(!add) Multiply(dst, fragmentToProcess, attenuation*adsr.U);
					else AddMultiplied(dst, fragmentToProcess, attenuation*adsr.U);
				}
				else
				{
					float u = adsr.U*attenuation;
					const float du = adsr.DU*attenuation;
					if(!add) LinearMultiply(dst, fragmentToProcess, u, du);
					else LinearMultiplyAdd(dst, fragmentToProcess, u, du);
					adsr.U = u/attenuation;
					adsr.SamplesProcessedExternally(fragmentToProcess.Length());
				}
				dst.PopFirstN(fragmentToProcess.Length());
			}
		}
		fragmentOffset += float(samplesProcessed);
		if(fragmentOffset >= mSampleFragment.Length())
		{
			// Если этот объект имеет собственный фрагмент семплов, то на него уже наложено экспоненциальное затухание.
			// Достаточно уменьшить один общий множитель.
			if(OwnDataArray()) attenuation *= mAttenuationStep;
			fragmentOffset = 0;
		}
	}
}

void WaveTableSampler::generateWithVaryingRate(Span<float> dst, bool add,
	float& ioFragmentOffset, float& ioAttenuation, Math::SineRange<float>& ioFreqOscillator, AdsrAttenuator& ioAdsr)
{
	float fragmentOffset = ioFragmentOffset;
	float attenuation = ioAttenuation;
	auto freqOscillator = ioFreqOscillator;

	float adsrU = ioAdsr? ioAdsr.U: 1;
	const float adsrDU = ioAdsr.DU;
	const size_t len = mSampleFragment.Length();
	const float att = OwnDataArray()? 1: mAttenuationStep;
	while(dst.Begin != dst.End)
	{
		const int ii = int(fragmentOffset);
		size_t i = size_t(ii);
		const float totalAttenuation = attenuation*adsrU;
		const float factor = fragmentOffset - float(ii);
		size_t j = i + 1;
		const float dt = mRate*(1 + freqOscillator.Next());
		fragmentOffset += dt;
		if(j >= len)
		{
			if(i >= len)
			{
				fragmentOffset -= float(len);
				if(OwnDataArray()) attenuation *= mAttenuationStep;
				i -= len;
			}
			j -= len;
		}
		attenuation *= att;

		const float a = mSampleFragment.Begin[i];
		const float b = mSampleFragment.Begin[j];
		const float sample = (a + (b-a)*factor)*totalAttenuation;

		if(!add) *dst.Begin++ = sample;
		else *dst.Begin++ += sample;

		adsrU += adsrDU;
	}
	
	if(ioAdsr) ioAdsr.U = adsrU;

	ioFragmentOffset = fragmentOffset;
	ioAttenuation = attenuation;
	ioFreqOscillator = freqOscillator;
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

void WaveTableSampler::generateKS(Span<float> dstLeft, Span<float> dstRight, bool add)
{
	const float rightMult = dstRight == null? 0.5f: mRightPanMultiplier;
	const float leftMult = 1 - rightMult;
	if(dstRight == null) dstRight = dstLeft;
	INTRA_DEBUG_ASSERT(dstLeft.Length() == dstRight.Length());

	float adsrU = (mADSR? mADSR.U: 1);
	const float adsrDU = mADSR.DU;
	const size_t len = mSampleFragment.Length();
	while(dstLeft.Begin != dstLeft.End)
	{
		const intptr ii = intptr(mFragmentOffset);
		const float factor = (mFragmentOffset - float(ii)) * adsrU;
		const float dt = mRate*(1 + mFreqOscillator.Next());
		size_t i = size_t(ii);
		size_t j = i + 1;
		mFragmentOffset += dt;
		if(j >= len)
		{
			if(i >= len)
			{
				mFragmentOffset -= float(len);
				smoothFilterBuffer(mSampleFragmentData, mSampleFragmentData.Last(), mSmoothingFactor, mAttenuationStep);
				i -= len;
			}
			j -= len;
		}
		const float a = mSampleFragmentData[i];
		const float b = mSampleFragmentData[j];
		const float sample = (a + (b-a)*factor)*adsrU;
		if(!add)
		{
			*dstLeft.Begin++ = sample*leftMult;
			*dstRight.Begin++ = sample*rightMult;
		}
		else
		{
			*dstLeft.Begin++ += sample*leftMult;
			*dstRight.Begin++ += sample*rightMult;
		}
		adsrU += adsrDU;
	}
	if(mADSR) mADSR.U = adsrU;
}


Span<float> WaveTableSampler::operator()(Span<float> dst, bool add)
{
	if(mSampleFragment.Empty()) return dst;
	auto dstCopy = dst.Take(mADSR.SamplesLeft());
	auto notProcessedDstPart = dst.Drop(dstCopy.Length());
	if(mRate == 1 && mFreqOscillator == null)
	{
		generateWithDefaultRate(dstCopy, add, mFragmentOffset, mAttenuation, mADSR);
		mADSR.SamplesProcessedExternally(dstCopy.Length());
	}
	else
	{
		while(!dstCopy.Full())
		{
			auto dstPartToProcess = dstCopy.TakeAdvance(mADSR.CurrentStateSamplesLeft());
			if(mSmoothingFactor == 0) generateWithVaryingRate(dstPartToProcess, add,
				mFragmentOffset, mAttenuation, mFreqOscillator, mADSR);
			else generateKS(dst, add);
			mADSR.SamplesProcessedExternally(dstPartToProcess.Length());
		}
	}
	if(!notProcessedDstPart.Empty()) return notProcessedDstPart;
	return mAttenuation < 0.0001f? dst.Tail(1): dst.Tail(0);
}

size_t WaveTableSampler::operator()(Span<float> dstLeft, Span<float> dstRight, bool add)
{
	INTRA_DEBUG_ASSERT(dstLeft.Length() == dstRight.Length());
	if(mSampleFragment.Empty()) return 0;
	float attenuation = mAttenuation*(1 - mRightPanMultiplier);
	
	float fragmentOffset = mFragmentOffset - float(mChannelDeltaSamples);
	if(fragmentOffset < 0)
	{
		fragmentOffset += float(mSampleFragment.Length());
		attenuation /= mAttenuationStep;
	}
	auto adsr = mADSR;
	auto dstLeftCopy = dstLeft.Take(mADSR.SamplesLeft());
	auto dstRightCopy = dstRight.Take(mADSR.SamplesLeft());
	const size_t samplesToProcess = dstLeftCopy.Length();

	if(mRate == 1 && mFreqOscillator == null && mSmoothingFactor == 0)
	{
		if(mRightPanMultiplier != 1) generateWithDefaultRate(dstLeftCopy, add, fragmentOffset, attenuation, adsr);
		if(mRightPanMultiplier != 0)
		{
			mAttenuation *= mRightPanMultiplier;
			generateWithDefaultRate(dstRightCopy, add, mFragmentOffset, mAttenuation, mADSR);
			mAttenuation /= mRightPanMultiplier;
		}
	}
	else
	{
		auto freqOscillator = mFreqOscillator;
		while(!dstLeftCopy.Full())
		{
			auto dstLeftPartToProcess = dstLeftCopy.TakeAdvance(mADSR.CurrentStateSamplesLeft());
			auto dstRightPartToProcess = dstRightCopy.TakeAdvance(dstLeftPartToProcess.Length());
			if(mSmoothingFactor != 0)
			{
				if(mRightPanMultiplier == 1) generateKS(dstRightPartToProcess, add);
				else generateKS(dstLeftPartToProcess, dstRightPartToProcess, add);
				mADSR.SamplesProcessedExternally(dstRightPartToProcess.Length());
				continue;
			}
			if(mRightPanMultiplier != 1)
			{
				generateWithVaryingRate(dstLeftPartToProcess, add, fragmentOffset, attenuation, freqOscillator, adsr);
				adsr.SamplesProcessedExternally(dstLeftPartToProcess.Length());
			}
			if(mRightPanMultiplier != 0)
			{
				mAttenuation *= mRightPanMultiplier;
				generateWithVaryingRate(dstRightPartToProcess, add, mFragmentOffset, mAttenuation, mFreqOscillator, mADSR);
				mADSR.SamplesProcessedExternally(dstRightPartToProcess.Length());
				mAttenuation /= mRightPanMultiplier;
			}
			else mFreqOscillator = freqOscillator;
		}
	}

	if(mRightPanMultiplier == 1)
	{
		if(!add) FillZeros(dstLeftCopy);
	}
	if(mRightPanMultiplier == 0)
	{
		if(!add) FillZeros(dstRightCopy);
		if(mSmoothingFactor == 0)
		{
			mADSR = adsr;
			mAttenuation = attenuation;
			mFragmentOffset = fragmentOffset + float(mChannelDeltaSamples);
			if(mFragmentOffset >= float(mSampleFragment.Length()))
			{
				mFragmentOffset -= float(mSampleFragment.Length());
				mAttenuation *= mAttenuationStep;
			}
		}
	}

	if(samplesToProcess < dstLeft.Length()) return samplesToProcess;
	return mAttenuation < 0.0001f? samplesToProcess - 1: samplesToProcess;
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
	uint samplesPerPeriod = uint(Math::Round(float(sampleRate)/freq - 0.5f));
	if(samplesPerPeriod == 0) samplesPerPeriod = 1;
	Random::FastUniform<float> noise;
	auto samplePeriod = dst.Take(samplesPerPeriod);
		
	for(size_t i = 0; i < samplesPerPeriod; i++)
	{
		float sample = float(i) * (float(samplesPerPeriod) - float(i)) / Math::Sqr(samplesPerPeriod/2);
		sample = sample*(1 - sample)*sample*(float(samplesPerPeriod)/2 - float(i)) / (samplesPerPeriod/2);
		sample += noise.SignedNext() / (samplesPerPeriod*4) / (1.0f / float(samplesPerPeriod*2) + Demp);
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
		ADSR(freq, volume, sampleRate), (sampleRate >> 7) % samples.Length());
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
