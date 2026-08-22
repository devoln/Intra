#pragma once

#include <Math/Math.h>
#include "Intra/Simd/Simd.h"

#include "Intra/Range/Span.h"
#include "Utils/FixedArray.h"
#include "Types.h"
#include "PianoEnvelope.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

/// Аддитивный семплер: сумма N независимых SineRange-осцилляторов (рекурсия
/// s2 = 2·cos(dphi)·s1 − s0, по 1 FMA на партиал на семпл). Партиалы берутся
/// из таблицы PianoRegions.h — реального пианино (Clavinova Grand), измерены
/// из семплов: амплитуда/фаза (окно атаки), частота (измеренный ratio —
/// растяжка/негармоничность), затухание (2-скоростное: быстрый начальный
/// спад λ1 + хвост λ2, по факту семпла).
///
/// Поверх таблицы — три физических механизма настоящего рояля:
///   - Атака per-partial: фундаментал нарастает медленно (τ≈AttackT), верхние
///     обертоны быстрее (τ≈AttackT/k) — чёткий ударный транзиент;
///   - Унисон: 1-3 «струны» на ноту с расстройкой DetuneCents (биения);
///   - Velocity→яркость: громче играешь — ярче тембр (слои SF2).
///
/// Горячий цикл (шаблон ниже) инстанцируется в AdditiveSampler.cpp, который
/// в size-сборках компилируется -O3 без LTO. По 4 партиалы за раз через
/// SIMD128 (wasm) / SSE2 (x86), скалярный фолбэк для сборок без SIMD.
/// Партиалы обрабатываются блоками: состояние осцилляторов живёт в регистрах
/// на весь блок, аккумуляторы — 4 лейна на сэмпл, горизонтальная свёртка —
/// один раз в конце блока.
class AdditiveSampler: public IGenericSampler
{
	// Состояние рекурсии SineRange: mS1[p], mS2[p] — два последних сэмпла
	// (амплитуда уже внутри: s1_0 = a·sin(phi)), mK[p] = 2·cos(dphi_p).
	FixedArray<float> mS1, mS2, mK;
	// mAmp[p] — множитель огибающей: 4-скоростное затухание по факту семпла
	// (быстрый начальный спад λ1 → средний λ2 → поздний средний λ3 → медленный
	// хвост λ4; затухание реальных семплов гнётся непрерывно — 3-скоростная
	// модель переключалась на хвост уже в 0.9с, а семпл держит быстрый спад
	// до ~1.8с, и хвост «пересиживался» на 10-25 дБ). Затухание начинается
	// после атаки с DecayOnset. mDecay[p] — активный шаг (1.0 до onset),
	// mDecay1/2/3/4 — шаги сегментов; переключение выполняется за O(count)
	// на точной границе сэмпла.
	FixedArray<float> mAmp, mDecay, mDecay1, mDecay2, mDecay3, mDecay4;
// mDecayRelease[p] — шаг затухания при отпускании клавиши (демпфер):
// быстрее обычного, с зависимостью от k (высокие гармоники гаснут
// быстрее, как в реальном пианино). Применяется через dec[p] в
// hot loop — никаких доп. вычислений на сэмпл.
	FixedArray<float> mDecayRelease;
// mAtk[p] — шаг per-partial разгона (0→1 за τ≈AttackT/k): фундаментал
// нарастает медленно, верхние обертоны быстрее; в любом случае обнуляется
// на DecayOnset, чтобы не тянуть амплитуду вверх во время затухания.
	FixedArray<float> mAtk;
	// Скрэтч-аккумуляторы: 4 лейна на сэмпл блока.
	FixedArray<float> mScratch;
	// «Молоточек»: короткий глухой удар (затухающий низкочастотный тон на
	// f0/2 + f0, ~10-40 мс) в начале ноты — не шум, а «глухой удар».
	FixedArray<float> mHammerNoise;
	float mHammerAmp;
	float mHammerDecay;
	size_t mHammerPos;
	// «Стрик» — удар по фундаменталу (region root 75, зона D5–E5): короткий
	// тон на f0 с τ≈15 мс. Измерено по семплу 75(L): H1 в первые 5–20 мс на
	// +13 дБ выше steady и гаснет за ~35 мс — глухой низкий «тук», которого
	// нет у соседних регионов (72/78: только +1..+5 дБ). Без него D5–E5
	// звучат ярче/«писклявее» семпла.
	FixedArray<float> mStrikeNoise;
	float mStrikeAmp;
	size_t mStrikePos;
	size_t mCount;   // число осцилляторов (партиалы × струны, кратно 4)
	float mVolume;
	float mExpStep;  // глобальное экспоненциальное затухание (не используется)
	// Переключение затухания: старт на DecayOnset (1.0 → λ1), λ1 → λ2 на
	// DecayOnset+SegT, λ2 → λ3 на DecayOnset+SegT+SegT2, λ3 → λ4 на
	// DecayOnset+SegT+SegT2+SegT3 (границы — окна измерений, из таблицы).
	size_t mDecayOnsetSamples;
	size_t mSegSamples;
	size_t mSegSamples2;
	size_t mSegSamples3;
	size_t mRendered;
	bool mDecayStarted;
	bool mSegSwitched;
	bool mSegSwitched2;
	bool mSegSwitched3;
	// Конец ноты: SF2 без лупа — fluidsynth играет семпл один раз, нота
	// заканчивается на SampleLen региона (масштабировано транспозицией: семпл,
	// сыгранный в 2 раза быстрее, в 2 раза короче). Перед концом — короткий
	// фейд (иначе щелчок); после — тишина.
	size_t mEndSamples;
	size_t mFadeSamples;
	// Стерео: панорамирование голосов унисона. При StereoPan > 0 голос 0
	// (первая струна) панорамируется влево, голос 1 — вправо. Pan = 0
	// означает моно (L=R). Измерено по разнице уровней L/R семплов SF2:
	// tanh(dB/6) даёт мягкую S-кривую в диапазоне [-1; +1].
	float mStereoPan;
	float mStereoGainL;
	float mStereoGainR;
	// Число партиал на голос (для стерео-разделения голосов в GenerateStereo).
	size_t mPartialsPerVoice;
	// Correction curve measured from the AcousticPiano SF2 roots. It is enabled
	// only for the AcousticPiano parameter set; other additive instruments keep
	// their own envelopes unchanged.
	float mEnvelopeGain[PianoEnvelopePointCount];
	float mEnvelopeTimeScale;
	float mEnvelopeLevel;
	size_t mEnvelopeSegment;
	bool mUseEnvelopeCorrection;
	// mDone = true когда нота полностью закончилась (mRendered >= mEndSamples):
	// GenerateMono/GenerateStereo возвращают 0, и NoteSampler удаляет голос.
	bool mDone;
// mReleased = true после NoteRelease(): dec[p] уже переключён на
// mDecayRelease, mEndSamples не используется для fade.
	bool mReleased;

public:
	static const size_t mBlockSize = 512;

