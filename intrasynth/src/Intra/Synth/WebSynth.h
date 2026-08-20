#pragma once

// JS-faithful building blocks ported from devoln/web-midisynth (instruments.js,
// wavegen.js, utils.js). The wavetable generator, envelope mapping, noise
// sampler and the two time-varying one-pole filters reproduce the web
// algorithm so the C++ version sounds identical.

#include <Cpp/Warnings.h>

#include <Math/Math.h>
#include <Random/FastUniform.h>

#include <Container/Sequential/Array.h>

#include <Range/Mutation/Copy.h>
#include <Range/Mutation/Transform.h>

#include "WaveTable.h"
#include "WaveTableGeneration.h"

#ifdef INTRA_PROBE_NAN
#include <stdio.h>
#endif
#include "WaveTableSampler.h"
#include "Envelope.h"
#include "Types.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

// ---------------------------------------------------------------------------
// Wavetable generator (WaveTableGeneratorFromHarmonics + AddSineHarmonicGauss)
// ---------------------------------------------------------------------------

struct WebHarmonicDesc { float Amplitude; float FreqMultiplier; float Bandwidth; };
struct WebResonanceDesc { float Frequency; float Width; float Amplitude; };
struct WebHarmonicSet
{
	Array<WebHarmonicDesc> Harmonics;
	Array<WebResonanceDesc> Resonances;
	bool IsResonanceMultiplicative = false;
};

// JS AddSineHarmonic: index = round(ratio*N); dstAmpls[index] += amplitude*N.
inline void WebAddSineHarmonic(Span<float> dstAmpls, float ratio, float amplitude)
{
	const size_t N = dstAmpls.Length()*2;
	const size_t index = size_t(Math::Round(ratio*float(N)));
	if(index >= dstAmpls.Length()) return;
	dstAmpls[index] += amplitude*float(N);
}

// JS AddSineHarmonicGauss. Unlike the old AddSineHarmonicGaussianProfile this
// does not scale the gaussian by the harmonic index and integrates the exact
// JS amplitude chain (amplitude*N*sqrt(pi)/2 with erf integration).
noinline void WebAddSineHarmonicGauss(Span<float> dstAmpls, float ratio, float baseRatio, float amplitude, float bandwidthCents)
{
	const size_t N = dstAmpls.Length()*2;
	double bwi = (Math::Pow2(bandwidthCents/1200.0f - 1.0f) - 0.5)*baseRatio;
	if(bwi < 1e-10)
	{
		WebAddSineHarmonic(dstAmpls, ratio, amplitude);
		return;
	}

	double rw = -ratio/bwi;
	const double rdw = 1.0/(double(N)*bwi);

	double range = 2;
	if(rdw > 1) range = 3*rdw;
	if(-range > rw)
	{
		const double elementsToSkip = Math::Floor((-range - rw)/rdw);
		dstAmpls.PopFirstN(size_t(elementsToSkip));
		rw += elementsToSkip*rdw;
	}
	if(rw < range) dstAmpls = dstAmpls.Take(size_t(Math::Ceil((range - rw)/rdw)));

	const double A = amplitude*double(N)*(Math::SqrtPI/2);
	double erf = Math::Erf(rw);
	while(!dstAmpls.Empty())
	{
		rw += rdw;
		const double erfNext = Math::Erf(rw);
		dstAmpls.Next() += float(A*(erfNext - erf));
		erf = erfNext;
	}
}

