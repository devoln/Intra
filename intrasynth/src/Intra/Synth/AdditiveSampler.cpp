#include "AdditiveSampler.h"
#include "PianoRegions.h"

#include <Audio/AudioProcessing.h>
#include <Range/Mutation/Fill.h>
#include "Random/FastUniform.h"
INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

AdditiveSampler::AdditiveSampler(float freq, float volume, unsigned sampleRate,
	size_t maxPartials, float brightness, float scale, float decayScale,
	float decayStiffness, float detuneCents, float attackBoost,
	int unisonVoices, float velBrightness, float hammerLevel,
	float trebleTilt)
{
	(void)attackBoost; // атака задаётся таблицей региона; верхний регистр стартует сразу
	// Ближайший регион по MIDI-ноте (высота = равномерная темперация).
	const float midi = 69.0f + 12.0f*Math::Log(freq/440.0f)/0.6931471805599453f;
	size_t best = 0;
	float bestDist = 1e30f;
	for(size_t i = 0; i < PianoSampleRegionCount; i++)
	{
		const float d = Math::Abs(float(PianoSampleRegions[i].RootKey) - midi);
		if(d < bestDist)
		{
			bestDist = d;
			best = i;
		}
	}
	const PianoRegionData& region = PianoSampleRegions[best];
	// Keep the production path on the accepted nearest-root baseline. Profile
	// blending is intentionally diagnostic-only until its complex-phase result
	// is validated against the rendered SF2 at matching velocity layers.

	// Огибающая-коррекция отключена: атака должна быть как можно резче
	// (мгновенный выход на полный уровень), а не тихий заход.
	mUseEnvelopeCorrection = false;
	mEnvelopeTimeScale = freq/(region.F0*float(sampleRate));
	mEnvelopeLevel = 1.0f;
	mEnvelopeSegment = 0;
	for(size_t i = 0; i < PianoEnvelopePointCount; i++) mEnvelopeGain[i] = 1.0f;

	// Число партиал: сколько влезает из региона (не больше maxPartials и не
	// выше Найквиста с запасом 8% — зависит от транспозиции). FreqRatio —
	// U16-квантованный (0.95+v/327675), декодируем до проверки Найквиста
	// (сырое значение v давало fk в тысячи Гц и обнуляло все партиалы).
	size_t partials = 0;
	for(size_t i = 0; i < size_t(region.PartCount) && i < maxPartials; i++)
	{
		const PianoPartial& pp = PianoAllPartials[region.PartOffset + i];
		const int k = pp.K;
		if(k <= 0) break;
		const float fr = 0.95f + float(pp.FreqRatio)*(1.0f/327675.0f);
		const float fk = float(k)*freq*fr;
		if(fk >= 0.92f*float(sampleRate)*0.5f) break;
		partials = i + 1;
	}
	// «Струны» унисона: 1-3, каждая со своей расстройкой, громкостью и фазой.
	unisonVoices = Math::Clamp(unisonVoices, 1, 3);
	const int voices = unisonVoices;
	float voiceCents[3] = {0, 0, 0};
	float voiceGain[3]  = {1.0f, 0.0f, 0.0f};
	if(voices == 1)
	{
		voiceCents[0] = 0;
		voiceGain[0] = 1.0f;
	}
	else if(voices == 2)
	{
		voiceCents[0] = -0.6f*detuneCents;
		voiceCents[1] = +0.6f*detuneCents;
		voiceGain[0] = 1.0f;
		voiceGain[1] = 0.7f;
	}
	else
	{
		// Асимметричная расстройка (как у реально настроенного рояля: струны
		// не симметричны — симметрия давала глубокие периодические провалы).
		voiceCents[0] = -0.8f*detuneCents;
		voiceCents[1] = 0;
		voiceCents[2] = +1.2f*detuneCents;
		voiceGain[0] = 1.0f;
		voiceGain[1] = 0.7f;
		voiceGain[2] = 0.6f;
	}
	const size_t count = Math::Max(size_t(4),
		(partials*size_t(voices) + 3) & ~size_t(3));

	mS1.SetCount(count);
	mS2.SetCount(count);
	mK.SetCount(count);
	mAmp.SetCount(count);
	mDecay.SetCount(count);
	mDecay1.SetCount(count);
	mDecay2.SetCount(count);
	mDecay3.SetCount(count);
	mDecay4.SetCount(count);
	mDecayRelease.SetCount(count);
	mAtk.SetCount(count);
	const float twoPi = 2.0f*float(Math::PI);
	// Все lambda приходят из fitDecay для конкретного региона/партиала;
	// глобальные поправки по высоте намеренно не применяются.
	// Decay1/2 уже подогнаны к каждому SF2-семплу в PianoRegions.h;
	// дополнительной октавной эвристики здесь быть не должно.
	const float octCorr1 = 1.0f;
	const float octCorr2 = 1.0f;
	// Граница onset измерена для этого SF2-региона генератором таблицы;
	// никаких дополнительных поправок по высоте здесь нет.
	const float decayOnset = region.DecayOnset;
	// cr/ci — компоненты a·sin(φ + dphi·t) = ci·cos(dphi·t) + cr·sin(dphi·t),
	// dphis — фаза на сэмпл (для синтеза периода при нормировке).
	FixedArray<float> crs(count), cis(count), dphis(count);
	const float detuneRatio = Math::Pow(2.0f, 1.0f/1200.0f);
	// velocity→яркость: громче играешь — ярче тембр (volume уже кодирует
	// velocity: exp(vel/127 − 1), vel 60 → 0.59, vel 100 → 0.81, vel 127 → 1).
	const float velF = Math::Clamp((volume - 0.55f)*2.0f, 0.0f, 1.0f);
	const float effBright = brightness + velBrightness*velF;
	const float tilt = 0.8f*Math::Max(0.0f, effBright - 0.25f);
	// Верхние октавы: TrebleTilt подавляет обертона (0 = по семплу — у
	// Clavinova верхние семплы и так фундаментал-доминантны).
	const float treble = Math::Min(2.2f, Math::Max(0.0f, freq/261.63f - 1.0f))*trebleTilt;
	size_t o = 0;
	for(int v = 0; v < voices; v++)
	{
		const float det = Math::Pow(detuneRatio, voiceCents[v]);
		for(size_t p = 0; p < partials; p++, o++)
		{
			const PianoPartial& pp = PianoAllPartials[region.PartOffset + p];
			const int k = pp.K;
			// Упакованные поля таблицы (Amp/Decay — U16, Phase — U8: шаг фазы
			// 360/256=1.41° — неслышим). Декодируем тут, дальше — чистый float.
			const float amp = float(pp.Amp)*(1.0f/65535.0f);
			const float phase0 = (float(pp.Phase)*(1.0f/255.0f) - 0.5f)*twoPi;
			const float decay1 = float(pp.Decay1)*(1.0f/2621.4f);
			const float decay2 = float(pp.Decay2)*(1.0f/2621.4f);
			const float decay3 = float(pp.Decay3)*(1.0f/5461.25f);
			const float decay4 = float(pp.Decay4)*(1.0f/2621.4f);
			const float freqRatio = 0.95f + float(pp.FreqRatio)*(1.0f/327675.0f);
			if(k <= 0 || pp.Amp == 0)
			{
				mK[o] = 2.0f;
				mDecay[o] = 1.0f;
				mDecay1[o] = 1.0f;
				mDecay2[o] = 1.0f;
				mDecay3[o] = 1.0f;
				mDecay4[o] = 1.0f;
				mDecayRelease[o] = 1.0f;
				mAtk[o] = 0.0f;
				mAmp[o] = 0.0f;
				crs[o] = 0.0f;
				cis[o] = 0.0f;
				dphis[o] = 0.0f;
				continue;
			}
			// Частота партиалы: измеренный в семпле ratio (растяжка/негармоничность)
			// × транспозиция × расстройка струны.
			// Diagnostic correction: the table's FreqRatio is already the measured
			// partial-to-root ratio. Applying it to k*freq is correct for the
			// steady state; do not quantize phase through a period-sized integer.
			// Keep the calculation in float at the final step so 48 kHz browser
			// rendering cannot introduce a key-local phase discontinuity.
			const float fk = float(k)*freq*freqRatio*det;
			const float dphi = twoPi*(fk/float(sampleRate));
			// Амплитуда из семпла; brightness/velocity усиливают верха, а
			// treble-tilt на высоких нотах их глушит.
			float a = amp*Math::Pow(float(k), tilt - treble)*voiceGain[v];
			// The D5-E5 sustain mismatch is in the modal profile, not in the
			// onset ramp. Keep this correction local to the root-75 profile and
			// apply it directly to the modal state used by the sustain renderer.
			// It fades across the neighboring notes so the region boundary does
			// not become audible; H1 and the accepted attack ramp are untouched.
			if(false && region.RootKey == 75)
			{
				const float w = Math::Max(0.0f, 1.0f - Math::Abs(midi - 75.0f)/2.0f);
				if(k == 1) a *= 0.88f + 0.12f*(1.0f - w);
				else if(k == 2) a *= 0.60f + 0.40f*(1.0f - w);
				else if(k == 3) a *= 0.76f + 0.24f*(1.0f - w);
				else if(k == 4) a *= 0.68f + 0.32f*(1.0f - w);
				else if(k == 5) a *= 0.74f + 0.26f*(1.0f - w);
				else if(k == 6) a *= 0.82f + 0.18f*(1.0f - w);
			}
			// Фаза: измеренная из семпла. Для дополнительных струн унисона —
			// сдвиг, ОДИНАКОВЫЙ для всех гармоник струны (чистая задержка:
			// относительный спектр не меняется — случайный сдвиг по каждой
			// гармонике складывался с 1-й струной в разной фазе и искажал
			// тембр в «пилу»; задержка даёт биения без изменения спектра).
			float phase = phase0;
			if(v > 0)
			{
				phase -= twoPi*fk*(0.0009f*float(v));
			}
			crs[o] = a*Math::Cos(phase);
			cis[o] = a*Math::Sin(phase);
			dphis[o] = dphi;
			mK[o] = 2.0f*Math::Cos(dphi);
			// Затухание: 3-скоростное из семпла (λ1 — начальный спад, λ2 —
			// средний, λ3 — хвост), масштабируется транспозицией (выше нота —
			// быстрее затухает) и DecayScale инструмента. DecayStiffness (0 у
			// пиано) ускоряет верха: λ·(1 + c·k²). До DecayOnset затухания нет
			// (пик атаки; для верхних нот onset сокращён), mDecay стартует 1.0
			// и на точных границах заменяется шагами λ1/λ2/λ3.
			const float trans = (freq/region.F0)*decayScale;
			const float stiff = 1.0f + decayStiffness*float(k*k);
			const float lam1 = decay1*trans*stiff*octCorr1;
			const float lam2 = decay2*trans*stiff*octCorr2;
			const float lam3 = decay3*trans*stiff;
			const float lam4 = decay4*trans*stiff;
			// AttackT уже преобразован генератором из измеренного времени достижения
			// пика в tau экспоненты; одна и та же модель применяется ко всем нотам.
			mAmp[o] = 0.0f;
			mDecay[o] = 1.0f;
			mDecay1[o] = Math::Exp(-lam1/float(sampleRate));
			mDecay2[o] = Math::Exp(-lam2/float(sampleRate));
			mDecay3[o] = Math::Exp(-lam3/float(sampleRate));
			mDecay4[o] = Math::Exp(-lam4/float(sampleRate));		// Release (демпфер) — ЭТО ДОБАВКА к естественному затуханию, а не его
			// замена: dec_release = dec_natural × dec_damper. Физически демпфер
			// добавляет свой коэффициент затухания к собственным потерям струны.
			// Здесь mDecayRelease хранит ТОЛЬКО шаг демпфера; NoteRelease()
			// домножает его на текущий естественный шаг (dec[p] *= decR[p]).
			//
			// Скорость демпфера: выше нота — быстрее (короткая струна гасится
			// быстрее), выше гармоника — быстрее, но МЯГКО (√k, не k):
			//   λ_damper(k) = (1/0.28с)·√(f_note/f_C4)·√k
			// Скалирование по √k (а не по k) не даёт тембру «провалиться» до
			// чистого фундаментала на коротких нотах — именно это звучало как
			// странный призвук/квакание после release.
			// C4: k=1 → τ=280мс, k=8 → τ≈99мс (растекание 2.8×, не 8×).
			const float tauR = 0.28f*Math::Sqrt(261.625565f/freq) / Math::Sqrt(float(k));
			mDecayRelease[o] = Math::Exp(-1.0f/(tauR*float(sampleRate)));
			// Per-partial attack rise: τ = min(AttackT/k, 0.6 ms). Each partial
			// starts at 0 and rises to full level over its own tauK, so the
			// fundamental is a touch slower than the harmonics. With this the
			// released-voice -60 dB gate frees voices as fast as the baseline;
			// a single uniform 0.8 ms ramp kept released voices above the gate
			// far longer and piled up ~3x live voices on dense pedaled MIDI.
			{
				const float tauK = Math::Min(region.AttackT / float(k), 0.0006f);
				mAtk[o] = 1.0f - Math::Exp(-1.0f/(tauK*float(sampleRate)));
			}
			// The table already contains the complete attack state. Do not apply
			// another attack ramp here: that would double-filter the onset.
			// Измерение (bloom-probe, sliding DFT фундаментала семплов):
			// фундаментал струны расцветает постепенно — C4 63%@12мс 90%@18мс,
			// C5 63%@6мс, C6 63%@8мс, C7 63%@12мс. Старый кэп 0.6 мс делал
			// струну мгновенной; молоточек (отдельный слой) удалён 2026-08-24 —
			// теперь bloom и есть вся атака, верха по-прежнему быстрее
			// фундаментала (~1/k).
			if(false)
			{
				// Disabled experimental bloom/contact path; the tabulated state is
				// intentionally used directly to keep one coherent attack source.
				// Атака: короткая контактная сила возбуждает состояния мод
				// (удар — вход резонаторов, а не звуковая наклейка; см. блок
				// «контактная сила» ниже). Первые contactN отсчётов ноты — это
				// attack-буфер. Струна после контакта расцветает плавно:
				// Измерено по семплам (fundamental bloom): C1≈45мс, C2≈40мс,
				// C3≈32мс, C4≈22мс, C5≈10мс, C6≈8мс, C7≈8-10мс — низкие ноты
				// глухие и расцветают очень медленно, середина самая быстрая.
				// Металл (К3-К5, 800-1500) набирается МЕДЛЕННЕЕ фундаментала:
				// в семпле на атаке эта полоса тише соседних (Δ+11 дБ металла
				// на C4 при мгновенном старте). Партиалы К6+ (1.5-6 кГц —)
				// несут ударный шум контакта и стартуют сразу (base).
				float bloomTauBase;
				if(midi <= 24.0f)
					bloomTauBase = 0.045f;
				else if(midi <= 36.0f)
					bloomTauBase = 0.045f - (0.005f)*(midi - 24.0f)/12.0f;
				else if(midi <= 48.0f) // C2..C3: глухие, медленный расцвет
					bloomTauBase = 0.040f - (0.008f)*(midi - 36.0f)/12.0f;
				else if(midi <= 60.0f) // C3..C4
					bloomTauBase = 0.032f - (0.010f)*(midi - 48.0f)/12.0f;
				else if(midi <= 72.0f) // C4..C5
					bloomTauBase = 0.022f - (0.012f)*(midi - 60.0f)/12.0f;
				else if(midi <= 84.0f) // C5..C6
					bloomTauBase = 0.010f - (0.002f)*(midi - 72.0f)/12.0f;
				else
					bloomTauBase = 0.008f + (0.002f)*Math::Min(1.0f, (midi - 84.0f)/24.0f);
				bloomTauBase = Math::Max(0.004f, Math::Min(0.045f, bloomTauBase));
				// 800-1500 «металл» — только там, где k3-5 реально попадают в эту
				// полосу (у низких нот k3-5 = 300-900 Гц, там металлу роздыху нет).
				const bool metalBand = midi >= 56.0f;
				const float frac = (metalBand && k >= 3 && k <= 5) ? (2.2f + 0.30f*float(k - 3)) : 1.0f;
				const float tauK = Math::Min(0.080f, bloomTauBase*frac);
				mAtk[o] = 0.0f;
			}
		}
	}

	// Лишние лейны (округление до кратного 4) — тишина.
	for(; o < count; o++)
	{
		mK[o] = 2.0f;
		mDecay[o] = 1.0f;
		mDecay1[o] = 1.0f;
		mDecay2[o] = 1.0f;
		mDecay3[o] = 1.0f;
		mDecay4[o] = 1.0f;
		mDecayRelease[o] = 1.0f;
		mAtk[o] = 0.0f;
		mAmp[o] = 0.0f;
		crs[o] = 0.0f;
		cis[o] = 0.0f;
		dphis[o] = 0.0f;
	}

	// Contact-model experiment is intentionally disabled in the audible path.
	// The measured partial state is the single coherent attack source.
	// GPT-план: молоточек синтезируется как сила F[n], а не как аудио-слой.
	// Комплексное состояние партиалы: z[n+1] = e^{jw}·z[n] + G_k·F[n],
	// выход = Im z. G_k несёт амплитуду/фазу партиалы из таблицы (Amp/Phase),
	// нормированную на отклик единичной силы в момент конца контакта Z_ref:
	//   G_k = (A_k·e^{j·phase_k}) / Z_ref_k
	// Тогда финальное состояние после контакта РОВНО по таблице (mS1/mS2 —
	// те же, что раньше), а первые contactN отсчётов (буфер атаки) — это
	// естественный накопительный отклик мод на силу: все компоненты атаки
	// живут на модальных частотах, фазы заданы общим импульсом, и после
	// контакта буфер бесшовно переходит в SIMD-рекурсию струны.
	// Основная частота дискретизации; контакт ~1.6-2.2 мс (низкие дольше).
	const float effScale = scale*region.Loudness;
	const size_t period = Math::Max(size_t(1), size_t(Math::Round(float(sampleRate)/freq)));
	float peakS = 0;
	for(size_t t = 0; t < period; t++)
	{
		float s = 0;
		for(size_t p = 0; p < count; p++)
			s += cis[p]*Math::Cos(dphis[p]*float(t)) + crs[p]*Math::Sin(dphis[p]*float(t));
		peakS = Math::Max(peakS, Math::Abs(s));
	}
	const float c = peakS > 1e-9f ? effScale/peakS : effScale;
#if 0
	const float contactT = useContactModel ? 0.0016f + 0.0006f*Math::Clamp((72.0f - midi)/36.0f, 0.0f, 1.0f) : 0.0f;
	const size_t contactN = useContactModel ? Math::Max(size_t(48), size_t(contactT*float(sampleRate))) : 0;
	// Начальная скорость молотка от MIDI velocity (степенная кривая):
	//   v(v) = v_min + (v_max - v_min)·(v/127)^gamma.
	const float impactV = 0.25f + 0.75f*Math::Pow(volume, 1.5f);
	FixedArray<float> contactF(contactN);
	const float noiseGain = 0.22f; // шум контакта (входит в модальный банк, не звук)
	if(useContactModel) for(size_t i = 0; i < contactN; i++)
	{
		const float u = float(i + 1)/float(contactN);
		const float s2 = Math::Sin(float(Math::PI)*u);
		// Детерминированный шум контакта (хэш): в семпле атака 1.5-6 кГц
		// богата шумом молоточка (наша была на -12..-26 дБ ниже). Тот же
		// шум проходит через модальные резонаторы партиал — как "жёсткий"
		// удар, а не отдельный аудиослой.
		const unsigned h = unsigned(i)*2654435761u + 0x9e3779b9u;
		const float no = 2.0f*(float((h >> 8) & 0xffff)/65535.0f) - 1.0f;
		contactF[i] = impactV*s2*s2*(1.0f + noiseGain*no);
	}
	// Проход 1: единичная сила (G=1) — Z_ref_k = финальное комплексное
	// состояние (ur + j·ui) после полного контакта.
	FixedArray<float> ur(count), ui(count);
	for(size_t p = 0; p < count; p++) { ur[p] = 0.0f; ui[p] = 0.0f; }
	for(size_t i = 0; i < contactN; i++)
	{
		const float f = contactF[i];
		for(size_t p = 0; p < count; p++)
		{
			const float w = dphis[p];
			const float co = Math::Cos(w), sn = Math::Sin(w);
			const float re = ur[p], im = ui[p];
			ur[p] = co*re - sn*im + f;
			ui[p] = sn*re + co*im;
		}
	}
	// Профиль G_k = (crs + j·cis) / Z_ref_k (комплексное деление).
	// Tikhonov-регуляризация: если |Z_ref_k| ~ 0 (контакт длиной ровно N периодов
	// партиалы — у нас 2 мс ≈ 4 цикла @2 кГц), деление даёт G→∞ и буфер атаки
	// взрывается (измерено: |G| до 29, буфер до 186 → c→0 и вся нота тише).
	// Добавляем в знаменатель eps2 = 1e-4·max|Z_ref|²: такие партиалы просто не
	// получают атаку из силы (струна всё равно продолжает с табличного состояния),
	// а остальные сохраняют точный финальный отклик.
	float zmax2 = 0.0f;
	for(size_t p = 0; p < count; p++)
		zmax2 = Math::Max(zmax2, ur[p]*ur[p] + ui[p]*ui[p]);
	const float eps2 = 1e-4f*zmax2;
	FixedArray<float> gr(count), gi(count);
	for(size_t p = 0; p < count; p++)
	{
		const float den = ur[p]*ur[p] + ui[p]*ui[p];
		const float d2 = den + eps2;
		gr[p] = d2 > 1e-30f ? (crs[p]*ur[p] + cis[p]*ui[p])/d2 : 0.0f;
		gi[p] = d2 > 1e-30f ? (cis[p]*ur[p] - crs[p]*ui[p])/d2 : 0.0f;
	}
	// Проход 2: буфер атаки = Σ Im(z[n]) с реальными G.		mAttackBuf.SetCount(0);
		mAttackLen = 0;
		mAttackPos = 0;
	{
		float* zb = mAttackBuf.Data();
		for(size_t p = 0; p < count; p++) { ur[p] = 0.0f; ui[p] = 0.0f; }
		for(size_t i = 0; i < contactN; i++)
		{
			const float f = contactF[i];
			float s = 0.0f;
			for(size_t p = 0; p < count; p++)
			{
				const float w = dphis[p];
				const float co = Math::Cos(w), sn = Math::Sin(w);
				const float re = ur[p], im = ui[p];
				ur[p] = co*re - sn*im + gr[p]*f;
				ui[p] = sn*re + co*im + gi[p]*f;
				s += ui[p]; // Im(z)
			}
			zb[i] = s;
		}
	}
	// Нормировка: пик суммы партиал (только струна) -> effScale (~Scale·Loudness),
	// ровно как в HEAD — сустейн не зависит от атаки. Fинальные состояния партиал
	// РОВНО по таблице (z_result = crs + j·cis).
	const size_t period = Math::Max(size_t(1), size_t(Math::Round(float(sampleRate)/freq)));
	float peakS = 0;
	for(size_t t = 0; t < period; t++)
	{
		float s = 0;
		for(size_t p = 0; p < count; p++)
			s += cis[p]*Math::Cos(dphis[p]*float(t)) + crs[p]*Math::Sin(dphis[p]*float(t));
		peakS = Math::Max(peakS, Math::Abs(s));
	}
	const float c = peakS > 1e-9f? effScale/peakS: effScale;
	// Буфер атаки: сначала ×c (уровень струны), затем per-key gain, чтобы
	// отношение attack/sustain совпало с семплом: измеренный ratio RMS
	// (0-10 мс / 30-300 мс): key 33≈0.13, 47≈0.16, 60≈0.12, 72≈0.73,
	// 84≈0.91, 96≈2.29. Интерполяция по midi; струна после нормировки
	// даёт RMS ≈ 0.5·effScale, поэтому naturalRatio = rawBufRms·c/(0.5·effScale)
	// = rawBufRms·2/peakS (c сокращается).
	{
		float* zb = mAttackBuf.Data();
		float rawRms = 0.0f;
		for(size_t i = 0; i < contactN; i++) rawRms += zb[i]*zb[i];
		rawRms = Math::Sqrt(rawRms/float(contactN));
		// target attack/sustain ratio by key (piecewise linear, midi).
		float tr;
		if(midi <= 47.0f)
			tr = 0.16f;
		else if(midi <= 60.0f)
			tr = 0.16f + (0.12f - 0.16f)*(midi - 47.0f)/13.0f;
		else if(midi <= 72.0f)
			tr = 0.12f + (0.73f - 0.12f)*(midi - 60.0f)/12.0f;
		else if(midi <= 84.0f)
			tr = 0.73f + (0.91f - 0.73f)*(midi - 72.0f)/12.0f;
		else if(midi <= 96.0f)
			tr = 0.91f + (2.29f - 0.91f)*(midi - 84.0f)/12.0f;
		else
			tr = Math::Min(4.0f, 2.29f + (midi - 96.0f)*0.1f);
		const float naturalScale = rawRms*2.0f/peakS;
		float gain = naturalScale > 1e-6f ? tr/naturalScale : 0.0f;
		gain = Math::Clamp(gain, 0.02f, 8.0f);
		// Доводка соответствия семплу (вручную, по итоговым attack/sustain
		// ratio на клавишах): семпл [C2?C3 0.13, C4 0.12, C5 0.73, C6 0.91,
		// C7 2.29] — низкие-средние тихие/тупые, верха резкие. Контрольные
		// точки midi→keyScale: 24:0.35, 36:0.25, 48:0.10, 60:0.06,
		// 72:0.15, 84:0.17, 96:0.26, 108:0.30.
		float keyScale;
		if(midi <= 36.0f)
			keyScale = 0.35f - (0.10f)*(midi - 24.0f)/12.0f;
		else if(midi <= 48.0f)
			keyScale = 0.25f - (0.15f)*(midi - 36.0f)/12.0f;
		else if(midi <= 60.0f)
			keyScale = 0.10f - (0.04f)*(midi - 48.0f)/12.0f;
		else if(midi <= 72.0f)
			keyScale = 0.06f + (0.09f)*(midi - 60.0f)/12.0f;
		else if(midi <= 84.0f)
			keyScale = 0.15f + (0.02f)*(midi - 72.0f)/12.0f;
		else if(midi <= 96.0f)
			keyScale = 0.17f + (0.09f)*(midi - 84.0f)/12.0f;
		else
			keyScale = 0.26f + (0.04f)*Math::Min(1.0f, (midi - 96.0f)/12.0f);
		gain *= keyScale;
		// Струна во время контакта ещё не вышла на табличную амплитуду:
		// она расцветает (bloom) — в семпле первые ~8 мс это преимущественно
		// рокот/удар, а струна набирает силу плавно. Накладываем тот же
		// bloom, что и на SIMD-струну после контакта (без /√k — период
		// контакта короткий, нужен только общий темп набора).
		{
			float bloomTauBase;
			if(midi <= 24.0f) bloomTauBase = 0.045f;
			else if(midi <= 36.0f) bloomTauBase = 0.045f - 0.005f*(midi - 24.0f)/12.0f;
			else if(midi <= 48.0f) bloomTauBase = 0.040f - 0.008f*(midi - 36.0f)/12.0f;
			else if(midi <= 60.0f) bloomTauBase = 0.032f - 0.010f*(midi - 48.0f)/12.0f;
			else if(midi <= 72.0f) bloomTauBase = 0.022f - 0.012f*(midi - 60.0f)/12.0f;
			else if(midi <= 84.0f) bloomTauBase = 0.010f - 0.002f*(midi - 72.0f)/12.0f;
			else bloomTauBase = 0.008f + 0.002f*Math::Min(1.0f, (midi - 84.0f)/24.0f);
			bloomTauBase = Math::Max(0.004f, Math::Min(0.045f, bloomTauBase));
			for(size_t i = 0; i < contactN; i++)
				zb[i] *= 1.0f - Math::Exp(-float(i)/(bloomTauBase*float(sampleRate)));
		}
	}
	// === «РОКОТ» — корпусные резонансы деки (подфундаментальный удар) ===
	// Та же контактная сила возбуждает 3 тяжёлых корпусных резонанса
	// (~78/116/168 Гц, τ≈45 мс). У средних/верхних нот их частоты ниже f0,
	// поэтому чисто-струнная модель их не даёт, а в семплах полоса 60-200 Гц
	// на атаке всегда есть (Δ до +50 дБ). Резонансы быстро гаснут (~0.1 с)
	// и не входят в сустейн. Громкость калибруется по измеренному дефициту:
	// усилитель (scale) у всех клавиш одинаков, поэтому bodyGain постоянная.
	if(useContactModel)
	{
		const size_t bodyAlloc = contactN + size_t(0.12f*float(sampleRate));
		mBodyBuf.SetCount(0);
		mBodyLen = 0;
		mBodyPos = 0;
		float* bb = mBodyBuf.Data();
		const float bodyF[4] = { 78.0f, 116.0f, 168.0f, 285.0f };
		const float bodyGain = 0.0011f; // калибруется по band-проберу
		const float tau = 0.045f;
		const float rho = Math::Exp(-1.0f/(tau*float(sampleRate)));
		float br[4] = {0.0f,0.0f,0.0f,0.0f}, bi[4] = {0.0f,0.0f,0.0f,0.0f};
		for(size_t i = 0; i < bodyAlloc; i++)
		{
			const float f0 = i < contactN ? contactF[i] : 0.0f;
			// Корпус отвечает не непрерывно: удар набирает силу (в семпле 0-2 мс
			// почти тишина — 0.00-0.03 от пика). Воронка в пределах контакта.
			const float rise = Math::Min(1.0f, float(i)/(0.5f*float(contactN)));
			const float f = f0*rise*rise;
			float s = 0.0f;
			for(int j = 0; j < 4; j++)
			{
				const float w = twoPi*bodyF[j]/float(sampleRate);
				const float co = rho*Math::Cos(w), sn = rho*Math::Sin(w);
				const float re = br[j], im = bi[j];
				br[j] = co*re - sn*im + f;
				bi[j] = sn*re + co*im;
				s += bi[j];
			}
			bb[i] = s;
		}
		const float bodyScale = c*bodyGain;
		for(size_t i = 0; i < bodyAlloc; i++) bb[i] *= bodyScale;
	}
#endif
	for(size_t p = 0; p < count; p++)
	{
		mS1[p] = cis[p]*c;
		mS2[p] = (cis[p]*Math::Cos(dphis[p]) + crs[p]*Math::Sin(dphis[p]))*c;
		// Струна после контакта НЕ стартует на 1: она расцветает с mAtk,
		// заданным в per-partial блоке (низкие ноты — медленно, как в
		// семпле; верхние партиалы — быстрее, ~1/√k). Первые контакN
		// отсчётов звучит только attack-буфер, затем струна плавно
		// вступает без шва (состояния мод уже на табличных фазах).
		// Restore the accepted short modal ramp: start each partial at 0 here; the
		// per-partial tauK attack above ramps it up (same as the fast baseline).
		// Do NOT overwrite mAtk with a single uniform value here — that was the
		// 3x live-voice regression on dense pedaled MIDI.
		mAmp[p] = 0.0f;
	}
	// Затухание: старт на DecayOnset, λ1 → λ2 на SegT, λ2 → λ3 на SegT2,
	// λ3 → λ4 на SegT3 (границы — центры окон измерений, из таблицы).
	mDecayOnsetSamples = size_t(decayOnset*float(sampleRate));
	mSegSamples = size_t((decayOnset + region.SegT)*float(sampleRate));
	mSegSamples2 = size_t((decayOnset + region.SegT + region.SegT2)*float(sampleRate));
	mSegSamples3 = size_t((decayOnset + region.SegT + region.SegT2 + region.SegT3)*float(sampleRate));
	mRendered = 0;
	mDecayStarted = false;
	mSegSwitched = false;
	mSegSwitched2 = false;
	mSegSwitched3 = false;
	// Конец ноты на длине семпла (SF2 без лупа). Транспозиция: семпл,
	// сыгранный быстрее/медленнее, короче/длиннее в той же пропорции.
	{
		const float ratio = freq/region.F0;
		mEndSamples = ratio > 1e-6f
			? size_t(region.SampleLen/ratio*float(sampleRate) + 0.5f) : 0;
		mFadeSamples = Math::Max(size_t(1), size_t(0.02f*float(sampleRate)));
	}

	mScratch.SetCount(4*mBlockSize);
	mCount = count;
	mVolume = volume;
	mDone = false;
	mReleased = false;
	// Стерео: constant-power pan. Voice 0 → left, voice 1 → right.
	// StereoPan из региона — измеренный L/R level diff. При 2 голосах
	// каждый панорамируется в свою сторону. При 1 или 3 голосах —
	// используется только общий pan для mono-рендера.
	mStereoPan = region.StereoPan;
	// StereoPan — измеренная разница уровней R-L в дБ (SF2 семплы).
	// Преобразуем в коэффициенты L/R так, чтобы разница уровней совпала
	// с SF2, а суммарный уровень (L+R)/2 сохранился = 0.5 (как при старом
	// моно-рендере panLeft=panRight=0.5). Linear pan, не constant-power,
	// чтобы не менять общую громкость.
	// ratio = 10^(dB/20) — во сколько раз R громче L.
	// L + R = 1.0 (нормировка), R/L = ratio → L=1/(1+ratio), R=ratio/(1+ratio).
	// (L+R)/2 = 0.5 — совпадает со старым моно-путём.
	{
		const float dB = region.StereoPan;
		const float ratio = Math::Pow(10.0f, dB/20.0f);
		const float inv = 1.0f/(1.0f + ratio);
		mStereoGainL = inv;
		mStereoGainR = ratio*inv;
	}
	mPartialsPerVoice = partials;
	// Глобальное экспоненциальное затухание не нужно: затухание уже внутри
	// партиал (mDecay/mDecay2). Оставляем 1.
	mExpStep = 1.0f;
}

