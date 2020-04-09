#include "WaveTableSampler.h"
#include "ExponentialAttenuation.h"
#include "FixedRateTask.h"

#include "Generators/Sawtooth.h"
#include "Generators/Square.h"
#include "Generators/Pulse.h"
#include "Generators/WhiteNoise.h"

#include "Extra/Unstable/Audio/AudioBuffer.h"

#include "Intra/Range/Span.h"

#include "Funal/Bind.h"

#include "Intra/Math/Math.h"

#include "Simd/Simd.h"

#include "Intra/Range/Mutation/Copy.h"
#include "Intra/Range/Mutation/Fill.h"
#include "Intra/Range/Mutation/Transform.h"

#include "Extra/Container/Sequential/Array.h"

#include "Random/FastUniform.h"

static inline auto randGen(CSpan<float> periodicWave, float rate, float volume)
{
	return Random::FastUniform<unsigned>(
		1436491347u ^ unsigned(periodicWave.Length()) ^ unsigned(rate*1537) ^ unsigned(volume * 349885300.0f)
	);
}

WaveTableSampler::WaveTableSampler(CSpan<float> periodicWave, float rate,
	float attenuationPerSample, float volume, float vibratoDeltaPhase,
	float vibratoValue, const Envelope& envelope, size_t channelDeltaSamples):
	mSampleFragmentStart(periodicWave.Data()), mSampleFragmentLength(unsigned(periodicWave.Length())),
	mRate(rate), mLeftMultiplier(0.5f), mRightMultiplier(0.5f), mReverbMultiplier(0),
	mFreqOscillator(vibratoValue, 0, vibratoDeltaPhase), mEnvelope(envelope),
	mExpAtten(ExponentAttenuator::FromFactorAndStep(volume, attenuationPerSample)),
	mFragmentOffset(randGen(periodicWave, rate, volume)(mSampleFragmentLength)),
	mRightFragmentOffset((unsigned(mFragmentOffset) + channelDeltaSamples) % mSampleFragmentLength)
{}

