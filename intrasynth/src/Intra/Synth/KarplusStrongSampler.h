#pragma once

#include <Math/Math.h>

#include "Intra/Range/Span.h"
#include "Utils/FixedArray.h"
#include "Random/FastUniform.h"
#include "Types.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

/// Karplus-Strong plucked-string sampler, ported verbatim from devoln/web-midisynth
/// (GenGuitarPeriod + FilterKS + GuitarWaveGenerator) and adapted to the Intra
/// IGenericSampler interface. The delay line is a circular buffer mutated in place
/// by the feedback loop, so this is a sequential kernel rather than a per-channel one.
///
/// The web implementation computes a frequency-dependent smoothing factor:
///   smoothFactor = base * (0.9 - pow(log2(freq/16.352)*12/128, exp)*mul + rand*0.1)
/// and a precise period `sampleRate/freq - smoothFactor`. The delay line length
/// is that period rounded to an integer, and the read position advances by
/// `length/precisePeriod` samples per output sample (web's `playbackSpeed`),
/// accumulated as int+frac without sample interpolation — exactly what web does
/// by resampling the loop buffer with AudioBufferSourceNode.playbackRate and
/// then running FilterKS over it sequentially.
///
/// The hot loop lives in KarplusStrongSampler.cpp so the size build can compile
/// it at -O3 while the cold InstrumentLibrary constructor stays at -Oz.
class KarplusStrongSampler: public IGenericSampler
{
	FixedArray<float> mDelayLine;
	// Позиция чтения строки: целая часть + дробный аккумулятор. Держим
	// позицию в целых, чтобы убрать float->int конверсию из горячей цепочки.
	size_t mPos;
	float mFrac;
	float mRateFrac;
	size_t mRateInt;
	float mPrevSample;
	float mSmoothFactor;
	// Текущая амплитуда ноты: base volume*scale, домноженный на каждом семпле
	// на mExpStep. Экспоненциальное затухание наложено прямо в цикл генерации
	// (как в web-midisynth Envelope.Exp), поэтому отдельный проход-модификатор
	// не нужен.
	float mVolume;
	float mExpStep;

public:
	KarplusStrongSampler(float freq, float volume, unsigned sampleRate,
		float damping, float smoothFactorBase, float smoothFactorMul, float smoothFactorExp,
		float scale, float expCoeff);

	/// Рендерит numSamples отсчётов строки Карплуса-Стронга в dst.
	/// Позиция хранится как (целая mPos, дробная mFrac): в горячей цепочке
	/// нет float->int конверсии, обёртка по периоду — редкая ветка.
	///
	/// Лямбда-sink принципиальна: на wasm поинтер-инкремент в лямбде
	/// генерирует заметно лучший код, чем индексная запись dst[k] в общем
	/// цикле (замерено на этой же сборке, см. коммит KS-переписывания).
	template<typename TSink> void RenderInto(size_t numSamples, TSink&& sink)
	{
		const size_t rateInt = mRateInt;
		const float rateFrac = mRateFrac;
		const size_t len = mDelayLine.Length();
		auto line = mDelayLine.AsRange();
		const float k1 = 1.0f - mSmoothFactor;
		const float k2 = mSmoothFactor;
		size_t i = mPos;
		float frac = mFrac;
		float prev = mPrevSample;
		float vol = mVolume;
		const float expStep = mExpStep;

		auto emit = [&]()
		{
			frac += rateFrac;
			if(frac >= 1.0f)
			{
				frac -= 1.0f;
				i += rateInt + 1;
			}
			else i += rateInt;
			// Обёртка по периоду: редкая и предсказуемая ветка.
			if(i >= len) i -= len;
			// Чтение без интерполяции, как в web-midisynth FilterKS: позиция
			// идёт по целым с шагом rate (playbackSpeed), а не lerp-ит между
			// соседними сэмплами. Lerp между line[i] и line[i+1] давал
			// завышение высоты на ~+7-9 центов и полностью разваливал петлю
			// на высоких нотах (rateInt=0: чтение убегало вперёд по ещё не
			// обработанным сэмплам, см. scripts/_tmp-ks-variants.js).
			const float out = line[i]*k1 + prev*k2;
			line[i] = out;
			prev = out;
			sink(out*vol);
			vol *= expStep;
		};

		size_t left = numSamples;
		while(left >= 2)
		{
			emit();
			emit();
			left -= 2;
		}
		if(left) emit();
		mPos = i;
		mFrac = frac;
		mPrevSample = prev;
		mVolume = vol;
	}

	size_t GenerateMono(Span<float> ioDst) override;
	size_t GenerateStereo(Span<float> ioDstLeft, Span<float> ioDstRight) override;

private:
	static unsigned randGen(float freq, float volume, unsigned sampleRate);
	static void generateExcitation(Span<float> dst, float damping, Random::FastUniform<float>& noise);
};

/// Instrument factory producing KarplusStrongSampler. Parameters are taken
/// verbatim from web-midisynth's KarplusStrong presets (Damping, SmoothFactor
/// formula and Volume). The exponential decay (web's Envelope.Exp) is applied
/// inside the sampler itself; the ADSR envelope lives on the NoteSampler.
struct KarplusStrongInstrument
{
	float Damping;
	float SmoothFactorBase;
	float SmoothFactorMul;
	float SmoothFactorExp;
	float Scale;
	float ExpCoeff;

	GenericSamplerRef operator()(float freq, float volume, unsigned sampleRate) const
	{
		return new KarplusStrongSampler(freq, volume, sampleRate,
			Damping, SmoothFactorBase, SmoothFactorMul, SmoothFactorExp, Scale, ExpCoeff);
	}
};

INTRA_WARNING_POP