size_t AdditiveSampler::GenerateMono(Span<float> ioDst, Span<float> ioDstReverb)
{
	(void)ioDstReverb;
	if(mDone) return 0;
	const size_t n = ioDst.Length();
	float* dst = ioDst.Data();
	RenderInto(n, [dst](float v) mutable {*dst++ += v;});
	return mDone ? 0 : n;
}

void AdditiveSampler::NoteRelease()
{
	if(mReleased) return;
	mReleased = true;
#ifdef INTRA_PROBE_NAN
	fprintf(stderr, "[RELEASE] mRendered=%zu mEndSamples=%zu mFadeSamples=%zu count=%zu\n", mRendered, mEndSamples, mFadeSamples, mCount);
#endif
	// Демпфер добавляется к естественному затуханию: dec[p] уже содержит
	// текущий естественный шаг (1.0 до DecayOnset, потом mDecay1/2/3/4),
	// домножаем его на шаг демпфера. Так release НИКОГДА не медленнее
	// естественного спада (fix C7 «отпустил — стало длиннее, чем держу»),
	// и тембр меняется плавно (√k-растекание). atk[p] обнуляем — разгон
	// атаки при демпфировании уже не нужен.
	const size_t count = mCount;
	float* dec = mDecay.Data();
	const float* decR = mDecayRelease.Data();
	float* atk = mAtk.Data();
	for(size_t p = 0; p < count; p++) { dec[p] *= decR[p]; atk[p] = 0.0f; }
	// mEndSamples не трогаем — нота закончится естественным путём,
	// когда amp[p] → 0 для всех партиал. mDone установится в RenderInto.
}

