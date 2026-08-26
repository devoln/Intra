#pragma once

#include <Math/Math.h>
#include "Intra/Simd/Simd.h"

#include "Intra/Range/Span.h"
#include "Utils/FixedArray.h"
#include "Types.h"

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
// mAtk[p] — шаг per-partial разгона амплитуды струны после контакта:
// возбуждённые ударом партиалы стартуют с уровня gSeam (последний сэмпл
// буфера атаки) и дорастают до табличной амплитуды (gSeam→1), не
// возбуждённые (G≈0) — с нуля (0→1), всё за τ≈AttackT/k. В любом случае
// mAtk обнуляется на DecayOnset, чтобы не тянуть амплитуду вверх при
// затухании.
	FixedArray<float> mAtk;
// Биения унисона при voices==2: две расстроенные струны коллапсированы в
// ОДИН лейн на партиалу (вдвое меньше синусоид в горячем цикле — было
// 43.8× realtime на Chopin, стало ~70×). Амплитуда лейна уже включает
// g0+g1, а биение воспроизводится медленной огибающей
//   E(t) = sqrt(cos²(Δ·t) + r²·sin²(Δ·t)),  r = (g0−g1)/(g0+g1),
// Δ — половинная разность частот струн (растёт с номером партиалы k:
// Δ ≈ π·k·f0·(det1−det0)/sr — биения у k-й гармоники в k раз быстрее).
// Огибающая пересчитывается раз в блок (псевдоконстанта: mBeatE0/mBeatE1
// на границах блока), внутри блока — линейная интерполяция, поэтому на
// стыках блоков нет ступенек/щелчков. Точность: амплитуда суммы двух
// струн воспроизводится точно; отброшен только фазовый воббл
// |θ| ≤ atan(r) ≈ 10° (периодический, на слух незаметен). Для голосов 1/3
// (и любых не-2-голосных инструментов) mBeatOn=false — лейны как раньше.
	FixedArray<float> mBeatStep, mBeatPh;
	FixedArray<float> mBeatE0, mBeatE1;
	float mBeatR;
	bool mBeatOn;
	// Буфер атаки контактной силы: предвычисленный накопленный отклик мод
	// на удар молоточка (первые mAttackLen отсчётов ноты). Последний сэмпл
	// буфера равен первому сэмплу струны (состояние мод после контакта =
	// табличному состоянию), поэтому шов буфер→SIMD-рекурсия бесшовный.
	FixedArray<float> mAttackBuf;
	size_t mAttackLen;
	size_t mAttackPos;
	// «Рокот»: корпусные резонансы деки (78/116/168/285 Гц, τ≈45 мс),
	// возбуждённые той же контактной силой. У средних/верхних нот их
	// частоты ниже f0 струны, которых модальный банк не даёт, а в семплах
	// полоса 60-300 Гц на атаке всегда есть. Играется сразу после
	// attack-буфера и гаснет за ~0.12 с, в сустейн не входит.
	FixedArray<float> mBodyBuf;
	size_t mBodyLen;
	size_t mBodyPos;
	// «Удар» — фундаментальный транзиент: в семплах SF2 h1 (и слабее h2) на
	// атаке бьёт пиком на +3..+14 дБ выше сустейна (зависит от высоты; C7
	// +13.9 дБ) и гаснет за ~100 мс — плотный удар молоточка по струне, он
	// и даёт высоким нотам объём. Чисто-аддитивная струна его не даёт (моды
	// стартуют ровно по таблице), поэтому без него атака верхних нот звучит
	// «пищалкой»: h2/h1 ≈ 0 дБ вместо −10..−20 дБ у семпла. Транзиент —
	// оверлей (как «рокот»): фаза-выровненная сумма партиал h1/h2 × A1/A2 ×
	// огибающая (подъём τr≈14 мс, спад τd≈40 мс, норм. к пику 1), старт с
	// нуля — без щелчка; в сустейн не входит, при release гасится сразу.
	FixedArray<float> mPushBuf;
	size_t mPushLen;
	size_t mPushPos;
	// «Блум» сустейна — времязависимый тембр, которого нет у плоской таблицы:
	// в сырых семплах SF2 обертона h2–h3 держатся ПОВЫШЕННО первые ~0.1–0.7 с
	// сустейна (на D#5 h2 почти вровень с h1 на 0.1–0.3 с, к ~0.7 с сседает к
	// плоскому уровню), а наша плоская струна (Session 7) разница со семплом
	// по h2–h3 в первые полсекунды длинной ноты 4–10 дБ — это и звучит как
	// «бедно». Блум — третий оверлей (как рокот/удар): фаза-выровненная сумма
	// партиал h2–h3 × малый множитель × огибающая (0 до 45 мс — после атаки,
	// подъём τr≈55 мс, экспоненц. спад τd=0.20 с), стартует с нуля — без
	// щелчка; к ~0.75 с доходит до нуля и в поздний сустейн не входит.
	FixedArray<float> mBloomBuf;
	size_t mBloomLen;
	size_t mBloomPos;
	// Bloom включается только для региона 75 (остров D5–E5, см. ctor).
	bool mBloomOn;
