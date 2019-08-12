#include "WaveFormSampler.h"

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
