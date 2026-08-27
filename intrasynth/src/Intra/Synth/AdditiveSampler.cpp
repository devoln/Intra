#include "AdditiveSampler.h"
#include "PianoRegions.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

AdditiveSampler::AdditiveSampler(float freq, float volume, unsigned sampleRate,
	size_t maxPartials, float brightness, float scale, float decayScale,
	float decayStiffness, float detuneCents,
	int unisonVoices, float velBrightness, float trebleTilt)
{
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
	// 2026-08-26: per-key unison spread. Расстройка по регионам подогнана к
	// биениям, измеренным в сырых семплах SF2 (окна 100 мс, моно-сумма):
	//   root 43 (G2): h2 ~0.5 Гц  → ~4.0 цента
	//   root 47 (B2): биений нет  → 0
	//   root 51 (E3): h2 ~1.5 Гц  → ~7.0 цента
	//   root 54 (F#3), 57 (A3): нет → 0
	//   root 60 (C4): ~0.5 Гц (край слабый) → 0.3
	//   C5+: 0.3→1.4 цента (мерцание в первую секунду, Session 13).
	// Глубокий бас (≤ A#1) оставлен без биений: там семпл показывает
	// низкочастотную амплитудную «болтанку», которую нельзя объяснить
	// расстройкой струн (потребовалось бы 20–100+ центов) — не воспроизводим.
	// Верхние партиалы в басе не бьются по построению (per-partial вес
	// глубины ниже), поэтому «иииоуу» на длинных нотах исключено.
	{
		const float spreadHi = 1.4f;  // эталонная расстройка на C5+ (AcousticPiano)
		float base;
		if(midi <= 40.0f) base = 0.0f;
		else if(midi < 45.0f) base = 4.0f;   // регион 43 (G2)
		else if(midi < 49.0f) base = 0.0f;   // регион 47 (B2)
		else if(midi < 53.0f) base = 7.0f;   // регион 51 (E3)
		else if(midi < 56.0f) base = 0.0f;   // регион 54 (F#3)
		else if(midi < 59.0f) base = 0.0f;   // регион 57 (A3)
		else if(midi < 62.0f) base = 0.3f;   // регион 60 (C4)
		else base = 0.3f + 1.1f*Math::Min(1.0f, (midi - 60.0f)/12.0f);
		detuneCents *= base / spreadHi;
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
		// Глубина биений по клавишам (2026-08-26): у семплов SF2 середина
		// клавиатуры бьётся МЕЛКО (C5 h2 ~7 дБ, D#5 ~5 дБ, C4 ~4 дБ), требли
		// — глубоко (C6+ 20-50 дБ), низ почти не бьётся. Плоский баланс
		// 1.0/0.7 (провал 15 дБ) на C4-E5 давал «странный отзвук» — глубокую
		// медленную раскачку на длинных нотах, которой в семпле нет. Уровень
		// второй струны задаёт глубину (пик/провал = (1+g1)/|1−g1|): после
		// нормировки пика тембр и средний уровень не меняются (у обеих струн
		// одинаковые партиалы — меняется только размах биений). Session 14c:
		// мелкая огибающая (8.4 дБ) распространяется на весь бас и середину
		// (≤ E5), глубокая (15 дБ) остаётся только в требли (C6+), где семплы
		// бьются глубоко. Узлы: ≤76: 0.45, 76→84: 0.45→0.7, ≥84: 0.7.
		float g1 = 0.45f;
		if(midi >= 84.0f) g1 = 0.7f;
		else if(midi > 76.0f) g1 = 0.45f + 0.25f*(midi - 76.0f)/8.0f;
		voiceGain[1] = g1;
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
	// Коллапс двух струн в один лейн на партиалу (оптимизация горячего
	// цикла, 2026-08-26): сумма двух расстроенных синусоид с одинаковой
	// фазой  a·(g0·sin(ω0t+φ) + g1·sin(ω1t+φ))  =  a·(g0+g1)·E(t)·sin(ωt+φ+θ),
	// E = sqrt(cos²(Δt) + r²·sin²(Δt)), r = (g0−g1)/(g0+g1), ω = (ω0+ω1)/2.
	// Лейн получает амплитуду a·(g0+g1) и номинальную частоту ω, биение
	// даёт огибающая E(t) в горячем цикле (см. RenderInto). Точность: по
	// амплитуде — точно, отброшен фазовый воббл |θ| ≤ atan(r) ≈ 10°.
	const bool beatCollapse = (voices == 2);
	const int lanes = beatCollapse ? 1 : voices;
	const float gSum = beatCollapse ? (voiceGain[0] + voiceGain[1]) : 1.0f;
	const size_t count = Math::Max(size_t(4),
		(partials*size_t(lanes) + 3) & ~size_t(3));
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
	mBeatStep.SetCount(count);
	mBeatPh.SetCount(count);
	mBeatE0.SetCount(count);
	mBeatE1.SetCount(count);
	mBeatR2.SetCount(count);
	for(size_t p = 0; p < count; p++) { mBeatStep[p] = 0.0f; mBeatPh[p] = 0.0f; mBeatR2[p] = 1.0f; }
	// Базовая глубина биения r = (g0−g1)/(g0+g1); на партиалу докручивается
	// весом w(k) в цикле лейнов (mBeatR2 = (1−(1−r)·w)²). r², а не r:
	// огибающая E = sqrt(1 − (1−r²)·sin²) использует квадрат.
	const float beatR = beatCollapse ? (1.0f - voiceGain[1])/(1.0f + voiceGain[1]) : 0.0f;
	mBeatOn = beatCollapse;
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
	for(int v = 0; v < lanes; v++)
	{
		// При коллапсе — номинальная частота (без расстройки): расстройку
		// струн несёт огибающая биений.
		const float det = beatCollapse ? 1.0f : Math::Pow(detuneRatio, voiceCents[v]);
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
			// D4 — та же шкала, что у D3 (5461.25), а не 2621.4: в таблице
			// D4 == D3 (3-сегментная модель утра), и 4-й сегмент должен быть
			// no-op. При шкале 2621.4 получалось λ4 = 2.08·λ3 — хвост после
			// SegT3 гас вдвое быстрее семпла (у D#5: 11 дБ/с вместо 5 дБ/с),
			// и нота «обрывалась» в сустейне.
			const float decay4 = float(pp.Decay4)*(1.0f/5461.25f);
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
				mBeatStep[o] = 0.0f;
				mBeatR2[o] = 1.0f;
				mAmp[o] = 0.0f;
				crs[o] = 0.0f;
				cis[o] = 0.0f;
				dphis[o] = 0.0f;
				continue;
			}
			// Частота партиалы: измеренный в семпле ratio (растяжка/негармоничность)
			// × транспозиция × расстройка струны.
			const float fk = float(k)*freq*freqRatio*det;
			const float dphi = twoPi*(fk/float(sampleRate));
			// Амплитуда из семпла; brightness/velocity усиливают верха, а
			// treble-tilt на высоких нотах их глушит. При коллапсе унисона
			// амплитуда лейна — ПОЛНАЯ сумма струн (g0+g1): пик суммы при
			// совпадающей фазе = (g0+g1)·a, биение докручивает огибающая.
			float a = amp*Math::Pow(float(k), tilt - treble)*(beatCollapse ? gSum : voiceGain[v]);
			// Фаза: измеренная из семпла. Для дополнительных струн унисона
			// фаза НЕ сдвигается: удар молоточка возбуждает струны в фазе,
			// а биения возникают из-за расстройки (voiceCents), а не из-за
			// начального сдвига. Раньше здесь был сдвиг на фиксированную
			// задержку 0.9 мс (phase -= twoPi*fk*0.0009*v) — при суммировании
			// струн это давало гребенчатый фильтр: на D#5 (fk=623 Гц) пара
			// голосов складывалась в противофазе (201.9° → h1 ослаблен на
			// 7.2 дБ), а h2 (1249 Гц) — в фазе (+3.9 дБ). Итог: «октавный
			// гул» +10 дБ на всём острове D5–E5, хотя таблица амплитуд
			// сбалансирована. Без сдвига спектр каждой струны сохраняется
			// в сумме, и относительный баланс гармоник — ровно по таблице.
			float phase = phase0;
			crs[o] = a*Math::Cos(phase);
			cis[o] = a*Math::Sin(phase);
			dphis[o] = dphi;
			if(beatCollapse)
			{
				// Шаг фазы биения Δ = π·fk·(det1−det0)/sr (знак не важен: E
				// зависит от cos²/sin²). Масштаб по партиале: fk ≈ k·f0, поэтому
				// у k-й гармоники биения в k раз быстрее, как в семпле.
				const float det0 = Math::Pow(detuneRatio, voiceCents[0]);
				const float det1 = Math::Pow(detuneRatio, voiceCents[1]);
				mBeatStep[o] = float(Math::PI)*(fk/float(sampleRate))*(det1 - det0);
				// Per-partial вес глубины биения (2026-08-26): у семпла бьются
				// низкие партиалы (h1/h2), а h3+ держат уровень — глубокая
				// медленная огибающая на h5/h6 при неподвижном h1 давала
				// «иииоуу» (спектр темнел за секунды). w=0 — партиала не
				// бьётся вовсе. В басе/середине h1 у семпла тоже не бьётся
				// (глубина 4-6 дБ = шум окна) — вес 0.25, чтобы не было
				// медленного «насоса»; к C5 плавно до 1 (в требли h1 бьётся
				// глубоко, C6+ 10-33 дБ).
				float w = 0.0f;
				if(k == 1)
				{
					if(midi <= 60.0f) w = 0.25f;
					else if(midi >= 72.0f) w = 1.0f;
					else w = 0.25f + 0.75f*(midi - 60.0f)/12.0f;
				}
				else if(k == 2) w = 1.0f;
				else if(k == 3) w = 0.5f;
				const float rEff = 1.0f - (1.0f - beatR)*w;
				mBeatR2[o] = rEff*rEff;
			}
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
			mAmp[o] = 0.0f;
			mDecay[o] = 1.0f;
			mDecay1[o] = Math::Exp(-lam1/float(sampleRate));
			mDecay2[o] = Math::Exp(-lam2/float(sampleRate));
			mDecay3[o] = Math::Exp(-lam3/float(sampleRate));
			mDecay4[o] = Math::Exp(-lam4/float(sampleRate));
			// Release (демпфер) — ЭТО ДОБАВКА к естественному затуханию, а не его
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
			// Per-partial attack rise для партиал, которых удар не успевает
			// раскачать (короткий контакт не передаёт энергию высоким модам):
			// τ = min(AttackT/k, 0.6 мс). Эти партиалы входят после буфера
			// контактной силы плавно; возбуждённые ударом приходят из буфера
			// уже на полном уровне (mAtk обнуляется в блоке контактной силы).
			// С таким mAtk голос после release добирается до −60 дБ-гейта так
			// же быстро, как в базисе; единый рамп 0.8 мс держал голоса выше
			// гейта заметно дольше и копил ~3× живых голосов на педальных MIDI.
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
		mBeatStep[o] = 0.0f;
		mAmp[o] = 0.0f;
		crs[o] = 0.0f;
		cis[o] = 0.0f;
		dphis[o] = 0.0f;
	}
	// === Атака: контактная сила возбуждает те же моды ===
	// Молоточек — не отдельный звуковой слой, а сила F[n], входящая в
	// рекурсию партиал: z[n+1] = e^{jw}·z[n] + G_k·F[n], выход = Im z.
	// Первые contactN отсчётов ноты — накопленный отклик мод (буфер атаки),
	// дальше — та же SIMD-рекурсия струны с табличным состоянием. G_k
	// подобраны так, что состояние мод после контакта РОВНО равно табличному
	// (Amp/Phase из семпла): атака и сустейн — один модальный банк, и шов
	// буфер→струна бесшовный по построению (последний сэмпл буфера равен
	// первому сэмплу струны — см. seeding ниже).
	//
	// Уроки прошлых щелчков (2026-08-24, сессии 28-30):
	//   - сила стартует с нуля (sin²-огибающая + явный нулевой первый сэмпл) —
	//     нет скачка F[0]−0 на первом сэмпле;
	//   - импульс не плоский: sin²-форма даёт подъём и спад огибающей
	//     (плоская RMS-нормированная «подушка» и была щёлкающим шумом);
	//   - буфер — отклик тех же мод, а не отдельный слой: после контакта
	//     нечему вступать «отдельно».
	const float contactT = 0.0016f + 0.0006f*Math::Clamp((72.0f - midi)/36.0f, 0.0f, 1.0f);
	const size_t contactN = Math::Max(size_t(48), size_t(contactT*float(sampleRate)));
	// Начальная скорость молотка от MIDI velocity (степенная кривая):
	//   v(v) = v_min + (v_max - v_min)·(v/127)^gamma.
	// На модальный буфер не влияет (G нормирует отклик), но определяет
	// силу удара по корпусу — «рокот» ниже.
	const float impactV = 0.25f + 0.75f*Math::Pow(volume, 1.5f);
	FixedArray<float> contactF(contactN);
	{
		const float noiseGain = 0.22f; // шум контакта (входит в моды, не отдельный звук)
		for(size_t i = 0; i < contactN; i++)
		{
			const float u = float(i + 1)/float(contactN);
			const float s2 = Math::Sin(float(Math::PI)*u);
			// Детерминированный шум контакта (хэш): в семпле атака 1.5-6 кГц
			// богата шумом молоточка; тот же шум проходит через модальные
			// резонаторы, как «жёсткий» удар, а не отдельный аудиослой.
			const unsigned h = unsigned(i)*2654435761u + 0x9e3779b9u;
			const float no = 2.0f*(float((h >> 8) & 0xffff)/65535.0f) - 1.0f;
			contactF[i] = impactV*s2*s2*(1.0f + noiseGain*no);
		}
		contactF[0] = 0.0f; // нулевой первый сэмпл — нет скачка на старте
	}
	// Уровень: нормировка пика суммы партиал за один период на effScale.
	const float effScale = scale*region.Loudness;
	const size_t period = Math::Max(size_t(1), size_t(Math::Round(float(sampleRate)/freq)));
	// cos/sin(dphis[p]) — константы партиалы; предвычисляем один раз. В wasm
	// нет аппаратного sin: cosf/sinf — полиномиальная libm (сотни инструкций),
	// их нельзя вызывать внутри циклов по (i,p).
	FixedArray<float> coTab(count), snTab(count);
	for(size_t p = 0; p < count; p++)
	{
		const float w = dphis[p];
		coTab[p] = Math::Cos(w);
		snTab[p] = Math::Sin(w);
	}
	float peakS = 0;
	{
		// Пик суммы за период — через рекуррентное вращение (4 умножения на
		// партиалу на шаг вместо вызова тригонометрии).
		FixedArray<float> ca(count), sa(count);
		for(size_t p = 0; p < count; p++) { ca[p] = 1.0f; sa[p] = 0.0f; }
		for(size_t t = 0; t < period; t++)
		{
			float s = 0;
			for(size_t p = 0; p < count; p++)
				s += cis[p]*ca[p] + crs[p]*sa[p];
			peakS = Math::Max(peakS, Math::Abs(s));
			for(size_t p = 0; p < count; p++)
			{
				const float na = ca[p]*coTab[p] - sa[p]*snTab[p];
				sa[p] = ca[p]*snTab[p] + sa[p]*coTab[p];
				ca[p] = na;
			}
		}
	}
	const float c = peakS > 1e-9f ? effScale/peakS : effScale;
	// Проход 1 (G=1): Z_ref_k — комплексное состояние после полного контакта.
	// Math::Cos/Sin — forceinline-обёртки над cosf/sinf: компилятор сам
	// выносит их из цикла по i (dphis[p] от i не зависит), поэтому здесь
	// держим инлайн, а не предвычисленные массивы.
	FixedArray<float> ur(count), ui(count), gr(count), gi(count);
	for(size_t p = 0; p < count; p++) { ur[p] = 0.0f; ui[p] = 0.0f; }
	for(size_t i = 0; i < contactN; i++)
	{
		const float f = contactF[i];
		for(size_t p = 0; p < count; p++)
		{
			const float co = coTab[p], sn = snTab[p];
			const float re = ur[p], im = ui[p];
			ur[p] = co*re - sn*im + f;
			ui[p] = sn*re + co*im;
		}
	}
	// G_k = (crs + j·cis)/Z_ref_k (комплексное деление). Tikhonov-регуляризация:
	// партиалы, которые удар не успевает раскачать (|Z_ref|² << max), получают
	// G ≈ 0 — их атака берётся per-partial bloom'ом струны (mAtk), как в
	// базисе, а не взрывным делением (без регуляризации |G| доходил до 29 и
	// буфер атаки «взрывался», затягивая всю ноту).
	float zmax2 = 0.0f;
	for(size_t p = 0; p < count; p++)
	{
		// Регуляризация по отклику РЕАЛЬНЫХ мод. Паддинг-лейны и «молчаливые»
		// строки таблицы (Amp==0) имеют dphis=0: на единичную силу они накапливают
		// постоянную DC-составляющую (ur = ΣF), и без этого фильтра именно они
		// выходили на максимум |Z_ref|². Тогда eps2 считался от их DC-отклика,
		// а сами они помечались driven → mAmp=1.0 с dec=1.0 навсегда → гейт −60 дБ
		// не мог закрыть голос → живые «хвосты» копились (13× вместо 43×).
		if(crs[p]*crs[p] + cis[p]*cis[p] > 0.0f)
			zmax2 = Math::Max(zmax2, ur[p]*ur[p] + ui[p]*ui[p]);
	}
	const float eps2 = 1e-4f*zmax2;
	FixedArray<float> driven(count);
	for(size_t p = 0; p < count; p++)
	{
		const float den = ur[p]*ur[p] + ui[p]*ui[p];
		const float d2 = den + eps2;
		gr[p] = d2 > 1e-30f ? (crs[p]*ur[p] + cis[p]*ui[p])/d2 : 0.0f;
		gi[p] = d2 > 1e-30f ? (cis[p]*ur[p] - crs[p]*ui[p])/d2 : 0.0f;
		// driven — только лейны с реальной табличной амплитудой: молчаливые
		// строки (Amp==0) и паддинг не должны получать amp=1/atk=0, иначе они
		// навсегда держат maxAmp=1 и блокируют гейт очистки голосов.
		driven[p] = crs[p]*crs[p] + cis[p]*cis[p] > 0.0f && d2 > 1e-30f && den >= eps2;
	}
	// Проход 2: буфер атаки = Σ Im(z[n]) с реальными G. contactN+1 сэмплов:
	// последний равен Σ Im(реального состояния после контакта) — ровно первому
	// сэмплу струны, шов без разрыва.
	mAttackBuf.SetCount(contactN + 1);
	mAttackLen = contactN + 1;
	mAttackPos = 0;
	float gSeam = 0.0f;
	{
		float* zb = mAttackBuf.Data();
		for(size_t p = 0; p < count; p++) { ur[p] = 0.0f; ui[p] = 0.0f; }
		for(size_t i = 0; i < contactN; i++)
		{
			const float f = contactF[i];
			float s = 0.0f;
			for(size_t p = 0; p < count; p++)
			{
				const float co = coTab[p], sn = snTab[p];
				const float re = ur[p], im = ui[p];
				ur[p] = co*re - sn*im + gr[p]*f;
				ui[p] = sn*re + co*im + gi[p]*f;
				s += ui[p];
			}
			zb[i] = s;
		}
		// Реальное накопленное состояние после контакта (для возбуждённых
		// партиал оно равно табличному; для остальных ≈ 0).
		float sEnd = 0.0f;
		for(size_t p = 0; p < count; p++) sEnd += ui[p];
		zb[contactN] = sEnd;
		for(size_t i = 0; i <= contactN; i++) zb[i] *= c;
		// Уровень атаки: отклик мод на удар по построению бьёт пиком в
		// несколько раз выше сустейна (во время контакта моды складываются
		// в фазе), и без нормировки каждый удар звучит как щелчок. Приводим
		// attack/sustain к измеренным отношениям семпла (RMS 0-10 мс /
		// 30-300 мс: key 33≈0.13, 47≈0.16, 60≈0.12, 72≈0.73, 84≈0.91,
		// 96≈2.29 — интерполяция по midi) и умножаем на keyScale (ручная
		// доводка по контрольным точкам: 24:0.35, 36:0.25, 48:0.10,
		// 60:0.06, 72:0.15, 84:0.17, 96:0.26, 108:0.30).
		float rawRms = 0.0f;
		for(size_t i = 0; i < contactN; i++) rawRms += zb[i]*zb[i];
		rawRms = Math::Sqrt(rawRms/float(contactN));
		// naturalScale — во сколько раз буфер громче сустейна (RMS/RMS):
		// струна после нормировки даёт RMS ≈ 0.5·effScale, буфер уже ×c,
		// поэтому отношение = rawRms/(0.5·effScale) = rawRms·2/peakS.
		const float naturalScale = rawRms*2.0f/peakS;
		float tr;
		if(midi <= 47.0f) tr = 0.16f;
		else if(midi <= 60.0f) tr = 0.16f + (0.12f - 0.16f)*(midi - 47.0f)/13.0f;
		else if(midi <= 72.0f) tr = 0.12f + (0.73f - 0.12f)*(midi - 60.0f)/12.0f;
		else if(midi <= 84.0f) tr = 0.73f + (0.91f - 0.73f)*(midi - 72.0f)/12.0f;
		else if(midi <= 96.0f) tr = 0.91f + (2.29f - 0.91f)*(midi - 84.0f)/12.0f;
		else tr = Math::Min(4.0f, 2.29f + (midi - 96.0f)*0.1f);
		float keyScale;
		if(midi <= 36.0f) keyScale = 0.35f - 0.10f*(midi - 24.0f)/12.0f;
		else if(midi <= 48.0f) keyScale = 0.25f - 0.15f*(midi - 36.0f)/12.0f;
		else if(midi <= 60.0f) keyScale = 0.10f - 0.04f*(midi - 48.0f)/12.0f;
		else if(midi <= 72.0f) keyScale = 0.06f + 0.09f*(midi - 60.0f)/12.0f;
		else if(midi <= 84.0f) keyScale = 0.15f + 0.02f*(midi - 72.0f)/12.0f;
		else if(midi <= 96.0f) keyScale = 0.17f + 0.09f*(midi - 84.0f)/12.0f;
		else keyScale = 0.26f + 0.04f*Math::Min(1.0f, (midi - 96.0f)/12.0f);
		float gain = naturalScale > 1e-6f ? tr/naturalScale : 0.0f;
		gain = Math::Clamp(gain, 0.02f, 8.0f);
		gain *= keyScale;
		// Bloom: в семпле струна набирает силу плавно (низ 40-45 мс,
		// верх 8-10 мс) — тот же темп накладываем на буфер, чтобы удар
		// «раскатывался», а не бил ступенькой. Последний сэмпл буфера
		// (i == contactN) приводится к gain·bloom(contactN) = gSeam — ровно
		// на этот уровень сейдится амплитуда струны, шов остаётся точным.
		float bloomTauBase;
		if(midi <= 24.0f) bloomTauBase = 0.045f;
		else if(midi <= 36.0f) bloomTauBase = 0.045f - 0.005f*(midi - 24.0f)/12.0f;
		else if(midi <= 48.0f) bloomTauBase = 0.040f - 0.008f*(midi - 36.0f)/12.0f;
		else if(midi <= 60.0f) bloomTauBase = 0.032f - 0.010f*(midi - 48.0f)/12.0f;
		else if(midi <= 72.0f) bloomTauBase = 0.022f - 0.012f*(midi - 60.0f)/12.0f;
		else if(midi <= 84.0f) bloomTauBase = 0.010f - 0.002f*(midi - 72.0f)/12.0f;
		else bloomTauBase = 0.008f + 0.002f*Math::Min(1.0f, (midi - 84.0f)/24.0f);
		bloomTauBase = Math::Max(0.004f, Math::Min(0.045f, bloomTauBase));
		gSeam = gain*(1.0f - Math::Exp(-float(contactN)/(bloomTauBase*float(sampleRate))));
		for(size_t i = 0; i <= contactN; i++)
			zb[i] *= gain*(1.0f - Math::Exp(-float(i)/(bloomTauBase*float(sampleRate))));
	}
	// === «РОКОТ» — корпусные резонансы деки (подфундаментальный удар) ===
	// Та же контактная сила возбуждает 4 тяжёлых корпусных резонанса
	// (~78/116/168/285 Гц, τ≈45 мс). У средних/верхних нот их частоты ниже
	// f0, поэтому чисто-струнная модель их не даёт, а в семплах полоса
	// 60-300 Гц на атаке всегда есть. Резонансы быстро гаснут (~0.1 с) и
	// не входят в сустейн. Громкость калибруется по измеренному дефициту:
	// усилитель (scale) у всех клавиш одинаков, поэтому bodyGain постоянна.
	// Удар набирает силу (в семпле 0-2 мс почти тишина) — воронка rise² в
	// пределах контакта.
	{
		const size_t bodyAlloc = contactN + size_t(0.12f*float(sampleRate));
		mBodyBuf.SetCount(bodyAlloc);
		mBodyLen = bodyAlloc;
		mBodyPos = 0;
		float* bb = mBodyBuf.Data();
		const float bodyF[4] = { 78.0f, 116.0f, 168.0f, 285.0f };
		const float bodyGain = 0.0011f; // калибруется по band-проберу
		const float tau = 0.045f;
		const float rho = Math::Exp(-1.0f/(tau*float(sampleRate)));
		float br[4] = {0.0f, 0.0f, 0.0f, 0.0f}, bi[4] = {0.0f, 0.0f, 0.0f, 0.0f};
		for(size_t i = 0; i < bodyAlloc; i++)
		{
			const float f0 = i < contactN ? contactF[i] : 0.0f;
			// Корпус отвечает не непрерывно: удар набирает силу (в семпле
			// 0-2 мс почти тишина — 0.00-0.03 от пика). Воронка в пределах
			// контакта.
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
	// === «УДАР» — фундаментальный транзиент h1(+h2) ===
	// В семплах SF2 на атаке h1 бьёт пиком выше сустейна (измерено по сырым
	// семплам, пик 5-40 мс / сустейн 60-200 мс: C4 0.0 дБ, C5 +3.1,
	// D#5 +10.0, C6 +3.2, D6 +5.7, F6 +5.9, A6 +8.8, C7 +13.9, E7 +13.7,
	// G7 +14.3), h2 — примерно вдвое слабее и тем слабее, чем выше нота.
	// Это удар молоточка: h1 раскачивается сильнее остальных мод и гаснет за
	// ~50-70 мс. Воспроизводим оверлеем, фаза-выровненным к струне (та же
	// комбинация ci·cos(wt) + cr·sin(wt), что у партиал), огибающая стартует
	// с нуля — ни шва на стыке буфер→струна, ни щелчка.
	{
		// A1 — пиковое превышение h1 (линейное: 10^(дБ/20) − 1), интерполяция
		// по измеренным точкам семпла; ниже C4 удара нет.
		float A1;
		if(midi <= 60.0f) A1 = 0.0f;
		else if(midi <= 72.0f) A1 = 1.43f*(midi - 60.0f)/12.0f;
		else if(midi <= 75.0f) A1 = 1.43f + (3.16f - 1.43f)*(midi - 72.0f)/3.0f;
		else if(midi <= 84.0f) A1 = 3.16f + (1.45f - 3.16f)*(midi - 75.0f)/9.0f;
		else if(midi <= 87.0f) A1 = 1.45f + (1.93f - 1.45f)*(midi - 84.0f)/3.0f;
		else if(midi <= 90.0f) A1 = 1.93f + (1.97f - 1.93f)*(midi - 87.0f)/3.0f;
		else if(midi <= 93.0f) A1 = 1.97f + (2.75f - 1.97f)*(midi - 90.0f)/3.0f;
		else if(midi <= 96.0f) A1 = 2.75f + (4.95f - 2.75f)*(midi - 93.0f)/3.0f;
		else if(midi <= 99.0f) A1 = 4.95f + (4.84f - 4.95f)*(midi - 96.0f)/3.0f;
		else if(midi <= 102.0f) A1 = 4.84f + (5.19f - 4.84f)*(midi - 99.0f)/3.0f;
		else A1 = 5.19f;
		// h2-удар относительно h1 (w2, линейное отношение пиков): измерено по
		// сырым семплам SF2 (окно 10-30 мс, с учётом A1): C5 0.74, D#5 0.62,
		// C6 1.0, A6 1.4, C7 0.3 (у C7 h2 в таблице и так близок к h1 — удар
		// его почти не раскачивает), E7 1.1; D6-F6 шумно в семплах, берём ~1.1.
		float w2;
		if(midi <= 72.0f) w2 = 0.8f;
		else if(midi <= 75.0f) w2 = 0.8f + (0.62f - 0.8f)*(midi - 72.0f)/3.0f;
		else if(midi <= 84.0f) w2 = 0.62f + (1.0f - 0.62f)*(midi - 75.0f)/9.0f;
		else if(midi <= 93.0f) w2 = 1.0f + (1.4f - 1.0f)*(midi - 84.0f)/9.0f;
		// w2 — отношение пиков «удара» h2/h1. Так как push-удар h2 пропорционален
		// амплитуде h2-лейна (A2·state_h2), после снижения табличных амплитуд h2
		// (96: 1900→300, 99: 470→110) w2 поднят так, чтобы произведение w2·Amp
		// сохранилось (атака не изменилась): 96: 0.3·1900 = 570 → 1.9·300;
		// 99: 1.1·470 = 517 → 4.7·110. Сустейн при этом — новая табличная форма.
		else if(midi <= 96.0f) w2 = 1.4f + (1.9f - 1.4f)*(midi - 93.0f)/3.0f;
		else if(midi <= 99.0f) w2 = 1.9f + (4.7f - 1.9f)*(midi - 96.0f)/3.0f;
		else if(midi <= 101.0f) w2 = 4.7f;
		else w2 = 1.1f;
		// Кламп 5.0: после снижения табличных амплитуд h2 (96: 1900→300,
		// 99: 470→110) компенсирующий w2 достигает 4.7; старый потолок 1.5
		// не давал сохранить атаку (произведение w2·Amp_h2).
		w2 = Math::Clamp(w2, 0.2f, 5.0f);
		const float A2 = A1*w2;
		// Лейны «удара»: ищем РЕАЛЬНЫЕ k=1 (фундаментал) и k=2 (октава) с
		// ненулевой амплитудой. У большинства регионов это первые две строки
		// таблицы, но в верхних регионах (84/93/96/99) первая строка — старший
		// партиал (k=5/k=4/k=3/k=4), а k=1 стоит второй; у 96 есть и мёртвый
		// дубль k=2 (Amp=0). Жёсткие o1=0/o2=1 давали «удар» h2 на частоте
		// фундаментала, а h1 — на случайной высокой гармонике.
		size_t o1 = size_t(-1), o2 = size_t(-1);
		{
			size_t o2Best = size_t(-1);
			uint16 a2Best = 0;
			for(size_t p = 0; p < partials; p++)
			{
				const PianoPartial& pr = PianoAllPartials[region.PartOffset + p];
				if(pr.K == 1 && pr.Amp > 0 && o1 == size_t(-1)) o1 = p;
				if(pr.K == 2 && pr.Amp > 0 && pr.Amp > a2Best) { a2Best = pr.Amp; o2Best = p; }
			}
			if(A2 > 0.01f && o2Best != size_t(-1)) o2 = o2Best;
		}
		mPushLen = 0;
		mPushPos = 0;
		if(A1 > 0.01f && o1 != size_t(-1))
		{
			// Ramped удар: подъём за ~3 мс (быстрее семплового «тука» h1, но
			// не ступенькой — старт строго с нуля, без щелчка), спад τ≈15 мс.
			// Измерение 2026-08-26: прежний τr=14 мс/τd=40 мс слишком «размазывал»
			// удар — в окне 0-10 мс h2/h1 оставалось −6.7 вместо −9.9 у семпла
			// (h1 не успевал подняться над октавой). Новый удар пикует ~5 мс и
			// гаснет к ~50-70 мс, как h1-тук в сыром семпле («settle ~30-50 мс»).
			const float tauR = 0.003f, tauD = 0.015f;
			// Длина: пока огибающая > ~1% пика (τd·ln(100) ≈ 0.069 с).
			mPushLen = size_t(tauD*Math::Log(100.0f)*float(sampleRate)) + 1;
			// Нормировка огибающей к пику 1 (пик при t* = τr·ln(1+τd/τr)).
			const float tStar = tauR*Math::Log(1.0f + tauD/tauR);
			const float gMax = (1.0f - Math::Exp(-tStar/tauR))*Math::Exp(-tStar/tauD);
			mPushBuf.SetCount(mPushLen);
			float* pb = mPushBuf.Data();
			// Огибающая и вращение без тригонометрии на сэмпл: g(i) =
			// (1−rR^i)·rD^i/gMax, а волна cis·cos(i·w) + crs·sin(i·w) — это Y_i
			// комплексного вращения (X_0,Y_0) = (crs,cis) на w каждый шаг.
			// Оверлеи калибровались по амплитуде ОДНОЙ струны (первого голоса):
			// при коллапсе унисона лейн несёт сумму струн (g0+g1), поэтому
			// уровень удара приводим обратно делением на gSum.
			const float ovStr = beatCollapse ? 1.0f/gSum : 1.0f;
			const float rR = Math::Exp(-1.0f/(tauR*float(sampleRate)));
			const float rD = Math::Exp(-1.0f/(tauD*float(sampleRate)));
			float x1 = crs[o1]*ovStr, y1 = cis[o1]*ovStr;
			const float cw1 = Math::Cos(dphis[o1]), sw1 = Math::Sin(dphis[o1]);
			float x2 = 0.0f, y2 = 0.0f, cw2 = 0.0f, sw2 = 0.0f;
			if(o2 != size_t(-1))
			{
				x2 = crs[o2]*ovStr; y2 = cis[o2]*ovStr;
				cw2 = Math::Cos(dphis[o2]); sw2 = Math::Sin(dphis[o2]);
			}
			float rp = 1.0f, dp = 1.0f;
			for(size_t i = 0; i < mPushLen; i++)
			{
				const float g = (1.0f - rp)*dp/gMax;
				float s = A1*y1;
				if(o2 != size_t(-1)) s += A2*y2;
				pb[i] = c*g*s;
				rp *= rR; dp *= rD;
				const float nx1 = x1*cw1 - y1*sw1;
				y1 = x1*sw1 + y1*cw1;
				x1 = nx1;
				if(o2 != size_t(-1))
				{
					const float nx2 = x2*cw2 - y2*sw2;
					y2 = x2*sw2 + y2*cw2;
					x2 = nx2;
				}
			}
		}
	}
	// === «БЛУМ» сустейна — яркая голова, сседающая к таблице ===
	// В сырых семплах SF2 у D5–E5 (регион 75) обертона h2–h3 в первые
	// ~0.1–0.7 с держатся ПОВЫШЕННО (h2 почти вровень с h1 на 0.1–0.3 с:
	// +1 дБ, к ~0.7 с сседает к плоской табличной форме −8 дБ; h3 выше на
	// 4–8 дБ) и только потом проседают к плоской табличной форме. У плоской
	// струны (Session 7 дала региону ОДИНАКОВОЕ затухание, чтобы отношения
	// держались ровно от t=0) этой временной «яркой головы» нет — первые
	// полсекунды длинной ноты звучат беднее семпла на 4–10 дБ по h2–h3.
	// Блум — третий оверлей: фаза-выровненная сумма партиал h2–h3 × мал.
	// множитель × огибающая (0 до 45 мс — после атаки, подъём τr≈55 мс,
	// экспоненц. спад τd=0.20 с), стартует с нуля — без щелчка, к ~0.75 с
	// доходит до нуля, в поздний сустейн (уже плоский и откалиброванный) не
	// входит. Атака не меняется: блум начинается после неё и на её окнах
	// (0–30 мс) пренебрежимо мал.
	mBloomPos = 0;
	mBloomLen = 0;
	mBloomOn = false;
	// Только остров D5–E5 (регион 75): у соседних регионов (69/72/78/81/84)
	// ранний сустейн УЖЕ совпадает с семплом (h2 в пределах 0–2 дБ) — блум там
	// перелетал бы в «двойной» тембр. Измеренный разрыв 0.1–0.7 с именно у
	// региона 75: h2 −9…−10 дБ, h3 −4…−8 дБ (см. docs/tasks/20260825-*).
	if(region.RootKey == 75)
	{
		// Пер-частичные усиления головы сустейна (линейная амплитуда):
		//   h2 ≈ +7 дБ (×2.2), h3 ≈ +3 дБ (×1.4); h4 — не трогаем: разрыв мал
		// (наши −28 против семпла −27), а с блумом он перелетал в +7 дБ.
		const float b2 = 2.2f, b3 = 1.4f;
		// Огибающая: 0 до t=delay (после атаки), затем (1−e^(−(t−delay)/τr))·e^(−t/τd),
		// норм. к пику; хвост обрезается на −26 дБ с коротким линейным фейдом.
		const float tauR = 0.055f, tauD = 0.20f;
		const float delay = 0.045f;
		const float ts = delay + tauR*Math::Log(1.0f + tauD/tauR);
		const float gMax = (1.0f - Math::Exp(-(ts-delay)/tauR))*Math::Exp(-ts/tauD);
		const size_t lenCut = size_t((tauD*Math::Log(1.0f/0.026f))*float(sampleRate));
		const size_t len = lenCut + size_t(0.02f*float(sampleRate)); // +20 мс фейд
		// Найти лейны h2/h3 с реальной амплитудой (h1/h4/h5 не участвуют).
		size_t lanes[4] = {size_t(-1), size_t(-1), size_t(-1), size_t(-1)};
		for(size_t p = 0; p < partials; p++)
		{
			const PianoPartial& pr = PianoAllPartials[region.PartOffset + p];
			if((unsigned)pr.K >= 2 && (unsigned)pr.K <= 3 && pr.Amp > 0)
				if(lanes[pr.K] == size_t(-1)) lanes[pr.K] = p;
		}
		const float bAmp[4] = {0.0f, 0.0f, b2, b3};
		const bool any = (lanes[2] != size_t(-1) && b2 > 0.01f) || (lanes[3] != size_t(-1) && b3 > 0.01f);
		if(any && gMax > 1e-3f)
		{
			mBloomOn = true;
			mBloomLen = len;
			mBloomBuf.SetCount(len);
			float* bb = mBloomBuf.Data();
			float x[4] = {0}, y[4] = {0}, cw[4] = {0}, sw[4] = {0};
			bool active[4] = {false, false, false, false};
			for(int K = 2; K <= 3; K++)
			{
				const size_t p = lanes[K];
				if(p == size_t(-1) || bAmp[K] <= 0.01f) continue;
				// Фаза на стыке: струна начинает играть после attack-буфера со
				// своего t=0 состояния (состояния SineRange при проигрывании
				// буфера не продвигаются), а блум играет с сэмпла 0. Чтобы на
				// сэмпле mAttackLen фазы совпали (иначе блум складывается с
				// партиалом от противо- до синфазно в зависимости от частоты —
				// на C5 h3 это давало разрушающую интерференцию −10 дБ), стартовый
				// фазор блума поворачиваем НАЗАД на mAttackLen·dphi.
				const float aBack = float(mAttackLen)*dphis[p];
				const float ca = Math::Cos(aBack), sa = Math::Sin(aBack);
				// Блум калиброван по одиночной струне — при коллапсе делим на gSum.
				const float ovStr = beatCollapse ? 1.0f/gSum : 1.0f;
				x[K] = (crs[p]*ca + cis[p]*sa)*ovStr;
				y[K] = (-crs[p]*sa + cis[p]*ca)*ovStr;
				cw[K] = Math::Cos(dphis[p]); sw[K] = Math::Sin(dphis[p]);
				active[K] = true;
			}
			// Бегущие множители огибающей: decay_ = e^(−i/(τd·sr)); rise_ =
			// 1−e^(−(i−delay)/(τr·sr)) через экспоненциальное сглаживание
			// rise_ += (1−rise_)·(1−rR) — начинается с 0 на i=dN. 4 умножения
			// на сэмпл — конструктор дёшев.
			const float rD = Math::Exp(-1.0f/(tauD*float(sampleRate)));
			const float rR = Math::Exp(-1.0f/(tauR*float(sampleRate)));
			const size_t dN = size_t(delay*float(sampleRate));
			float decay_ = 1.0f, rise_ = 0.0f;
			for(size_t i = 0; i < len; i++)
			{
				float g = 0.0f;
				if(i > dN) rise_ += (1.0f - rise_)*(1.0f - rR);
				if(i >= dN) g = rise_*decay_/gMax;
				float s = 0.0f;
				for(int K = 2; K <= 3; K++) if(active[K]) s += bAmp[K]*y[K];
				bb[i] = c*g*s;
				decay_ *= rD;
				for(int K = 2; K <= 3; K++)
				{
					if(!active[K]) continue;
					const float nxx = x[K]*cw[K] - y[K]*sw[K];
					y[K] = x[K]*sw[K] + y[K]*cw[K];
					x[K] = nxx;
				}
			}
			// Линейный фейд последних 20 мс к нулю (без щелчка на обрезании).
			const size_t fadeN = size_t(0.02f*float(sampleRate));
			for(size_t i = lenCut; i < len; i++)
			{
				const float f = float(len - i)/float(fadeN);
				bb[i] *= f;
			}
		}
	}

	// Струна: состояния — табличные (моды после контакта), амплитуды:
	//   - возбуждённые ударом входят на уровне gSeam (последний сэмпл буфера
	//     уже приведён к нему — шов бесшовный) и дорастают до табличной
	//     амплитуды через per-partial mAtk;
	//   - не возбуждённые (G≈0) — плавный bloom с нуля через per-partial mAtk.
	for(size_t p = 0; p < count; p++)
	{
		mS1[p] = cis[p]*c;
		mS2[p] = (cis[p]*Math::Cos(dphis[p]) + crs[p]*Math::Sin(dphis[p]))*c;
		if(driven[p]) mAmp[p] = gSeam;
		else mAmp[p] = 0.0f;
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
	mOverlayGain = 1.0f;
	mOverlayRel = 1.0f;
	mOverlayActive = true;
	mSampleRate = sampleRate;
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
	// Транзиенты удара/корпуса при отпускании гаснут плавно (τ≈8 мс):
	// это «удар в воздухе», демпфер убивает и его (иначе на стаккато
	// h1-удар +14 дБ продолжал бы звучать после отпускания), но резкий
	// обрыв в ненулевой амплитуде дал бы щелчок.
	mOverlayRel = Math::Exp(-1.0f/(0.008f*float(mSampleRate)));
	// mEndSamples не трогаем — нота закончится естественным путём,
	// когда amp[p] → 0 для всех партиал. mDone установится в RenderInto.
}

size_t AdditiveSampler::GenerateMono(Span<float> ioDst)
{
	if(mDone) return 0;
	const size_t n = ioDst.Length();
	float* dst = ioDst.Data();
	RenderInto(n, [dst](float v) mutable { *dst++ += v; });
	return mDone ? 0 : n;
}

size_t AdditiveSampler::GenerateStereo(Span<float> ioDstLeft, Span<float> ioDstRight)
{
	if(mDone) return 0;
	const size_t n = Math::Min(ioDstLeft.Length(), ioDstRight.Length());
	float* dstL = ioDstLeft.Data();
	float* dstR = ioDstRight.Data();
	if(mStereoPan == 0.0f)
	{
		RenderInto(n, [dstL, dstR](float v) mutable { *dstL++ += v; *dstR++ += v; });
		return mDone ? 0 : n;
	}
	const float gl = mStereoGainL;
	const float gr = mStereoGainR;
	RenderInto(n, [dstL, dstR, gl, gr](float v) mutable { *dstL++ += v*gl; *dstR++ += v*gr; });
	return mDone ? 0 : n;
}

INTRA_WARNING_POP