void WaveTableSampler::generateWithDefaultRate(SamplerTaskContainer& dstTasks, size_t offsetInSamples, size_t numSamples)
{
	size_t leftOffsetInSamples = offsetInSamples;
	size_t rightOffsetInSamples = offsetInSamples;
	size_t leftSamplesLeft = numSamples;
	size_t rightSamplesLeft = numSamples;
	auto& leftEnvelope = mEnvelope;
	auto rightEnvelope = mEnvelope;
	auto& leftExpAtten = mExpAtten;
	auto rightExpAtten = mExpAtten;
	if(mRightFragmentOffset > size_t(mFragmentOffset))
	{
		if(OwnExponentialAttenuatedDataArray())
			rightExpAtten.Factor /= rightExpAtten.FactorStep;
	}

	while(leftSamplesLeft > 0)
	{
		//За одну итерацию обрабатываем не более одного сегмента огибающей
		auto leftFragment = SampleFragment(size_t(mFragmentOffset), leftSamplesLeft).Take(leftEnvelope.CurrentSegment.SamplesLeft);
		EnvelopeSegment leftSegment = leftEnvelope.CurrentSegment;
		if(!OwnExponentialAttenuatedDataArray() && leftExpAtten.FactorStep != 1)
		{
			//Требуется накладывать экспоненциальное затухание, причём используется сторонний периодический семпл.
			//В этом случае невозможно применить трюк с предварительным наложением экспоненты на периодический семпл.
			//Поэтому честно накладываем экспоненту и ADSR. Класс ADSR умеет делать это всё за один проход.
			leftSegment.Exp *= leftExpAtten;
		}
		if(mReverbMultiplier)
		{
			EnvelopeSegment reverbSegment = leftSegment;
			reverbSegment.Exp.Factor *= mReverbMultiplier;
			dstTasks.Add<NormalRateTask>(0, leftOffsetInSamples, leftFragment, reverbSegment);
		}
		if(mLeftMultiplier)
		{
			leftSegment.Exp.Factor *= mLeftMultiplier;
			dstTasks.Add<NormalRateTask>(0, leftOffsetInSamples, leftFragment, leftSegment);
		}

		leftOffsetInSamples += leftFragment.Length();
		leftSamplesLeft -= leftFragment.Length();
		leftEnvelope.CurrentSegment.Advance(leftFragment.Length());
		if(leftEnvelope.CurrentSegment.SamplesLeft == 0) leftEnvelope.StartNextSegment();
		leftExpAtten.SkipSamples(leftFragment.Length());
		mFragmentOffset += float(leftFragment.Length());
		if(mFragmentOffset >= mSampleFragmentLength)
		{
			// Если этот объект имеет собственный фрагмент семплов, то на него уже наложено экспоненциальное затухание.
			// Достаточно уменьшить один общий множитель.
			if(OwnExponentialAttenuatedDataArray()) leftExpAtten.Factor *= leftExpAtten.FactorStep;
			mFragmentOffset = 0;
		}
	}
	if(mRightMultiplier) while(rightSamplesLeft > 0)
	{
		//TODO: RightSampleFragment for WaveFormSampler
		auto rightFragment = SampleFragment(size_t(mRightFragmentOffset), rightSamplesLeft).Take(rightEnvelope.CurrentSegment.SamplesLeft);
		
		const size_t rightSamplesToProcess = rightFragment.Length();

		EnvelopeSegment rightSegment = rightEnvelope.CurrentSegment;
		if(!OwnExponentialAttenuatedDataArray() && rightExpAtten.FactorStep != 1)
		{
			//Требуется накладывать экспоненциальное затухание, причём используется сторонний периодический семпл.
			//В этом случае невозможно применить трюк с предварительным наложением экспоненты на периодический семпл.
			//Поэтому честно накладываем экспоненту и ADSR. Класс ADSR умеет делать это всё за один проход.
			rightSegment.Exp *= rightExpAtten;
		}
		rightSegment.Exp.Factor *= mRightMultiplier;
		dstTasks.Add<NormalRateTask>(0, offsetInSamples, rightFragment, rightSegment);
		
		rightOffsetInSamples += rightFragment.Length();
		rightSamplesLeft -= rightFragment.Length();
		rightEnvelope.CurrentSegment.Advance(rightFragment.Length());
		if(rightEnvelope.CurrentSegment.SamplesLeft == 0) rightEnvelope.StartNextSegment();
		mRightFragmentOffset += float(rightFragment.Length());
		if(mRightFragmentOffset >= mSampleFragmentLength)
		{
			// Если этот объект имеет собственный фрагмент семплов, то на него уже наложено экспоненциальное затухание.
			// Достаточно уменьшить один общий множитель.
			if(OwnExponentialAttenuatedDataArray()) rightExpAtten.Factor *= rightExpAtten.FactorStep;
			mRightFragmentOffset = 0;
		}
	}
}

static float smoothFilterBuffer(Span<float> dst, CSpan<float> src, float prevSample, float smoothFactor, float attenuation)
{
	INTRA_DEBUG_ASSERT(dst.Length() == src.Length());
	smoothFactor *= attenuation;
	const float invSmoothFactor = attenuation - smoothFactor;
	while(dst.Begin != dst.End)
	{
		const float curSample = *src.Begin++;
		*dst.Begin++ = invSmoothFactor*prevSample + smoothFactor*curSample;
		prevSample = curSample;
	}
	return prevSample;
}