// Port of WaveTableGeneratorFromHarmonics. The resulting table is NOT peak
// normalized (JS uses InplaceInverseFFTNonNormalized); relative loudness is
// controlled by each instrument's Volume.
noinline WaveTableCache CreateWebWaveTables(const Array<WebHarmonicSet>& sets, size_t tableSize)
{
	WaveTableCache result;
	Array<WebHarmonicSet> setsCopy = sets;
	result.Generator = [setsCopy, tableSize](float freq, unsigned sampleRate)
	{
		WaveTable tbl;
		tbl.BaseLevelLength = tableSize;
		tbl.Data.Reserve(tableSize*2);
		tbl.Data.SetCount(tableSize/2);
		tbl.BaseLevelRatio = freq/float(sampleRate);
		const float baseRatio = tbl.BaseLevelRatio;

		Array<WebHarmonicDesc> finalHarmonics;
		float amplSum = 0;
		for(const auto& set: setsCopy)
		{
			for(const auto& harm: set.Harmonics)
			{
				const float ifreq = harm.FreqMultiplier*freq;
				if(ifreq > float(sampleRate)/2.0f) continue; // harmonics sorted by frequency

				float amplitude = harm.Amplitude;
				if(!set.Resonances.Empty())
				{
					float res = 0;
					for(const auto& r: set.Resonances)
					{
						// Нулевая ширина резонанса даёт деление на ноль (x = ifreq/0 -> inf,
						// затем 0*exp(-inf)/0 -> NaN). Такие резонансы не имеют физического
						// смысла и пропускаются — это также защищает от NaN при повреждении
						// данных или при копировании без elision на -Oz/-Os сборках.
						if(r.Width <= 0.0f) continue;
						const float x = (ifreq - r.Frequency)/r.Width;
						res += r.Amplitude*Math::Exp(-0.5f*x*x)/(2.507f*r.Width);
					}
					if(set.IsResonanceMultiplicative) amplitude *= res;
					else amplitude += res;
				}
				finalHarmonics.AddLast(WebHarmonicDesc{amplitude, harm.FreqMultiplier, harm.Bandwidth});
				amplSum += amplitude;
			}
		}
		amplSum *= float(tableSize);
		for(const auto& h: finalHarmonics)
			WebAddSineHarmonicGauss(tbl.Data.AsRange(), baseRatio*h.FreqMultiplier, baseRatio, h.Amplitude/amplSum, h.Bandwidth);

		ConvertAmplitudesToSamplesUnnormalized(tbl, false);
		return tbl;
	};
	result.AllowMipmaps = false;
	return result;
}

// ---------------------------------------------------------------------------
// Envelope (Envelope.Segments/Volume/Exp/ExponentialInterpolation)
// ---------------------------------------------------------------------------

struct WebEnvelope
{
	float Attack = 0;       // Segments[0]
	float Decay = 0;        // Segments[1] (0 for 1-2 segment envelopes)
	float Sustain = 1;      // Volume[2] plateau
	float Release = 0;      // Segments[last]
	float Exp = 0;          // exponential decay coefficient (separate gain in JS)
	bool Exponential = false;
	bool StartsAtFull = false; // Volume[0] == 1 -> no attack ramp
};

noinline EnvelopeFactory MakeWebEnvelope(const WebEnvelope& e)
{
	EnvelopeFactory f = EnvelopeFactory::ADSR(
		e.StartsAtFull ? 0.0f : e.Attack,
		e.Decay, e.Sustain, e.Release, e.Exponential);
	if(e.StartsAtFull) f.StartVolume = 1.0f;
	return f;
}

// ---------------------------------------------------------------------------
// Noise sampler (NoiseWaveform: GenNoisePeriod + LowpassFilter at note freq)
// ---------------------------------------------------------------------------

class WebNoiseSampler: public IGenericSampler
{
	Array<float> mTable;
	size_t mPos = 0;
	float mVolume;

public:
	WebNoiseSampler(float freq, float volume, unsigned sampleRate, size_t tableSize, float scale, uint seed)
	{
		Random::FastUniform<float> noise(seed);
		mTable.SetCount(tableSize);
		for(size_t i = 0; i < tableSize; i++) mTable[i] = noise.SignedNext();

		// JS LowpassFilter(samples, freq/sampleRate): alpha = 2*pi*cutoffRatio.
		const float alpha = 2.0f*float(Math::PI)*freq/float(sampleRate);
		float prev = 0;
		for(size_t i = 0; i < tableSize; i++)
		{
			const float s = mTable[i];
			prev = s*alpha + prev*(1.0f - alpha);
			mTable[i] = prev;
		}
		mVolume = volume*scale;
	}