	///   MaxPartials — верхняя граница числа партиал (режется также по Найквисту
	///     с запасом 8%);
	///   Brightness — 0..1, при > 0.25 слегка усиливает верхние партиалы
	///     (a_k ·= k^(0.8·(Brightness−0.25))); 0.25 — нейтрально;
	///   Scale — пик суммы партиал (нормируется синтезом одного периода);
	///   DecayScale — множитель на измеренное затухание (1 = как в семпле,
	///     <1 — длиннее, >1 — короче);
	///   DecayStiffness — «жёсткость»: λ_k ·= (1 + c·k²) (0 = по семплу);
	///   DetuneCents — расстройка унисона (полный разброс, 0 = одна струна);
	///   AttackBoost — зарезервировано (0), атака — рамп по AttackT из таблицы;
	///   UnisonVoices — число «струн» на ноту (1..3);
	///   VelBrightness — чувствительность яркости к velocity (0..1);
	///   HammerLevel — сила глухого удара молоточка (0 = нет; короткий
	///     низкочастотный всплеск на f0/2+f0, как измерено по атаке семплов);
	///   TrebleTilt — подавление обертонов с ростом высоты (0 = по семплу).
	AdditiveSampler(float freq, float volume, unsigned sampleRate,
		size_t maxPartials, float brightness, float scale, float decayScale,
		float decayStiffness, float detuneCents, float attackBoost,
		int unisonVoices, float velBrightness, float hammerLevel,
		float trebleTilt);

