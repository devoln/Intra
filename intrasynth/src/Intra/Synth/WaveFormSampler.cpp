#include "WaveFormSampler.h"

#include "Generators.hh"
#include "Generators/Square.h"
#include <Random/FastUniform.h>
#include <Audio/Synth/ExponentialAttenuation.h>

using Intra::Audio::Synth::ExponentialAttenuate;

// Mirrors WaveTableSampler::randGen (used by its constructor for the initial
// phase). WaveFormSampler builds its fragment after the base constructor has
// already run, so it recomputes the phase with the real fragment length here.
static auto randGen(Span<const float> periodicWave, float rate, float volume)
{
	return Random::FastUniform<unsigned>(
		1436491347u ^ unsigned(periodicWave.Length()) ^ unsigned(rate*1537) ^ unsigned(volume * 349885300.0f)
	);
}

// One-sample placeholder passed to the base constructor. The real fragment is
// allocated by prepareInternalData after the object is fully constructed.
static const float sPlaceholderFragmentSample = 0;

static unsigned GetGoodSignalPeriod(float samplesPerPeriod, unsigned maxPeriods, float eps)
{
	const float fract = Math::Fract(samplesPerPeriod);
	if(fract <= eps/2) return 1;
	float minDeltaCnt = 1;
	unsigned minDeltaN = 0;
	for(unsigned n = 1; fract*float(n) < float(maxPeriods) || minDeltaCnt > eps; n++)
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

void WaveFormSampler::prepareInternalData(const void* params, WaveForm wave,
	float freq, float volume, unsigned sampleRate, bool goodPeriod, bool prepareToStereoDataMutation)
{
	const float samplesPerPeriod = float(sampleRate)/freq;
	if(samplesPerPeriod < 1) return;
	const unsigned signalPeriods = !goodPeriod ? 1 :
		GetGoodSignalPeriod(samplesPerPeriod, unsigned(Math::Round(1000/samplesPerPeriod)) + 1, 0.2f);
	const unsigned goodSignalPeriodSamples = unsigned(Math::Round(samplesPerPeriod*float(signalPeriods)));
	if(signalPeriods > 1) freq = float(sampleRate*signalPeriods) / float(goodSignalPeriodSamples);
	mSampleFragmentData.SetCount(prepareToStereoDataMutation ? goodSignalPeriodSamples*2 : goodSignalPeriodSamples);
	mSampleFragmentStart = mSampleFragmentData.Data();
	mSampleFragmentLength = goodSignalPeriodSamples;
	wave(params, mSampleFragmentData.AsRange().Take(mSampleFragmentLength), freq, volume, sampleRate);

	// The base constructor ran with a placeholder span, so recompute the phase
	// offsets for the real fragment here. This matches WaveTableSampler's
	// constructor (rate=1, volume=1); channelDeltaSamples is applied in the body.
	mFragmentOffset = float(randGen(Span<const float>(mSampleFragmentStart, mSampleFragmentLength), 1, 1)(mSampleFragmentLength));
	mRightFragmentOffset = unsigned(mFragmentOffset) % mSampleFragmentLength;
}

void WaveFormSampler::preattenuateExponential(float expCoeff, unsigned sampleRate)
{
	const float ek = Math::Exp(-expCoeff/float(sampleRate));
	Span<float> dst = mSampleFragmentData.AsRange();
	Span<const float> src = mSampleFragmentData.AsRange();
	const float startAtten = Math::Exp(expCoeff*mFragmentOffset/float(sampleRate));
	float atten = startAtten;
	ExponentialAttenuate(dst, src, atten, ek);
	mExpAtten.FactorStep = atten/startAtten;
}

WaveFormSampler::WaveFormSampler(const void* params, WaveForm wave,
	float expCoeff, float volume, float freq, unsigned sampleRate,
	float vibratoFrequency, float vibratoValue, float smoothingFactor, const Envelope& envelope):
	WaveTableSampler(
		Span<const float>(&sPlaceholderFragmentSample, 1),
		1, 1, 1, 2*float(Math::PI)*vibratoFrequency/float(sampleRate), vibratoValue, envelope, 0),
	mRightSampleFragmentStartIndex(0), mSmoothingFactor(smoothingFactor)
{
	// Build the real fragment only after every member (including
	// mSampleFragmentData) has been constructed. The old code called this from
	// the base-class initializer, reading/writing mSampleFragmentData before its
	// lifetime began — undefined behaviour that crashes under SAFE_HEAP.
	prepareInternalData(params, wave, freq, volume, sampleRate, smoothingFactor == 0, smoothingFactor != 0);

	const size_t channelDeltaSamples = (sampleRate >> 7) % mSampleFragmentLength;
	if(mRightFragmentOffset < channelDeltaSamples) mRightFragmentOffset += mSampleFragmentLength;
	mRightFragmentOffset -= channelDeltaSamples;

	if(expCoeff == 0) return; //��������� ���, ������� ������� mExpAtten = {1, 1} - no op

	if(canDataMutate()) mExpAtten.FactorStep = Math::Exp(-expCoeff/float(sampleRate));
	else preattenuateExponential(expCoeff, sampleRate);
}

Sampler& WaveInstrument::CreateSampler(float freq, float volume, unsigned sampleRate,
	SamplerContainer& dst, uint16* oIndex) const
{
	const float vibratoFreq = (VibratoFrequency < 0 ? -freq : 1) * VibratoFrequency;
	WaveFormSampler& result = dst.Add<WaveFormSampler>(Wave, ExpCoeff,
		volume*Scale, freq*FreqMultiplier, sampleRate,
		vibratoFreq, VibratoValue, SmoothingFactor, Envelope(sampleRate));
	if(oIndex) *oIndex = uint16(dst.Length() - 1);
	return result;
}

WaveFormSampler WaveInstrument::operator()(float freq, float volume, unsigned sampleRate) const
{
	const float vibratoFreq = (VibratoFrequency < 0 ? -freq : 1) * VibratoFrequency;
	return WaveFormSampler(Wave, ExpCoeff,
		volume*Scale, freq*FreqMultiplier, sampleRate,
		vibratoFreq, VibratoValue, SmoothingFactor, Envelope(sampleRate));
}

static float smoothFilterBuffer(Span<float> dst, Span<const float> src, float prevSample, float smoothFactor, float attenuation)
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



void SineWaveForm::operator()(Span<float> dst, float freq, float volume, unsigned sampleRate) const
{
	Math::SineRange<float> sine(volume, 0, float(2*Math::PI*freq/float(sampleRate)));
	ReadTo(sine, dst);
}

void SawtoothWaveForm::operator()(Span<float> dst, float freq, float volume, unsigned sampleRate) const
{
	Generators::Sawtooth saw(UpdownRatio, freq, volume, sampleRate);
	ReadTo(saw, dst);
}

void PulseWaveForm::operator()(Span<float> dst, float freq, float volume, unsigned sampleRate) const
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

void WhiteNoiseWaveForm::operator()(Span<float> dst, float freq, float volume, unsigned sampleRate) const
{
	unsigned samplesPerPeriod = unsigned(Math::Round(float(sampleRate)/freq));
	if(samplesPerPeriod == 0) samplesPerPeriod = 1;
	Random::FastUniform<float> noise;
	auto samplePeriod = dst.Take(samplesPerPeriod);
	for(size_t i = 0; i < samplesPerPeriod; i++) dst.Put(noise.SignedNext()*volume);
	while(!dst.Full()) WriteTo(samplePeriod, dst);
}

void GuitarWaveForm::operator()(Span<float> dst, float freq, float volume, unsigned sampleRate) const
{
	(void)freq; (void)sampleRate;
	unsigned samplesPerPeriod = dst.Length();
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
