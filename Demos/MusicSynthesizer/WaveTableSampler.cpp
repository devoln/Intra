#include "WaveTableSampler.h"
#include "ExponentialAttenuation.h"

#include "Generators/Sawtooth.h"
#include "Generators/Square.h"
#include "Generators/Pulse.h"
#include "Generators/WhiteNoise.h"

#include "Audio/AudioBuffer.h"

#include "Utils/Span.h"

#include "Funal/Bind.h"

#include "Math/Math.h"

#include "Simd/Simd.h"

#include "Range/Mutation/Copy.h"
#include "Range/Mutation/Fill.h"
#include "Range/Mutation/Transform.h"

#include "Container/Sequential/Array.h"

#include "Random/FastUniform.h"

static inline auto randGen(CSpan<float> periodicWave, float rate, float volume)
{
	return Random::FastUniform<uint>(
		1436491347u ^ uint(periodicWave.Length()) ^ uint(rate*1537) ^ uint(volume * 349885300.0f)
	);
}

WaveTableSampler::WaveTableSampler(CSpan<float> periodicWave, float rate,
	float attenuationPerSample, float volume, float vibratoDeltaPhase,
	float vibratoValue, const Envelope& envelope, size_t channelDeltaSamples):
	mSampleFragmentStart(periodicWave.Data()), mSampleFragmentLength(uint(periodicWave.Length())),
	mRate(rate), mLeftMultiplier(0.5f), mRightMultiplier(0.5f), mReverbMultiplier(0),
	mFreqOscillator(vibratoValue, 0, vibratoDeltaPhase), mEnvelope(envelope),
	mExpAtten(ExponentAttenuator::FromFactorAndStep(volume, attenuationPerSample)),
	mFragmentOffset(randGen(periodicWave, rate, volume)(mSampleFragmentLength)),
	mRightFragmentOffset((uint(mFragmentOffset) + channelDeltaSamples) % mSampleFragmentLength)
{}

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

CSpan<float> WaveFormSampler::prepareInternalData(const void* params, WaveForm wave,
	float freq, float volume, uint sampleRate, bool goodPeriod, bool prepareToStereoDataMutation)
{
	const float samplesPerPeriod = float(sampleRate)/freq;
	if(samplesPerPeriod < 1) return;
	const uint goodPeriod = !goodPeriod? 1: GetGoodSignalPeriod(samplesPerPeriod, uint(Math::Round(1000/samplesPerPeriod)) + 1, 0.2f);
	const uint goodSignalPeriodSamples = uint(Math::Round(samplesPerPeriod*float(goodPeriod)));
	if(goodPeriod) freq = float(sampleRate*goodPeriod) / float(goodSignalPeriodSamples);
	else mRate = float(goodSignalPeriodSamples)/samplesPerPeriod;
	mSampleFragmentData.SetCount(prepareToStereoDataMutation? goodSignalPeriodSamples*2: goodSignalPeriodSamples);
	mSampleFragmentStart = mSampleFragmentData.Data();
	mSampleFragmentLength = goodSignalPeriodSamples;
	wave(params, mSampleFragmentData, freq, volume, sampleRate);
}

void WaveFormSampler::preattenuateExponential(float expCoeff, uint sampleRate)
{
	const float ek = Math::Exp(-expCoeff/float(sampleRate));
	Span<float> dst = mSampleFragmentData;
	CSpan<float> src = mSampleFragmentData;
	const float startAtten = Math::Exp(expCoeff*mFragmentOffset/float(sampleRate));
	float atten = startAtten;
	ExponentialAttenuate(dst, src, atten, ek);
	mExpAtten.FactorStep = atten/startAtten;
}

WaveFormSampler::WaveFormSampler(const void* params, WaveForm wave,
	float expCoeff, float volume, float freq, uint sampleRate,
	float vibratoFrequency, float vibratoValue, float smoothingFactor, const Envelope& envelope):
	WaveTableSampler(
		prepareInternalData(params, wave, freq, volume, sampleRate, smoothingFactor == 0, smoothingFactor != 0),
		1, 1, 1, 2*float(Math::PI)*vibratoFrequency/float(sampleRate), vibratoValue, envelope, 0),
	mRightSampleFragmentStartIndex(0), mSmoothingFactor(smoothingFactor)
{
	const size_t channelDeltaSamples = (sampleRate >> 7) % mSampleFragmentLength;
	if(mRightFragmentOffset < channelDeltaSamples) mRightFragmentOffset += mSampleFragmentLength;
	mRightFragmentOffset -= channelDeltaSamples;

	if(expCoeff == 0) return; //затухания нет, поэтому оставим mExpAtten = {1, 1} - no op

	if(canDataMutate()) mExpAtten.FactorStep = Math::Exp(-expCoeff/float(sampleRate));
	else preattenuateExponential(expCoeff, sampleRate);
}