// Фейд оверлеев (рокот+удар) при release: демпфер убивает и «удар в
	// воздухе», но плавно (τ≈8 мс), иначе стаккато на верхних нотах обрывало
	// h1-транзиент в +14 дБ щелчком. mOverlayGain умножается на mOverlayRel
	// каждый сэмпл, пока идут оверлеи.
	float mOverlayGain;
	float mOverlayRel;
	bool mOverlayActive;
	unsigned mSampleRate;
	// Скрэтч-аккумуляторы: 4 лейна на сэмпл блока.
	FixedArray<float> mScratch;
	size_t mCount;   // число осцилляторов (партиалы × струны, кратно 4)
	float mVolume;
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
	///   UnisonVoices — число «струн» на ноту (1..3);
	///   VelBrightness — чувствительность яркости к velocity (0..1);
	///   TrebleTilt — подавление обертонов с ростом высоты (0 = по семплу).
	///
	/// Атака — контактная сила: короткий sin²-импульс возбуждает те же
	/// моды, что звучат в сустейне (см. конструктор); отдельного слоя
	/// молоточка нет.
	AdditiveSampler(float freq, float volume, unsigned sampleRate,
		size_t maxPartials, float brightness, float scale, float decayScale,
		float decayStiffness, float detuneCents,
		int unisonVoices, float velBrightness, float trebleTilt);

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
	while(numSamples)
	{
		// Атака контактной силы: первые mAttackLen отсчётов — предвычисленный
		// накопленный отклик мод на удар молоточка (конструктор). Последний
		// сэмпл буфера равен первому сэмплу струны, поэтому переход в
		// SIMD-рекурсию ниже бесшовный. Затухание на этом отрезке не
		// переключается (контакт ~2 мс всегда раньше DecayOnset). Параллельно
		// играется «рокот» (корпус): mBodyBuf[0..contactN] добавляется к
		// атаке, остаток доигрывает поверх струны в свёртке ниже.
		while(mAttackPos < mAttackLen && numSamples)
		{
			const size_t t = mRendered++;
			float s = mAttackBuf[mAttackPos++];
			if(mOverlayActive)
			{
				if(mBodyPos < mBodyLen) s += mBodyBuf[mBodyPos++]*mOverlayGain;
				if(mPushPos < mPushLen) s += mPushBuf[mPushPos++]*mOverlayGain;
				if(mBloomOn && mBloomPos < mBloomLen) s += mBloomBuf[mBloomPos++];
				mOverlayGain *= mOverlayRel;
				if(mBodyPos >= mBodyLen && mPushPos >= mPushLen && (!mBloomOn || mBloomPos >= mBloomLen)) mOverlayActive = false;
			}
			if(mEndSamples)
			{
				if(t >= mEndSamples) s = 0.0f;
				else if(t + mFadeSamples >= mEndSamples)
					s *= float(mEndSamples - t)/float(mFadeSamples);
			}
			sink(s*vol);
			numSamples--;
		}
		if(numSamples == 0) break;
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
			// Огибающая биений (коллапс 2-струнного унисона): пересчёт раз в
			// блок на границах [ph, ph + step·n], внутри блока — линейная
			// интерполяция (gv = E0 + (E1−E0)·i/n), поэтому на стыках блоков
			// огибающая непрерывна (нет ступенек/щелчков).
			if(mBeatOn)
			{
				const float r2 = mBeatR*mBeatR;
				const float* bs = mBeatStep.Data();
				float* bp = mBeatPh.Data();
				float* e0 = mBeatE0.Data();
				float* e1 = mBeatE1.Data();
				for(size_t p = 0; p < count; p++)
				{
					const float ph = bp[p];
					const float s0 = Math::Sin(ph);
					e0[p] = Math::Sqrt(1.0f - (1.0f - r2)*(s0*s0));
					const float ph1 = ph + bs[p]*float(n);
					const float s1 = Math::Sin(ph1);
					e1[p] = Math::Sqrt(1.0f - (1.0f - r2)*(s1*s1));
					bp[p] = ph1;
				}
			}
#if INTRA_SIMD_SUPPORT >= INTRA_SIMD_SSE2
			if(mBeatOn)
			{
				const float* e0a = mBeatE0.Data();
				const float* e1a = mBeatE1.Data();
				const float invN = 1.0f/float(n);
				for(size_t p = 0; p < count; p += 4)
				{
					__m128 s1v = _mm_loadu_ps(s1+p);
					__m128 s2v = _mm_loadu_ps(s2+p);
					__m128 kv  = _mm_loadu_ps(k+p);
					__m128 av  = _mm_loadu_ps(amp+p);
					const __m128 dv = _mm_loadu_ps(dec+p);
					const __m128 ak = _mm_loadu_ps(atk+p);
					const __m128 mv = _mm_sub_ps(dv, ak);
					const __m128 b0 = _mm_loadu_ps(e0a + p);
					const __m128 bd = _mm_sub_ps(_mm_loadu_ps(e1a + p), b0);
					for(size_t i = 0; i < n; i++)
					{
						const __m128 gv = _mm_add_ps(b0, _mm_mul_ps(bd, _mm_set1_ps(float(i)*invN)));
						const __m128 out = _mm_mul_ps(_mm_mul_ps(s1v, av), gv);
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
			}
			else
			{
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
			}
#else
			if(mBeatOn)
			{
				const float* e0a = mBeatE0.Data();
				const float* e1a = mBeatE1.Data();
				const float invN = 1.0f/float(n);
				for(size_t p = 0; p < count; p++)
				{
					float s1v = s1[p], s2v = s2[p], kv = k[p];
					float av = amp[p];
					const float mv = dec[p] - atk[p];
					const float ak = atk[p];
					const float b0 = e0a[p], bd = e1a[p] - b0;
					for(size_t i = 0; i < n; i++)
					{
						const float out = s1v*av*(b0 + bd*(float(i)*invN));
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
			}
			else
			{
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
			}
#endif
			// Оверлеи («рокот» + «удар» + «блум») подмешиваются в аккумулятор
			// отдельным коротким циклом, пока они активны.
			if(mOverlayActive)
			{
				size_t ov = 0;
				while(ov < n && (mBodyPos < mBodyLen || mPushPos < mPushLen || (mBloomOn && mBloomPos < mBloomLen)))
				{
					float o = 0.0f;
					if(mBodyPos < mBodyLen) o += mBodyBuf[mBodyPos++]*mOverlayGain;
					if(mPushPos < mPushLen) o += mPushBuf[mPushPos++]*mOverlayGain;
					if(mBloomOn && mBloomPos < mBloomLen) o += mBloomBuf[mBloomPos++];
					mOverlayGain *= mOverlayRel;
					acc[4*ov] += o;
					ov++;
				}
				if(mBodyPos >= mBodyLen && mPushPos >= mPushLen && (!mBloomOn || mBloomPos >= mBloomLen)) mOverlayActive = false;
			}
			// Свёртка 4 лейнов.
			for(size_t i = 0; i < n; i++)
			{
				float s = (acc[4*i] + acc[4*i+1]) + (acc[4*i+2] + acc[4*i+3]);
				const size_t t = mRendered - n + i;
				// Конец региона: фейд на последних mFadeSamples, дальше тишина
				// (как fluidsynth без лупа — нота заканчивается вместе с семплом).
				if(mEndSamples)
				{
					if(t >= mEndSamples) s = 0.0f;
					else if(t + mFadeSamples >= mEndSamples)
						s *= float(mEndSamples - t)/float(mFadeSamples);
				}
				sink(s*vol);
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
///   UnisonVoices — число «струн» на ноту (1..3);
///   VelBrightness — чувствительность яркости к velocity (0..1);
///   TrebleTilt — подавление обертонов на высоких нотах (0 = по семплу).
struct AdditivePianoInstrument
{
	float Brightness;
	size_t MaxPartials;
	float Scale;
	float DecayScale;
	float DecayStiffness;
	float DetuneCents;
	int UnisonVoices;
	float VelBrightness;
	float TrebleTilt;

	GenericSamplerRef operator()(float freq, float volume, unsigned sampleRate) const
	{
		return new AdditiveSampler(freq, volume, sampleRate,
			MaxPartials, Brightness, Scale, DecayScale, DecayStiffness,
			DetuneCents, UnisonVoices, VelBrightness, TrebleTilt);
	}
};

INTRA_WARNING_POP