	size_t GenerateMono(Span<float> ioDst, Span<float> ioDstReverb) override
	{
		(void)ioDstReverb;
		const size_t L = mTable.Length();
		for(size_t i = 0; i < ioDst.Length(); i++)
		{
			ioDst[i] += mTable[mPos]*mVolume;
			mPos = mPos + 1 < L ? mPos + 1 : 0;
		}
		return ioDst.Length();
	}

	size_t GenerateStereo(Span<float> ioDst, Span<float> ioDstRight, Span<float> ioDstReverb) override
	{
		(void)ioDstReverb;
		const size_t n = Math::Min(ioDst.Length(), ioDstRight.Length());
		const size_t L = mTable.Length();
		for(size_t i = 0; i < n; i++)
		{
			const float s = mTable[mPos]*mVolume;
			ioDst[i] += s;
			ioDstRight[i] += s;
			mPos = mPos + 1 < L ? mPos + 1 : 0;
		}
		return n;
	}
};

struct WebNoiseInstrument
{
	size_t TableSize = 32768;
	float VolumeScale = 1;
	uint Seed = 157898685;

	GenericSamplerRef operator()(float freq, float volume, unsigned sampleRate) const
	{return new WebNoiseSampler(freq, volume, sampleRate, TableSize, VolumeScale, Seed);}
};

// ---------------------------------------------------------------------------
// Time-varying one-pole filters (ExpExp Filter and CutoffFreq approximation)
// out = in*mu + prev*(1-mu), with mu(t) piecewise linear.
// ---------------------------------------------------------------------------

class WebTimeVaryingLowpass
{
	float mPrev = 0;
	float mMu;
	float mDMu = 0;
	size_t mIndex = 0;
	size_t mStepLeft = 0;
	size_t mStep;
	CSpan<float> mDeltas;

public:
	WebTimeVaryingLowpass(CSpan<float> deltas, float startMu, size_t step):
		mMu(startMu), mStep(step), mDeltas(deltas) {}

	void operator()(Span<float> dst)
	{
		for(float& out: dst)
		{
			if(mStepLeft == 0)
			{
				mStepLeft = mStep;
				mIndex++;
				float dmu = 0;
				if(mIndex < mDeltas.Length())
				{
					dmu = mDeltas[mIndex];
					if(dmu < -mMu/float(mStep)) dmu = -mMu/float(mStep);
				}
				mDMu = dmu;
			}
			const float s = mPrev*(1.0f - mMu) + out*mMu;
			mPrev = s;
			out = s;
			mMu += mDMu;
			mStepLeft--;
		}
	}
};

// ExpExp (Envelope.ExpExpK / ExpExpBase): smoothing coefficient table that
// makes the sound mellow over time. Port of GenSmoothCoeffTableExpExp + Filter.
inline Array<float> GenExpExpDeltas(float base, float k, size_t len = 1024, size_t step = 256)
{
	Array<float> res;
	res.AddLast(1.0f);
	float amul = Math::Pow(1.0f - base, float(step));
	float a = k;
	const float emk = Math::Exp(-k);
	float prevmu = 1.0f;
	for(size_t i = 1; i < len; i++)
	{
		a *= amul;
		const float mu = Math::Exp(a)*emk;
		res.AddLast((mu - prevmu)/float(step));
		prevmu = mu;
	}
	return res;
}

struct WebExpExpModifierFactory
{
	Array<float> Deltas;
	size_t Step = 256;

	WebExpExpModifierFactory(decltype(nullptr)=nullptr) {}
	WebExpExpModifierFactory(float base, float k): Deltas(GenExpExpDeltas(base, k)) {}