template<bool FreqOsc, bool Envelope>
void WaveTableSampler::generateWithVaryingRateTask(Span<float> dstLeft, Span<float> dstRight, Span<float> dstReverb)
{
	if(dstRight.Empty()) dstRight.Begin = dstRight.End = null;
	if(dstReverb.Empty()) dstReverb.Begin = dstReverb.End = null;
	const float rightMult = dstRight == null? 0.5f: mRightMultiplier;
	const float leftMult = dstRight == null? 0.5f: mLeftMultiplier;
	const float reverbMult = mReverbMultiplier;
	INTRA_DEBUG_ASSERT(Envelope || mEnvelope.CurrentSegment.Volume == 1);
	float envelopeU = mEnvelope.CurrentSegment.Volume;
	const float adsrDU = mEnvelope.DU;
	const size_t len = mSampleFragment.Length();
	const bool preattenuated = OwnExponentialAttenuatedDataArray();
	const float att = preattenuated? 1: mExpAtten.FactorStep;
	while(dstLeft.Begin != dstLeft.End)
	{
		const int ii = int(mFragmentOffset);
		size_t i = size_t(ii);
		const float totalAttenuation = mExpAtten.Factor * (Adsr? envelopeU: 1);
		const float factor = mFragmentOffset - float(ii);
		size_t j = i + 1;
		const float dt = FreqOsc? mRate*(1 + mFreqOscillator.Next()): mRate;
		mFragmentOffset += dt;
		float a;
		if(j >= len)
		{
			if(mSmoothingFactor != 0 && i < len) a = mLastFragmentSample = smoothFilterBuffer(mSampleFragmentData, mSampleFragment, mLastFragmentSample, mSmoothingFactor, 1);
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
		float sample = a + (b-a)*factor;
		sample *= totalAttenuation;

		*dstLeft.Begin++ += sample*leftMult;
		if(dstRight.Begin) *dstRight.Begin++ += sample*rightMult;
		if(dstReverb.Begin) *dstReverb.Begin++ += sample*reverbMult;

		if(Envelope)
		{
			if(!mEnvelope.CurrentSegment.Exponential) envelopeU += adsrDU;
			else envelopeU *= adsrDU;
		}
	}
	
	if(Envelope) if(mEnvelope) mEnvelope.CurrentSegment = envelopeU;
}

void WaveTableSampler::generateWithVaryingRate(SamplerTaskContainer& dstTasks, size_t offsetInSamples, size_t numSamples)
{
	const bool noOpEnvelope = mEnvelope.CurrentSegment.IsNoOp();
	if(mFreqOscillator == null)
	{
		if(noOpEnvelope) generateWithVaryingRateTask<false, false>(dstLeft, dstRight, dstReverb);
		else generateWithVaryingRateTask<false, true>(dstLeft, dstRight, dstReverb);
	}
	else
	{
		if(noOpEnvelope) generateWithVaryingRateTask<true, false>(dstLeft, dstRight, dstReverb);
		else generateWithVaryingRateTask<true, true>(dstLeft, dstRight, dstReverb);
	}
}

bool WaveTableSampler::Generate(SamplerTaskContainer& dstTasks, size_t offsetInSamples, size_t numSamples)
{
	if(mRate == 1 && mFreqOscillator == null)
	{
		generateWithDefaultRate(dstTasks, offsetInSamples, numSamples);
	}
	else while(numSamples > 0)
	{
		if(mEnvelope.CurrentSegment.SamplesLeft == 0) mEnvelope.StartNextSegment();
		auto dstLeftPartToProcess = dstLeftCopy.TakeAdvance(mEnvelope.CurrentSegment.SamplesLeft);
		auto dstRightPartToProcess = dstRightCopy.TakeAdvance(dstLeftPartToProcess.Length());
		auto dstReverbPartToProcess = dstReverbCopy.TakeAdvance(dstLeftPartToProcess.Length());
		generateWithVaryingRate(dstLeftPartToProcess, dstRightPartToProcess, dstReverbPartToProcess);
		mEnvelope.CurrentSegment.SamplesLeft -= dstLeftPartToProcess.Length();
	}

	if(samplesToProcess < ioDstLeft.Length()) return samplesToProcess;
	return mExpAtten.Factor < 0.00001f? samplesToProcess - 1: samplesToProcess;
}

Sampler& CreateSampler(float freq, float volume, unsigned sampleRate,
	SamplerContainer& dst, uint16* oIndex = null)
{
	const float vibratoFreq = (VibratoFrequency < 0? -freq: 1)*VibratoFrequency;
	return WaveFormSampler(Wave, ExpCoeff,
		volume*Scale, freq*FreqMultiplier, sampleRate,
		vibratoFreq, VibratoValue, SmoothingFactor, Envelope(sampleRate));
}

WaveTableSampler WaveTableInstrument::operator()(float freq, float volume, unsigned sampleRate) const
{
	auto& table = Tables->Get(freq, sampleRate);
	const float ratio = freq / float(sampleRate);
	const size_t level = table.NearestLevelForRatio(ratio);
	const auto samples = table.LevelSamples(level);
	return WaveTableSampler(samples, ratio/table.LevelRatio(level),
		Exp(-ExpCoeff/float(sampleRate)), volume*VolumeScale,
		2*float(PI)*VibratoFrequency/float(sampleRate), VibratoValue,
		Envelope(sampleRate), (sampleRate >> 7) % samples.Length());
}


WaveTable& WaveTableCache::Get(float freq, unsigned sampleRate) const
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
