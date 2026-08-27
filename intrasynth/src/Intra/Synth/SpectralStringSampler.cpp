#include "SpectralStringSampler.h"

#include <Audio/AudioProcessing.h>
#include <Range/Mutation/Fill.h>

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

SpectralStringSampler::SpectralStringSampler(float freq, float volume, unsigned sampleRate,
	float damping, float smoothFactorBase, float smoothFactorMul, float smoothFactorExp,
	float brightness, float stiffness, float scale, float expCoeff, float cutoffRatio):
	mPos(0), mHavePrev(false)
{
	Random::FastUniform<float> noise(randGen(freq, volume, sampleRate));

	// Та же формула сглаживания, что у KS (web-midisynth): per-sample smoothing
	// s. Brightness ослабляет демпфирование: s_eff = Damping·(1 - Brightness).
	const float note = Math::Log(freq / 16.352f) / Math::Log(2.0f) * 12.0f;
	const float noteFactor = Math::Max(0.0f, note / 128.0f);
	const float smoothFactor = smoothFactorBase *
		(0.9f - Math::Pow(noteFactor, smoothFactorExp)*smoothFactorMul + noise()*0.1f);
	const float s = Math::Max(0.0f, damping*(1.0f - brightness));

	const float precisePeriod = float(sampleRate) / freq - smoothFactor;
	const size_t period = Math::Max(size_t(16), size_t(Math::Round(precisePeriod)));
	mN = Math::CeilToNextPow2(uint(period));
	mHalf = mN / 2;
	mPeriod = period;
	// Читаем форму [0, period) — линию задержки KS — со скоростью
	// period/precisePeriod ≈ 1. Хвост [period, N) IFFT-буфера отбрасывается
	// (иначе форма = плектр + нули, и каждый выходной период состоит из
	// сжатого плектра и паузы — см. комментарий в RenderInto).
	mRate = float(period) / precisePeriod;
	// Кроссфейд: ~4% периода, в разумных пределах.
	mCrossfade = Math::Clamp(float(period)*0.04f, 8.0f, 64.0f);

	// Возмущение: тот же плектр, что у KS (genGuitarPeriod), длиной ровно в
	// один период; остаток до N дополнен нулями (хвост плектра ~0, так что
	// форма периода 0 практически непрерывна).
	FixedArray<float> excitation;
	excitation.SetCount(mN);
	FillZeros(excitation.AsRange());
	generateExcitation(excitation.AsRange().Take(period), damping, noise);

	// Спектр возмущения.
	mSpecRe.SetCount(mN);
	mSpecIm.SetCount(mN);
	for(size_t i = 0; i < mN; i++) mSpecRe[i] = excitation[i];
	FillZeros(mSpecIm.AsRange());
	Audio::InplaceFFT(mSpecRe.AsRange(), mSpecIm.AsRange());

	// Срез спектра выше cutoffRatio·Nyquist (эмуляция звукоснимателя):
	// 4-й порядок, гладкий. У перегруженных гитар убирает алиасинг драйва
	// (продукты waveshaper'а не выше ~2·fc) и белую шумовую "пудру" плектра,
	// которая после клипа слышна как физа/шум поверх струны.
	if(cutoffRatio > 0)
	{
		const float fc = cutoffRatio*float(sampleRate)*0.5f;
		for(size_t k = 0; k <= mHalf; k++)
		{
			const float f = float(k)*float(sampleRate)/float(mN);
			const float g = 1.0f/(1.0f + Math::Pow(f/fc, 4.0f));
			mSpecRe[k] *= g;
			mSpecIm[k] *= g;
			if(k > 0) { mSpecRe[mN-k] *= g; mSpecIm[mN-k] *= g; }
		}
	}

	// Коэффициенты затухания на период. Зеркальные бины (mN-k) получают то же
	// значение: |H(e^{-jω})| = |H(e^{jω})|, и форма периода должна оставаться
	// вещественной.
	mDecay.SetCount(mN);
	const float twoPi = 2.0f*float(Math::PI);
	if(stiffness > 0)
	{
		// Закон жёсткой струны: g_k = exp(-stiffness·k²·P'/sr).
		const float expScale = -stiffness*precisePeriod/float(sampleRate);
		for(size_t k = 0; k <= mHalf; k++)
		{
			const float g = Math::Exp(expScale*float(k*k));
			mDecay[k] = g;
			if(k > 0) mDecay[mN - k] = g;
		}
	}
	else
	{
		// KS-закон: g_k = |(1-s) + s·e^{-j2πk/N}|^{P'} — в точности кривая
		// затухания гармоник KS (one-pole фильтр, домноженный на период).
		const float k1 = 1.0f - s, k2 = s;
		for(size_t k = 0; k <= mHalf; k++)
		{
			const float w = twoPi*float(k)/float(mN);
			const float re = k1 + k2*Math::Cos(w);
			const float im = -k2*Math::Sin(w);
			const float g = Math::Pow(Math::Sqrt(re*re + im*im), precisePeriod);
			mDecay[k] = g;
			if(k > 0) mDecay[mN - k] = g;
		}
	}

	// Форма периода 0 = обратный FFT спектра плектра = сам плектр (round-trip,
	// InplaceInverseFFT нормализует делением на N). Кладём её в оба буфера:
	// на первом периоде кроссфейда нет (mHavePrev == false), а при первом
	// advancePeriod() Cpp::Swap сделает её «предыдущей».
	mWaveA.SetCount(mN);
	mWaveB.SetCount(mN);
	mFftRe.SetCount(mN);
	mFftIm.SetCount(mN);
	for(size_t i = 0; i < mN; i++) { mFftRe[i] = mSpecRe[i]; mFftIm[i] = mSpecIm[i]; }
	Audio::InplaceInverseFFT(mFftRe.AsRange(), mFftIm.AsRange());
	for(size_t i = 0; i < mN; i++) { mWaveA[i] = mFftRe[i]; mWaveB[i] = mFftRe[i]; }

	mVolume = volume * scale;
	mExpStep = expCoeff == 0 ? 1.0f : Math::Exp(-expCoeff/float(sampleRate));
}