	GenericModifier operator()(float, float, unsigned) const
	{
		return GenericModifier(WebTimeVaryingLowpass(Deltas.AsConstRange(), 1.0f, Step));
	}

	INTRA_FORCEINLINE explicit operator bool() const {return !Deltas.Empty();}
};

// CutoffFreq: lowpass whose cutoff ramps with the envelope. JS uses a biquad;
// we use a stable one-pole with alpha = 1 - exp(-2*pi*cutoff/sr). The release
// sweep is not modelled (the modifier has no note-off hook).
class WebCutoffFilter
{
	float mPrev = 0;
	float mAlpha = 0;
	float mDAlpha = 0;
	size_t mSamplesLeft = 0;
	float mEndAlphas[4];
	float mDurations[4];
	size_t mSegCount = 0;
	size_t mSegIndex = 0;

public:
	WebCutoffFilter(float startAlpha, float attackSamples, float a1, float decaySamples, float a2)
	{
		mAlpha = startAlpha;
		mEndAlphas[0] = a1; mDurations[0] = attackSamples;
		mEndAlphas[1] = a2; mDurations[1] = decaySamples;
		mEndAlphas[2] = a2; mDurations[2] = 1; // sustain hold
		mSegCount = attackSamples > 0 ? 3 : 2;
	}

	void operator()(Span<float> dst)
	{
		for(float& out: dst)
		{
			if(mSamplesLeft == 0)
			{
				if(mSegIndex >= mSegCount)
				{
					mDAlpha = 0; // hold cutoff after the last segment
				}
				else
				{
					const float dur = mDurations[mSegIndex];
					const float end = mEndAlphas[mSegIndex];
					mSegIndex++;
					mSamplesLeft = size_t(dur);
					mDAlpha = dur > 0 ? (end - mAlpha)/dur : 0;
				}
			}
			const float s = out*mAlpha + mPrev*(1.0f - mAlpha);
#ifdef INTRA_PROBE_NAN
			if(!(s > -1e12f && s < 1e12f))
			{
				static int probeLines = 0;
				if(probeLines < 10)
				{
					fprintf(stderr, "[CUTOFF] s=%.3e in=%.3e prev=%.3e alpha=%.3e dAlpha=%.3e segIdx=%zu segLeft=%zu\n",
						double(s), double(out), double(mPrev), double(mAlpha), double(mDAlpha), mSegIndex, mSamplesLeft);
					probeLines++;
				}
			}
#endif
			mPrev = s;
			out = s;
			mAlpha += mDAlpha;
			if(mSamplesLeft > 0) mSamplesLeft--;
		}
	}
};

struct WebCutoffFactory
{
	float Cutoffs[4] = {20000, 20000, 20000, 20000};
	WebEnvelope Envelope;

	WebCutoffFactory(decltype(nullptr)=nullptr) {}
	WebCutoffFactory(float c0, float c1, float c2, float c3, const WebEnvelope& env):
		Cutoffs{c0, c1, c2, c3}, Envelope(env) {}

	GenericModifier operator()(float, float, unsigned sampleRate) const
	{
		auto alpha = [sampleRate](float cutoff) -> float
		{
			const float f = Math::Max(cutoff, 1.0f);
			return 1.0f - Math::Exp(-2.0f*float(Math::PI)*f/float(sampleRate));
		};
		const float attackSamples = Math::Max(0.0f, Envelope.Attack*float(sampleRate));
		const float decaySamples = Math::Max(0.0f, Envelope.Decay*float(sampleRate));
		return GenericModifier(WebCutoffFilter(
			alpha(Cutoffs[0]), attackSamples, alpha(Cutoffs[1]), decaySamples, alpha(Cutoffs[2])));
	}

	INTRA_FORCEINLINE explicit operator bool() const {return false;}
};

INTRA_WARNING_POP