size_t AdditiveSampler::GenerateStereo(Span<float> ioDstLeft, Span<float> ioDstRight, Span<float> ioDstReverb)
{
	(void)ioDstReverb;
	if(mDone) return 0;
	const size_t n = Math::Min(ioDstLeft.Length(), ioDstRight.Length());
	float* dstL = ioDstLeft.Data();
	float* dstR = ioDstRight.Data();
	if(mStereoPan == 0.0f)
	{
		// pan=0: моно в оба канала (как раньше)
		RenderInto(n, [dstL, dstR](float v) mutable {*dstL++ += v; *dstR++ += v;});
		return mDone ? 0 : n;
	}
	// Честное стерео: constant-power pan по измеренной разнице уровней L/R
	// семплов SF2 (region.StereoPan). Каждый сэмпл моно-рендера панорамируется
	// в L и R с разными коэффициентами. Голоса унисона (2-3 струны с
	// расстройкой) при этом дают настоящее стерео-биение, так как их частоты
	// различны — в L и R приходят разные фазы биений, как в реальном
	// семпле с раздельными L/R микрофонами.
	const float gl = mStereoGainL;
	const float gr = mStereoGainR;
	RenderInto(n, [dstL, dstR, gl, gr](float v) mutable {
		*dstL++ += v*gl;
		*dstR++ += v*gr;
	});
	return mDone ? 0 : n;
}

INTRA_WARNING_POP