void WaveTableSampler::generateWithDefaultRate(Span<float> dstLeft, Span<float> dstRight, Span<float> dstReverb)
{
	if(dstRight == dstLeft) dstRight = null;
	auto rightAttenuation = mExpAtten;
	auto reverbAttenuation = mExpAtten;
	auto rightAdsr = mEnvelope;
	auto reverbAdsr = mEnvelope;

	if(mRightFragmentOffset > size_t(mFragmentOffset))
	{
		if(OwnExponentialAttenuatedDataArray())
			rightAttenuation.Factor /= rightAttenuation.FactorStep;
	}

	while(!dstLeft.Empty() || !dstRight.Empty() || !dstReverb.Empty())
	{
		auto leftFragment = SampleFragment(size_t(mFragmentOffset), dstLeft.Length());
		
		//TODO: RightSampleFragment for WaveFormSampler
		auto rightFragment = SampleFragment(size_t(mRightFragmentOffset), dstRight.Length());
		
		auto reverbFragment = SampleFragment(size_t(mFragmentOffset), dstReverb.Length());
		
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
			if(rightSamplesToProcess) rightAdsr(dstRight, rightFragment, mRightMultiplier, rightAttenuation);
			if(reverbFragment != null) reverbAdsr(dstReverb, reverbFragment, mReverbMultiplier, reverbAttenuation);
			mEnvelope(dstLeft, leftFragment, mLeftMultiplier, mExpAtten);
		}
		else
		{
			//Экспоненциальное затухание не требуется, либо имеется свой собственный периодический семпл, на который уже наложено экспоненциальное затухание.
			//Теперь экспонента сводится к умножению на постоянное в пределах данного фрагмента число (1 - если экспоненциальное затухание не требуется).
			//Остаётся только применить ADSR, указав эту константу в качестве громкости.
			if(rightSamplesToProcess) rightAdsr(dstRight, rightFragment, mExpAtten.Factor*mRightMultiplier);
			if(reverbFragment != null) reverbAdsr(dstReverb, reverbFragment, mExpAtten.Factor*mReverbMultiplier);
			mEnvelope(dstLeft, leftFragment, mExpAtten.Factor*mLeftMultiplier);
		}
		dstLeft.PopFirstExactly(leftFragment.Length());
		dstRight.PopFirstExactly(rightFragment.Length());
		dstReverb.PopFirstExactly(reverbFragment.Length());
		mFragmentOffset += float(leftSamplesToProcess);
		if(mFragmentOffset >= mSampleFragmentLength)
		{
			// Если этот объект имеет собственный фрагмент семплов, то на него уже наложено экспоненциальное затухание.
			// Достаточно уменьшить один общий множитель.
			if(OwnExponentialAttenuatedDataArray()) mExpAtten.Factor *= mExpAtten.FactorStep;
			mFragmentOffset = 0;
		}

		mRightFragmentOffset += rightSamplesToProcess;
		if(mRightFragmentOffset >= mSampleFragmentLength)
		{
			// Если этот объект имеет собственный фрагмент семплов, то на него уже наложено экспоненциальное затухание.
			// Достаточно уменьшить один общий множитель.
			if(OwnExponentialAttenuatedDataArray()) rightAttenuation.Factor *= rightAttenuation.FactorStep;
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
void WaveTableSampler::generateWithVaryingRate(Span<float> dstLeft, Span<float> dstRight, Span<float> dstReverb)
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
		float sample = (a + (b-a)*factor);
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

void WaveTableSampler::generateWithVaryingRate(Span<float> dstLeft, Span<float> dstRight, Span<float> dstReverb)
{
	const bool noOpEnvelope = mEnvelope.CurrentSegment.IsNoOp();
	if(mFreqOscillator == null)
	{
		if(noOpEnvelope) generateWithVaryingRate<false, false>(dstLeft, dstRight, dstReverb);
		else generateWithVaryingRate<false, true>(dstLeft, dstRight, dstReverb);
	}
	else
	{
		if(noOpEnvelope) generateWithVaryingRate<true, false>(dstLeft, dstRight, dstReverb);
		else generateWithVaryingRate<true, true>(dstLeft, dstRight, dstReverb);
	}
}

size_t WaveTableSampler::GenerateStereo(Span<float> ioDstLeft, Span<float> ioDstRight, Span<float> ioDstReverb)
{
	INTRA_DEBUG_ASSERT(dstLeft.Length() == dstRight.Length() && (dstLeft.Length() == dstReverb.Length() || dstReverb.Empty()));
	if(mSampleFragmentLength == 0) return 0; //TODO: как такое вообще может быть?
	const size_t samplesLeft = mEnvelope.CurrentSegment.SamplesLeft; //total samples left вместо этого???
	auto dstLeftCopy = ioDstLeft.Take(samplesLeft);
	auto dstRightCopy = ioDstRight.Take(samplesLeft);
	auto dstReverbCopy = ioDstReverb.Take(samplesLeft);
	const size_t samplesToProcess = dstLeftCopy.Length();

	if(mRate == 1 && mFreqOscillator == null && mSmoothingFactor == 0)
	{
		generateWithDefaultRate(dstLeftCopy, dstRightCopy, dstReverbCopy);
	}
	else while(!dstLeftCopy.Full())
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
	(void)freq; (void)sampleRate;
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

WaveFormSampler WaveInstrument::operator()(float freq, float volume, uint sampleRate) const
{
	const float vibratoFreq = (VibratoFrequency < 0? -freq: 1)*VibratoFrequency;
	return WaveFormSampler(Wave, ExpCoeff,
		volume*Scale, freq*FreqMultiplier, sampleRate,
		vibratoFreq, VibratoValue, SmoothingFactor, Envelope(sampleRate));
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
		Envelope(sampleRate), (sampleRate >> 7) % samples.Length());
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