void SpectralStringSampler::advancePeriod()
{
	const size_t n = mN;
	for(size_t k = 0; k < n; k++)
	{
		mSpecRe[k] *= mDecay[k];
		mSpecIm[k] *= mDecay[k];
	}
	for(size_t k = 0; k < n; k++)
	{
		mFftRe[k] = mSpecRe[k];
		mFftIm[k] = mSpecIm[k];
	}
	Audio::InplaceInverseFFT(mFftRe.AsRange(), mFftIm.AsRange());
	// Пинг-понг: старый текущий период становится предыдущим (хвост для
	// кроссфейда), новый записывается в освободившийся буфер.
	Cpp::Swap(mWaveA, mWaveB);
	for(size_t k = 0; k < n; k++) mWaveB[k] = mFftRe[k];
}

size_t SpectralStringSampler::GenerateMono(Span<float> ioDst)
{
	const size_t n = ioDst.Length();
	float* dst = ioDst.Data();
	RenderInto(n, [dst](float v) mutable {*dst++ += v;});
	return n;
}

size_t SpectralStringSampler::GenerateStereo(Span<float> ioDstLeft, Span<float> ioDstRight)
{
	const size_t n = Math::Min(ioDstLeft.Length(), ioDstRight.Length());
	float* dstL = ioDstLeft.Data();
	float* dstR = ioDstRight.Data();
	RenderInto(n, [dstL, dstR](float v) mutable {*dstL++ += v; *dstR++ += v;});
	return n;
}

unsigned SpectralStringSampler::randGen(float freq, float volume, unsigned sampleRate)
{
	return 1436491347u ^ unsigned(freq*1000) ^ unsigned(volume*349885300.0f) ^ sampleRate;
}

/// Port of web-midisynth's GenGuitarPeriod (with its final *5), identical to
/// KarplusStrongSampler's excitation: the same initial pluck for comparison.
void SpectralStringSampler::generateExcitation(Span<float> dst, float damping, Random::FastUniform<float>& noise)
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
