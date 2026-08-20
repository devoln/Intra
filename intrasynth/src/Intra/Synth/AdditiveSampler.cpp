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

	// The correction curve is enabled only for the measured AcousticPiano
	// configuration. Its gains are target-RMS/current-RMS from the matching
	// SF2 root sample, not a global octave heuristic.
	// Для подогнанного инструмента (AcousticPiano) молоточек выключен
	// (HammerLevel=0): ударная атака целиком содержится в измеренной кривой
	// коррекции — подъём из тишины + strike-выброс, см. PianoEnvelope.h.
	// Отдельный тональный «молоточек» (f0/2+f0) анализом семпла не
	// подтверждён и добавлял звонкий тон вместо глухого удара.
	// Огибающая-коррекция отключена: утренний звук — чистый рендер строк
	// (утренняя таблица регионов + атака τ=AttackT/k), без Level-буста ~2.9x,
	// из-за которого пианино стало громче раза в 2-3. Вернуть одним флагом:
	// mUseEnvelopeCorrection = brightness <= 0.2501f && ...
	mUseEnvelopeCorrection = false;
	mEnvelopeTimeScale = freq/(region.F0*float(sampleRate));
	mEnvelopeLevel = 1.0f;
	mEnvelopeSegment = 0;
	for(size_t i = 0; i < PianoEnvelopePointCount; i++) mEnvelopeGain[i] = 1.0f;
	if(mUseEnvelopeCorrection)
	{
		for(size_t i = 0; i < PianoEnvelopeCorrectionCount; i++)
		{
			if(PianoEnvelopeCorrections[i].RootKey != region.RootKey) continue;
			mEnvelopeLevel = PianoEnvelopeCorrections[i].Level;
			// One combined curve per root (attack shape + steady decay), stored
			// in 1/16384 units: the strike overshoot reaches +12 dB on the
			// highest roots, beyond the old 1/32768 (max 2x) scale.
			for(size_t p = 0; p < PianoEnvelopePointCount; p++)
				mEnvelopeGain[p] = float(PianoEnvelopeCorrections[i].Gain[p])*(1.0f/16384.0f);
			break;
		}
	}

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
			// псевдослучайный сдвиг (первая струна — ровно как в семпле, чтобы
			// не ломать атаку).
			float phase = phase0;
			if(v > 0)
			{
				const float h = Math::Sin(float(v + 1)*12.9898f + float(k)*78.233f)*43758.5453f;
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
			mDecay4[o] = Math::Exp(-lam4/float(sampleRate));
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

	mScratch.SetCount(4*mBlockSize);
	mCount = count;
	mVolume = volume;
	// Глобальное экспоненциальное затухание не нужно: затухание уже внутри
	// партиал (mDecay/mDecay2). Оставляем 1.
	mExpStep = 1.0f;
}

size_t AdditiveSampler::GenerateMono(Span<float> ioDst, Span<float> ioDstReverb)
{
	(void)ioDstReverb;
	const size_t n = ioDst.Length();
	float* dst = ioDst.Data();
	RenderInto(n, [dst](float v) mutable {*dst++ += v;});
	return n;
}

size_t AdditiveSampler::GenerateStereo(Span<float> ioDstLeft, Span<float> ioDstRight, Span<float> ioDstReverb)
{
	(void)ioDstReverb;
	const size_t n = Math::Min(ioDstLeft.Length(), ioDstRight.Length());
	float* dstL = ioDstLeft.Data();
	float* dstR = ioDstRight.Data();
	RenderInto(n, [dstL, dstR](float v) mutable {*dstL++ += v; *dstR++ += v;});
	return n;
}

INTRA_WARNING_POP