	/// Рендерит numSamples отсчётов. Лямбда-sink — как у KS/SpectralString:
	/// на wasm поинтер-инкремент в лямбде даёт лучший код, чем индексная
	/// запись. Сумма партиал копится в 4 лейна на сэмпл блока; после блока
	/// лейны сворачиваются и уходят в sink.
	template<typename TSink> void RenderInto(size_t numSamples, TSink&& sink)
	{
		const size_t count = mCount;
		float* s1 = mS1.Data();
		float* s2 = mS2.Data();
		float* k = mK.Data();
		float* amp = mAmp.Data();
		float* dec = mDecay.Data();
		float* atk = mAtk.Data();
		float vol = mVolume;
		const float expStep = mExpStep;
		while(numSamples)
		{
			size_t n = Math::Min(mBlockSize, numSamples);
			// Переключение на следующий сегмент затухания: старт на DecayOnset,
			// затем λ1 → λ2 на SegT, λ2 → λ3 на SegT2 (границы из таблицы).
			// При release (демпфере) эти переключения пропускаются — dec[p]
			// уже установлен на mDecayRelease в NoteRelease().
			if(!mReleased && !mDecayStarted && mRendered >= mDecayOnsetSamples)
			{
				float* dec1 = mDecay1.Data();
				for(size_t p = 0; p < count; p++) { dec[p] = dec1[p]; atk[p] = 0.0f; }
				mDecayStarted = true;
			}
			if(!mReleased && !mSegSwitched && mRendered >= mSegSamples)
			{
				float* dec2 = mDecay2.Data();
				for(size_t p = 0; p < count; p++) dec[p] = dec2[p];
				mSegSwitched = true;
			}
			if(!mReleased && !mSegSwitched2 && mRendered >= mSegSamples2)
			{
				float* dec3 = mDecay3.Data();
				for(size_t p = 0; p < count; p++) dec[p] = dec3[p];
				mSegSwitched2 = true;
			}
			if(!mReleased && !mSegSwitched3 && mRendered >= mSegSamples3)
			{
				float* dec4 = mDecay4.Data();
				for(size_t p = 0; p < count; p++) dec[p] = dec4[p];
				mSegSwitched3 = true;
			}
			// Не пересекать ближайшую границу внутри блока: следующий проход
			// применит новый шаг с точного сэмпла границы. При release — без
			// границ (демпфер не переключается).
			if(!mReleased)
			{
				if(!mDecayStarted) n = Math::Min(n, mDecayOnsetSamples - mRendered);
				else if(!mSegSwitched) n = Math::Min(n, mSegSamples - mRendered);
				else if(!mSegSwitched2) n = Math::Min(n, mSegSamples2 - mRendered);
				else if(!mSegSwitched3) n = Math::Min(n, mSegSamples3 - mRendered);
			}
			if(n == 0) continue;
			mRendered += n;
			float* acc = mScratch.Data();
			for(size_t i = 0; i < 4*n; i++) acc[i] = 0;
#if INTRA_SIMD_SUPPORT >= INTRA_SIMD_SSE2
			for(size_t p = 0; p < count; p += 4)
			{
				__m128 s1v = _mm_loadu_ps(s1+p);
				__m128 s2v = _mm_loadu_ps(s2+p);
				__m128 kv  = _mm_loadu_ps(k+p);
				__m128 av  = _mm_loadu_ps(amp+p);
				const __m128 dv = _mm_loadu_ps(dec+p);
				const __m128 ak = _mm_loadu_ps(atk+p);
				const __m128 mv = _mm_sub_ps(dv, ak);
				for(size_t i = 0; i < n; i++)
				{
					const __m128 out = _mm_mul_ps(s1v, av);
					const __m128 newS = _mm_sub_ps(_mm_mul_ps(kv, s2v), s1v);
					s1v = s2v;
					s2v = newS;
					av = _mm_add_ps(_mm_mul_ps(av, mv), ak);
					const __m128 a = _mm_loadu_ps(acc + 4*i);
					_mm_storeu_ps(acc + 4*i, _mm_add_ps(a, out));
				}
				_mm_storeu_ps(s1+p, s1v);
				_mm_storeu_ps(s2+p, s2v);
				_mm_storeu_ps(amp+p, av);
			}
#else
			for(size_t p = 0; p < count; p++)
			{
				float s1v = s1[p], s2v = s2[p], kv = k[p];
				float av = amp[p];
				const float mv = dec[p] - atk[p];
				const float ak = atk[p];
				for(size_t i = 0; i < n; i++)
				{
					const float out = s1v*av;
					const float newS = kv*s2v - s1v;
					s1v = s2v;
					s2v = newS;
					av = av*mv + ak;
					acc[4*i] += out;
				}
				s1[p] = s1v;
				s2[p] = s2v;
				amp[p] = av;
			}
#endif
			// Свёртка 4 лейнов + молоточек + огибающая. Разгон синусоид (если
			// нужен для этого регистра) уже внутри hot-лупа; верхний регистр
			// стартует сразу, без вязкого ramp от нуля.
			for(size_t i = 0; i < n; i++)
			{
				float s = (acc[4*i] + acc[4*i+1]) + (acc[4*i+2] + acc[4*i+3]);
				const size_t t = mRendered - n + i;
				if(mUseEnvelopeCorrection)
				{
					const float refTime = float(t)*mEnvelopeTimeScale;
					while(mEnvelopeSegment + 1 < PianoEnvelopePointCount - 1
						&& refTime > PianoEnvelopeTimes[mEnvelopeSegment + 1])
						mEnvelopeSegment++;
					float gain = mEnvelopeGain[PianoEnvelopePointCount - 1];
					if(refTime <= PianoEnvelopeTimes[0]) gain = mEnvelopeGain[0];
					else if(refTime < PianoEnvelopeTimes[PianoEnvelopePointCount - 1])
					{
						const size_t j = mEnvelopeSegment;
						const float u = (refTime - PianoEnvelopeTimes[j])
							/(PianoEnvelopeTimes[j + 1] - PianoEnvelopeTimes[j]);
						gain = mEnvelopeGain[j]*(1.0f - u) + mEnvelopeGain[j + 1]*u;
					}
					s *= gain*mEnvelopeLevel;
				}
				// Молоточек добавляется ПОСЛЕ коррекции огибающей: кривая коррекции
				// измерена по синусоидам (target/current по RMS), а удар — отдельный
				// перкуссионный шумовой компонент (беспитичевый), который не
				// проходит через строковую огибающую. У подогнанного инструмента он
				// выключен; у остальных добавляется поверх уже скорректированного
				// сигнала.
				if(mHammerPos < mHammerNoise.Length() && mHammerAmp > 1e-5f)
				{
					s += mHammerNoise[mHammerPos++]*mHammerAmp;
					mHammerAmp *= mHammerDecay;
				}
				// Стрик по фундаменталу (только region root 75) — после молоточка,
				// поверх скорректированного сигнала, как и молоточек.
				if(mStrikePos < mStrikeNoise.Length() && mStrikeAmp > 1e-5f)
				{
					s += mStrikeNoise[mStrikePos++]*mStrikeAmp;
				}
				// Конец семпла: фейд на последних mFadeSamples, дальше тишина
				// (как fluidsynth без лупа — нота заканчивается вместе с семплом).
				if(mEndSamples)
				{
					if(t >= mEndSamples) s = 0.0f;
					else if(t + mFadeSamples >= mEndSamples)
						s *= float(mEndSamples - t)/float(mFadeSamples);
				}
				sink(s*vol);
				vol *= expStep;
			}
			numSamples -= n;
		}
		mVolume = vol;
		if(mEndSamples && mRendered >= mEndSamples) mDone = true;
		else if(mReleased)
		{
			// При release (демпфере) нота заканчивается, когда все amp ~ 0.
			// Проверяем max |amp| только в конце блока.
			float maxAmp = 0.0f;
			for(size_t p = 0; p < count; p++)
			{
				const float a = amp[p] < 0.0f ? -amp[p] : amp[p];
				if(a > maxAmp) maxAmp = a;
			}
			// Порог −60 дБ (−100 дБ раньше): ниже уже не слышно в любой смеси,
			// а голос с τ=280 мс добирался бы до 1e-5 ~2.6 с — рендер после
			// release тормозил в разы (живые «хвосты» копились на каждой ноте).
			if(maxAmp < 1e-3f) mDone = true;
		}
	}

