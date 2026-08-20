#include "GaussianStringSampler.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

GaussianStringSampler::GaussianStringSampler(float freq, float volume, unsigned sampleRate,
	float damping, float smoothFactorBase, float smoothFactorMul, float smoothFactorExp,
	float scale, float expCoeff, float radiusA, float radiusCap):
	mPos(0), mTime(0), mLastR(-1), mInvWidth(1.0f)
{
	Random::FastUniform<float> noise(randGen(freq, volume, sampleRate));

	const float note = Math::Log(freq / 16.352f) / Math::Log(2.0f) * 12.0f;
	const float noteFactor = Math::Max(0.0f, note / 128.0f);
	const float smoothFactor = smoothFactorBase *
		(0.9f - Math::Pow(noteFactor, smoothFactorExp)*smoothFactorMul + noise()*0.1f);

	const float precisePeriod = float(sampleRate) / freq - smoothFactor;
	mLen = Math::Max(size_t(1), size_t(Math::Round(precisePeriod)));
	mRate = float(mLen) / precisePeriod;
	mLine.SetCount(mLen);
	generateExcitation(mLine.AsRange(), damping, noise);

	mVolume = volume * scale;
	mExpStep = expCoeff == 0 ? 1.0f : Math::Exp(-expCoeff/float(sampleRate));
	mRadiusA = radiusA;
	mRadiusCap = radiusCap;
}

size_t GaussianStringSampler::GenerateMono(Span<float> ioDst, Span<float> ioDstReverb)
{
	(void)ioDstReverb;
	const size_t n = ioDst.Length();
	float* dst = ioDst.Data();
	RenderInto(n, [dst](float v) mutable {*dst++ += v;});
	return n;
}

size_t GaussianStringSampler::GenerateStereo(Span<float> ioDstLeft, Span<float> ioDstRight, Span<float> ioDstReverb)
{
	(void)ioDstReverb;
	const size_t n = Math::Min(ioDstLeft.Length(), ioDstRight.Length());
	float* dstL = ioDstLeft.Data();
	float* dstR = ioDstRight.Data();
	RenderInto(n, [dstL, dstR](float v) mutable {*dstL++ += v; *dstR++ += v;});
	return n;
}

unsigned GaussianStringSampler::randGen(float freq, float volume, unsigned sampleRate)
{
	return 1436491347u ^ unsigned(freq*1000) ^ unsigned(volume*349885300.0f) ^ sampleRate;
}

/// Port of web-midisynth's GenGuitarPeriod (with its final *5), identical to
/// KarplusStrongSampler's excitation: the same initial pluck for comparison.
void GaussianStringSampler::generateExcitation(Span<float> dst, float damping, Random::FastUniform<float>& noise)
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
