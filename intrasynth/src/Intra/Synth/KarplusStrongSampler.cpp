#include "KarplusStrongSampler.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

KarplusStrongSampler::KarplusStrongSampler(float freq, float volume, unsigned sampleRate,
	float damping, float smoothFactorBase, float smoothFactorMul, float smoothFactorExp,
	float scale, float expCoeff):
	mPos(0), mFrac(0), mPrevSample(0)
{
	Random::FastUniform<float> noise(randGen(freq, volume, sampleRate));

	const float note = Math::Log(freq / 16.352f) / Math::Log(2.0f) * 12.0f;
	const float noteFactor = Math::Max(0.0f, note / 128.0f);
	mSmoothFactor = smoothFactorBase *
		(0.9f - Math::Pow(noteFactor, smoothFactorExp)*smoothFactorMul + noise()*0.1f);

	const float precisePeriod = float(sampleRate) / freq - mSmoothFactor;
	const size_t len = Math::Max(size_t(1), size_t(Math::Round(precisePeriod)));
	const float rate = float(len) / precisePeriod;
	mRateInt = size_t(rate);
	mRateFrac = rate - float(mRateInt);
	mDelayLine.SetCount(len);
	generateExcitation(mDelayLine.AsRange(), damping, noise);

	mVolume = volume * scale;
	mExpStep = expCoeff == 0 ? 1.0f : Math::Exp(-expCoeff/float(sampleRate));
}

size_t KarplusStrongSampler::GenerateMono(Span<float> ioDst)
{
	const size_t n = ioDst.Length();
	float* dst = ioDst.Data();
	RenderInto(n, [dst](float v) mutable {*dst++ += v;});
	return n;
}

size_t KarplusStrongSampler::GenerateStereo(Span<float> ioDstLeft, Span<float> ioDstRight)
{
	const size_t n = Math::Min(ioDstLeft.Length(), ioDstRight.Length());
	float* dstL = ioDstLeft.Data();
	float* dstR = ioDstRight.Data();
	RenderInto(n, [dstL, dstR](float v) mutable {*dstL++ += v; *dstR++ += v;});
	return n;
}

unsigned KarplusStrongSampler::randGen(float freq, float volume, unsigned sampleRate)
{
	return 1436491347u ^ unsigned(freq*1000) ^ unsigned(volume*349885300.0f) ^ sampleRate;
}

/// Port of web-midisynth's GenGuitarPeriod (with its final *5).
void KarplusStrongSampler::generateExcitation(Span<float> dst, float damping, Random::FastUniform<float>& noise)
{
	const size_t n = dst.Length();
	const float nf = float(n);
	for(size_t i = 0; i < n; i++)
	{
		const float fi = float(i);
		float sample = fi*(nf - fi) / (nf*nf*0.25f);
		sample = sample*(1.0f - sample)*sample*(nf*0.5f - fi) / (nf*0.5f);
		sample += noise.SignedNext() / (nf*4.0f) / (1.0f/(nf*2.0f) + damping);
		dst[i] = sample*5.0f;
	}
}

INTRA_WARNING_POP