	size_t GenerateMono(Span<float> ioDst, Span<float> ioDstReverb) override;
	size_t GenerateStereo(Span<float> ioDstLeft, Span<float> ioDstRight, Span<float> ioDstReverb) override;
	void NoteRelease() override;
};

/// Фабрика аддитивного фортепиано.
///   Brightness — 0..1, > 0.25 усиливает верхние партиалы (ярче тембр);
///   MaxPartials — верхняя граница числа партиал;
///   Scale — пик нормировки (общая громкость инструмента);
///   DecayScale — множитель сустейн-затухания (1 = семпл, <1 длиннее);
///   DecayStiffness — «жёсткость»: верха гаснут быстрее (0 = по семплу);
///   DetuneCents — расстройка унисона (биения; honky-tonk — широкая);
///   AttackBoost — зарезервировано (0), атака по семплу;
///   UnisonVoices — число «струн» на ноту (1..3);
///   VelBrightness — чувствительность яркости к velocity (0..1);
///   HammerLevel — сила удара молоточка (0 = нет);
///   TrebleTilt — подавление обертонов на высоких нотах (0 = по семплу).
struct AdditivePianoInstrument
{
	float Brightness;
	size_t MaxPartials;
	float Scale;
	float DecayScale;
	float DecayStiffness;
	float DetuneCents;
	float AttackBoost;
	int UnisonVoices;
	float VelBrightness;
	float HammerLevel;
	float TrebleTilt;

	GenericSamplerRef operator()(float freq, float volume, unsigned sampleRate) const
	{
		return new AdditiveSampler(freq, volume, sampleRate,
			MaxPartials, Brightness, Scale, DecayScale, DecayStiffness,
			DetuneCents, AttackBoost, UnisonVoices, VelBrightness, HammerLevel,
			TrebleTilt);
	}
};

INTRA_WARNING_POP
