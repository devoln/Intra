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
		voiceGain[1] = 0.35f;
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
			const float fk = float(k)*freq*freqRatio*det;
			const float dphi = twoPi*fk/float(sampleRate);
			// Амплитуда из семпла; brightness/velocity усиливают верха, а
			// treble-tilt на высоких нотах их глушит.
			const float a = amp*Math::Pow(float(k), tilt - treble)*voiceGain[v];
			// Фаза: измеренная из семпла. Для дополнительных струн унисона —
			// сдвиг, ОДИНАКОВЫЙ для всех гармоник струны (чистая задержка:
			// относительный спектр не меняется — случайный сдвиг по каждой
			// гармонике складывался с 1-й струной в разной фазе и искажал
			// тембр в «пилу»; задержка даёт биения без изменения спектра).
			float phase = phase0;
			if(v > 0)
			{
				const float h = Math::Sin(float(v + 1)*12.9898f)*43758.5453f;
				phase += twoPi*(h - Math::Floor(h));
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
			// Атака по партиале: τ = min(AttackT/k, 0.6 мс). По измерению атаки
			// семплов (scripts/_tmp-c3deep.js) струна в момент onset уже на
			// полной амплитуде — удар возбуждает её мгновенно, все партиалы
			// выходят на уровень за 2-3 мс (порядок τ=AttackT/k сохранён:
			// фундаментал чуть медленнее верхов). Без кэпа атака размазана.
			{
				const float tauK = Math::Min(region.AttackT / float(k), 0.0006f);
				mAtk[o] = 1.0f - Math::Exp(-1.0f/(tauK*float(sampleRate)));
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

	// Нормировка: пик суммы партиал (без рампов) → scale·Loudness региона.
	// Loudness — кривая уровня банка SF2 (верхняя октава тише середины на
	// 10-12 дБ): Scale инструмента — мастер-громкость, регион задаёт свою.
	// s(t) = Σ ci·cos(dphi·t) + cr·sin(dphi·t)  (это a·sin(φ + dphi·t)).
	const float effScale = scale*region.Loudness;
	const size_t period = Math::Max(size_t(16), size_t(Math::Round(float(sampleRate)/freq)));
	float peakS = 0;
	for(size_t t = 0; t < period; t++)
	{
		float s = 0;
		for(size_t p = 0; p < count; p++)
			s += cis[p]*Math::Cos(dphis[p]*float(t)) + crs[p]*Math::Sin(dphis[p]*float(t));
		peakS = Math::Max(peakS, Math::Abs(s));
	}
	const float c = peakS > 1e-9f? effScale/peakS: effScale;
	for(size_t p = 0; p < count; p++)
	{
		mS1[p] = cis[p]*c;
		mS2[p] = (cis[p]*Math::Cos(dphis[p]) + crs[p]*Math::Sin(dphis[p]))*c;
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

	// «Молоточек»: короткий глухой удар — затухающий низкочастотный тон, а не
	// шум (шумовой LP-удар звучал как «шипение», а не удар). Измерено по атаке
	// семплов: лишняя энергия первых ~10 мс сосредоточена у f0 (и ниже — «тело»
	// удара), поэтому удар = тон на f0/2 (медленное тело, τ≈12 мс) + тон на f0
	// (контакт, τ≈4 мс), фаза от нуля — без щелчка. Уровень не растёт с высотой.
	{
		mHammerAmp = effScale*hammerLevel*(0.6f + 0.4f*velF);
		mHammerDecay = Math::Exp(-160.0f/float(sampleRate));
		mHammerPos = 0;
		const size_t hlen = Math::Max(size_t(96), size_t(sampleRate)/20);
		mHammerNoise.SetCount(hlen);
		if(mHammerAmp > 1e-5f)
		{
			const float wLo = float(Math::PI)*freq/float(sampleRate);      // f0/2
			const float wHi = 2.0f*float(Math::PI)*freq/float(sampleRate); // f0
			const float dLo = Math::Exp(-80.0f/float(sampleRate));
			const float dHi = Math::Exp(-250.0f/float(sampleRate));
			float phLo = 0, phHi = 0, eLo = 1.0f, eHi = 1.0f;
			for(size_t i = 0; i < hlen; i++)
			{
				phLo += wLo;
				phHi += wHi;
				mHammerNoise[i] = 0.55f*eLo*Math::Sin(phLo) + 1.0f*eHi*Math::Sin(phHi);
				eLo *= dLo;
				eHi *= dHi;
			}
			float hpeak = 0;
			for(size_t i = 0; i < hlen; i++) hpeak = Math::Max(hpeak, Math::Abs(mHammerNoise[i]));
			if(hpeak > 1e-6f) for(size_t i = 0; i < hlen; i++) mHammerNoise[i] /= hpeak;
		}
	}

	// «Стрик» по фундаменталу (region root 75, D5–E5): тон на f0 с формой из
	// семпла 75(L): рамп ~4 мс → плато до ~20 мс → спад τ≈15 мс. H1 в семпле
	// на +13 дБ выше steady в 5–20 мс и гаснет за ~35 мс (глухой низкий
	// «тук»); у соседних регионов (72/78) его нет — только +1..+5 дБ.
	{
		// Strike включается только для AcousticPiano (brightness 0.25): у
		// остальных инструментов (BrightAcoustic 0.4, HonkyTonk 0.3, ...)
		// D5–E5 «тук» не подтверждён подгонкой под семпл.
		mStrikeAmp = (region.RootKey == 75 && Math::Abs(brightness - 0.25f) < 1e-4f)
			? effScale*1.36f*region.HammerLevel*(0.6f + 0.4f*velF) : 0.0f;
		mStrikePos = 0;
		const size_t slen = Math::Max(size_t(96), size_t(sampleRate)/12);
		mStrikeNoise.SetCount(slen);
		if(mStrikeAmp > 1e-5f)
		{
			const float w = 2.0f*float(Math::PI)*freq/float(sampleRate); // f0
			const float tA = 0.004f*float(sampleRate);   // рамп до полного уровня
			const float tH = 0.020f*float(sampleRate);   // конец плато
			const float d = Math::Exp(-1.0f/(0.015f*float(sampleRate))); // τ спада ≈15 мс
			float ph = 0;
			for(size_t i = 0; i < slen; i++)
			{
				ph += w;
				float g = float(i) < tA ? float(i)/tA : 1.0f;
				if(float(i) > tH) g *= Math::Pow(d, float(i) - tH);
				mStrikeNoise[i] = g*Math::Sin(ph);
			}
			float speak = 0;
			for(size_t i = 0; i < slen; i++) speak = Math::Max(speak, Math::Abs(mStrikeNoise[i]));
			if(speak > 1e-6f) for(size_t i = 0; i < slen; i++) mStrikeNoise[i] /= speak;
		}
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
	// семпле с раздельными L/R микрофонами. Hammer/strike — в центр.
	const float gl = mStereoGainL;
	const float gr = mStereoGainR;
	RenderInto(n, [dstL, dstR, gl, gr](float v) mutable {
		*dstL++ += v*gl;
		*dstR++ += v*gr;
	});
	return mDone ? 0 : n;
}

INTRA_WARNING_POP
