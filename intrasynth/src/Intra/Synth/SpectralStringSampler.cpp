#include "SpectralStringSampler.h"

#include <Audio/AudioProcessing.h>
#include <Range/Mutation/Fill.h>

#include "KarplusStrongSampler.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

// Ручки живого эксперимента (Update 125): одна переменная на процесс, пишет
// веб-UI через SynthSetGuitarTweaks, читает SpectralStringInstrument::operator()
// при создании голоса. По умолчанию выключены.
GuitarTweaks GuitarTweakState;

namespace
{
	/// СЕКЦИЯ ФОРМАНТНОГО ТРАКТА (Update 150) — RBJ-биквад с нормировкой на a0.
	/// kind: 0 — полка низа, 1 — резонансный пик, 2 — полка верха. Для пика qOrS —
	/// добротность, для полок — крутизна S. Формулы RBJ (Audio EQ Cookbook).
	void ToneBiquad(float& b0, float& b1, float& b2, float& a1, float& a2,
		float freq, float gainDb, float qOrS, uint kind, unsigned sampleRate)
	{
		const float f = Math::Clamp(freq, 20.0f, float(sampleRate)*0.45f);
		const float A = Math::Pow(10.0f, gainDb/40.0f);
		const float w = 2.0f*float(Math::PI)*f/float(sampleRate);
		const float cw = Math::Cos(w), sw = Math::Sin(w);
		float nb0, nb1, nb2, na0, na1, na2;
		if(kind == 1)
		{
			const float alpha = sw/(2.0f*Math::Max(qOrS, 0.05f));
			nb0 = 1.0f + alpha*A;
			nb1 = -2.0f*cw;
			nb2 = 1.0f - alpha*A;
			na0 = 1.0f + alpha/A;
			na1 = -2.0f*cw;
			na2 = 1.0f - alpha/A;
		}
		else
		{
			const float S = Math::Max(qOrS, 0.05f);
			const float alpha = 0.5f*sw*Math::Sqrt((A + 1.0f/A)*(1.0f/S - 1.0f) + 2.0f);
			const float two = 2.0f*Math::Sqrt(A)*alpha;
			if(kind == 0)
			{
				nb0 = A*((A + 1.0f) - (A - 1.0f)*cw + two);
				nb1 = 2.0f*A*((A - 1.0f) - (A + 1.0f)*cw);
				nb2 = A*((A + 1.0f) - (A - 1.0f)*cw - two);
				na0 = (A + 1.0f) + (A - 1.0f)*cw + two;
				na1 = -2.0f*((A - 1.0f) + (A + 1.0f)*cw);
				na2 = (A + 1.0f) + (A - 1.0f)*cw - two;
			}
			else
			{
				nb0 = A*((A + 1.0f) + (A - 1.0f)*cw + two);
				nb1 = -2.0f*A*((A - 1.0f) + (A + 1.0f)*cw);
				nb2 = A*((A + 1.0f) + (A - 1.0f)*cw - two);
				na0 = (A + 1.0f) - (A - 1.0f)*cw + two;
				na1 = 2.0f*((A - 1.0f) - (A + 1.0f)*cw);
				na2 = (A + 1.0f) - (A - 1.0f)*cw - two;
			}
		}
		const float inv = 1.0f/na0;
		b0 = nb0*inv;
		b1 = nb1*inv;
		b2 = nb2*inv;
		a1 = na1*inv;
		a2 = na2*inv;
	}
}

SpectralStringSampler::SpectralStringSampler(float freq, float volume, unsigned sampleRate,
	float damping, float sfBase, float sfMul, float sfExp, float expCoeff, float rateCap,
	float dampSlope, SpectralStringRegion tiltExpRegion, SpectralStringRegion inharmRegion, uint inharmPeriods,
	float beatSplit, float beatDepth,	float pickupDepth, float pickupAperture, SpectralStringRegion sustainMaxRegion, float cabQ, SpectralStringRegion cabPolesRegion, float postHpHz,
	SpectralStringRegion pickupRegion, SpectralStringRegion scaleRegion,
	SpectralStringRegion pickupHzRegion, SpectralStringRegion driveRegion,
	SpectralStringRegion biasRegion, SpectralStringRegion toneRegion,
	float presenceHz, float presenceDb, SpectralToneStack tone):
	mPos(0), mHavePrev(false)
{
	// РЕГИОНЫ ПО КЛАВИШАМ (Update 109): числа тракта берутся на трёх якорях
	// 40/64/88 (фактически E3/E5/E7 — см. SpectralRegionValue) и интерполируются
	// здесь ОДИН раз на голос. Ниже по коду — те же имена, что были у скалярных
	// параметров, поэтому рендер не изменился.
	const float pickup = SpectralRegionValue(pickupRegion, freq);
	const float scale = SpectralRegionValue(scaleRegion, freq);
	const float pickupHz = SpectralRegionValue(pickupHzRegion, freq);
	const float drive = SpectralRegionValue(driveRegion, freq);
	const float bias = SpectralRegionValue(biasRegion, freq);
	const float toneCutoff = SpectralRegionValue(toneRegion, freq);
	// Предел сустейна канала (Update 116) — тоже по регионам: подъём уровня на
	// входе клипера нужен верхам (у банка их сустейн яркий) и почти не нужен
	// низам, где он превращался в «дребезжащий» верх.
	const float sustainMax = SpectralRegionValue(sustainMaxRegion, freq);
	// Негармоничность (Update 112) — тоже по регионам: у банка B на верхах на
	// порядок больше, чем на низах (тонкая струна жёстче толстой).
	const float inharm = SpectralRegionValue(inharmRegion, freq);
	// Наклон датчика (Update 122) стал региональным: низ и середина требуют
	// заметно более гладкой струны, чем верх (см. комментарий у TiltExp в .h).
	const float tiltExp = SpectralRegionValue(tiltExpRegion, freq);
	// Сколько секций кабинета (Update 141) — тоже по регионам: у банка падение
	// верхнего ряда на высоких клавишах втрое круче нашего, а якорь 76 до C6
	// доходит ослабленным втрое (см. комментарий у CabPoles в .h). Порог 1.5 —
	// «вторая секция включена», поэтому при якорях [1, 1, 2.5] переключение стоит
	// РОВНО на C6, а всё до C5 включительно остаётся на одной секции.
	const float cabPoles = SpectralRegionValue(cabPolesRegion, freq);
	// Биения поляризаций (Update 112): модуляция амплитуды мод; выключены, если
	// глубина нулевая (тогда advancePeriod идёт прежним путём бит-в-бит).
	mBeatDepth = beatDepth > 0 ? Math::Clamp(beatDepth, 0.0f, 0.95f) : 0;
	mBeatSplit = beatSplit > 0 ? beatSplit : 0;
	mBeatPhase = 0;
	// ==== ЗАТРАВКА ОТ GUITAR STEEL (Update 106) ==============================
	// Зерно генератора, сглаживающий коэффициент петли и щипок берутся у
	// KarplusStrongSampler — те же формулы, тот же порядок обращений к генератору
	// (smoothFactor тратит один отсчёт шума, generateExcitation — остальные),
	// поэтому возбуждение для каждой клавиши совпадает с KS отсчёт в отсчёт.
	Random::FastUniform<float> noise(KarplusStrongSampler::randGen(freq, volume, sampleRate));
	const float sf = Math::Clamp(KarplusStrongSampler::smoothFactor(freq, sfBase, sfMul, sfExp, noise),
		0.0f, 0.9f);
	// Период ноты в отсчётах. Дробную часть ведёт mRate, поэтому округление
	// периода здесь ни на что не влияет: высота берётся из precisePeriod.
	const float precisePeriod = float(sampleRate)/freq - sf;
	const size_t period = Math::Max(size_t(16), size_t(Math::Round(precisePeriod)));
	const size_t tile = Math::CeilToNextPow2(uint(period));   // период в буфере при mPeriods = 1

	// ==== СЕТКА БУФЕРА: СКОЛЬКО ПЕРИОДОВ В НЁМ ЛЕЖИТ (Update 113) ==========
	// Буфер одного периода может нести только моды, укладывающиеся в него ЦЕЛОЕ
	// число циклов, то есть строго кратные f0. Чтобы частота моды могла быть
	// НЕкратной, буфер несёт mPeriods периодов: его сетка частот — f0/mPeriods,
	// и мода k встаёт на бин round(k·L·√(1 + B·k²)), то есть на СВОЮ частоту.
	// Только степень двойки (иначе тайл не целый и повтор перестаёт быть точным)
	// и не длиннее 8192 отсчётов (память и БПФ растут как mN, а на низах число
	// периодов само уменьшается).
	size_t periods = 1;
	if(inharm > 0 && inharmPeriods > 1)
	{
		size_t want = size_t(inharmPeriods);
		if(want > 8) want = 8;
		size_t p = 1;
		while(p*2 <= want) p *= 2;
		while(p > 1 && tile*p > size_t(8192)) p >>= 1;
		periods = p;
	}
	mPeriods = periods;
	mN = tile*mPeriods;
	// Форма РАСТЯГИВАЕТСЯ на весь буфер (Update 98): mN отсчётов = mPeriods
	// периодов; читается со скоростью mN/(mPeriods·precisePeriod), поэтому
	// реальный период вывода остаётся precisePeriod, а бин k·mPeriods несёт РОВНО
	// гармонику k щипка (при mPeriods = 1 это прежнее «бин k = гармоника k»).
	mRate = float(mN)/(float(mPeriods)*precisePeriod);
	// КРОССФЕЙД — НА ВЕСЬ БУФЕР (Update 113). Формы соседних буферов отличаются
	// ТОЛЬКО амплитудами мод (частоты и фазы впечатаны в бины), поэтому линейный
	// кроссфейд от предыдущей формы к текущей — это линейная аппроксимация
	// экспоненциальной огибающей КАЖДОЙ моды за буфер: на стыке буферов огибающая
	// непрерывна, а не ступенчата.
	//
	// Почему это обязательно, а не украшение: при коротком кроссфейде огибающая
	// успевала пройти ступенькой, и в спектре появлялась амплитудная модуляция на
	// частоте обновления f₀/mPeriods — ровно МЕЖДУ гармониками, где ухо слышит её
	// как дрожание. Замер .scratch/guitar-doublet113.mjs: боковины стояли на ±1 шаг
	// сетки от каждой моды и ехали вместе с mPeriods (−5 дБ при 8 периодах,
	// −1 дБ при 4, в сборке 112 их нет вовсе: там частота обновления — сам f₀,
	// 82 Гц, и боковины прячутся среди гармоник). Остаётся только ошибка линейной
	// аппроксимации экспоненты — второй порядок, (rate·T_буфера)²/8 долей.
	mCrossfade = float(mN);
	// Биения: спектр пересчитывается раз в БУФЕР, поэтому и фаза модуляции идёт
	// на mPeriods периодов за шаг (иначе её темп упал бы в mPeriods раз).
	mBeatStep = 2.0f*float(Math::PI)*mBeatSplit*float(mPeriods);
	mBeatPhaseStep = mBeatSplit*float(mPeriods);
	// Живые моды: частота f_k = k·f0·√(1 + B·k²) ниже Найквиста (k·√(1+B·k²) ≤
	// precisePeriod/2) и бин не выходит за половину буфера.
	const float kLimit = precisePeriod*0.5f;
	size_t kMax = 1;
	for(size_t k = 1; k <= mN/2; k++)
	{
		const float kf = float(k);
		const float stretch = Math::Sqrt(1.0f + inharm*kf*kf);
		if(kf*stretch > kLimit) break;
		if(float(mPeriods)*kf*stretch > float(mN/2)) break;
		kMax = k;
	}

	FixedArray<float> excitation;
	excitation.SetCount(mN);
	{
		// Щипок KS — буфер одного периода длиной len (как у KS). Он растягивается
		// линейной интерполяцией на ТАЙЛ (один период в буфере, mN/mPeriods
		// отсчётов) и ПОВТОРЯЕТСЯ mPeriods раз. Растяжение (а не укладка в первые
		// len отсчётов) нужно, чтобы форма осталась периодичной, а повтор — чтобы
		// спектр щипка стоял ТОЛЬКО на бинах, кратных mPeriods: бин k·mPeriods
		// несёт ровно гармонику k, из него мода k и берётся при раскладке
		// (Update 113). При mPeriods = 1 это прежний код.
		const size_t len = Math::Max(size_t(1), size_t(Math::Round(precisePeriod)));
		FixedArray<float> line;
		line.SetCount(len);
		KarplusStrongSampler::generateExcitation(line.AsRange(), damping, noise);
		const size_t tileLen = mN/mPeriods;   // степень двойки: тайл точно целый
		const float step = float(len)/float(tileLen);
		for(size_t i = 0; i < mN; i++)
		{
			const float p = float(i & (tileLen - 1))*step;
			const size_t a = size_t(p);
			const float frac = p - float(a);
			const size_t b = a + 1 < len ? a + 1 : 0;
			excitation[i] = line[a] + (line[b] - line[a])*frac;
		}
	}

	// КОМБ ЗВУКОСНИМАТЕЛЯ (Update 115) — В ЧАСТОТНОЙ ОБЛАСТИ, ПО МОДАМ.
	// Датчик стоит на деке, его расстояние до бриджа В МЕТРАХ постоянно, а
	// звучащая часть струны короче с высотой ноты: на октаву вверх — вдвое
	// короче, поэтому ДОЛЯ q = d/L удваивается на октаву (pickup в JSON задан
	// для C4). Мода k на точке q даёт амплитуду ∝ |1 − e^(−i2πkq)| = 2|sin(πkq)|,
	// то есть нули стоят РОВНО на гармониках n/q — этот же результат даёт
	// свёртка с парой отводов 1 − z^(−q·N).
	//
	// ПОЧЕМУ НЕ ВРЕМЕННОЙ СВЁРТКОЙ (как было до Update 115): во-первых, при
	// q = 0.5 нули ложатся РОВНО на чётные гармоники на всех клавишах выше A#4,
	// и нули были бесконечными — спектр становился «только нечётные», то есть
	// прямоугольной волной (замер .scratch/guitar-harm115e.mjs: нечётность +10.4 дБ
	// на C4 против банковских −1.2; крест-фактор 1.4 против 1.9). У реального
	// датчика катушка УСРЕДНЯЕТ движение по длине (апертура), и нули тем мельче,
	// чем короче волна моды: depth_k = pickupDepth·sinc(π·pickupAperture·k).
	// Во-вторых, свёртка во времени стоит mN операций на голос, а здесь —
	// несколько операций НА МОДУ, уже в готовом цикле раскладки.
	//
	// Заодно ушла дробная задержка: свёртка брала delayed с линейной
	// интерполяцией, то есть усиление нуля зависело от того, насколько q·tile
	// близко к целому, — от клавиши к клавише это гуляло.
	const bool hasComb = pickup > 0;
	const float combQ = hasComb ? Math::Clamp(pickup*freq/261.63f, 0.02f, 0.5f) : 0;
	const float combDepth = Math::Clamp(pickupDepth, 0.0f, 1.0f);
	const float combAp = Math::Max(pickupAperture, 0.0f)*float(Math::PI);

	// Спектр возмущения: разложение формы щипка по модам струны.
	mSpecRe.SetCount(mN);
	mSpecIm.SetCount(mN);
	for(size_t i = 0; i < mN; i++) mSpecRe[i] = excitation[i];
	FillZeros(mSpecIm.AsRange());
	Audio::InplaceFFT(mSpecRe.AsRange(), mSpecIm.AsRange());
	// Постоянная составляющая формы — статическое смещение струны: в звуке
	// его нет, а рабочую точку клипера оно бы сдвигало (это роль Bias).
	mSpecRe[0] = 0;
	mSpecIm[0] = 0;

	// ==== РАСКЛАДКА МОД НА СЕТКУ БУФЕРА (Update 113) ========================
	// Мода k берётся с бина k·mPeriods (там ровно гармоника k щипка) и ставится
	// на бин своей РАСТЯНУТОЙ частоты: dst = round(k·mPeriods·√(1 + B·k²)).
	// Полоса до клипера (PickupHz), наклон датчика (TiltExp) и НУЛИ КОМБА
	// ДАТЧИКА (Update 115, см. выше: H = 1 − depth_k·e^(−i2πkq)) — свойства
	// ГАРМОНИКИ, поэтому считаются по номеру k, а не по бину. Мод выше Найквиста
	// ноты не существует: их бины в раскладку не попадают, потому что выше kMax
	// цикла нет.
	//
	// Цели собираются в ОТДЕЛЬНОМ буфере (mFftRe/mFftIm свободны до первого
	// обратного БПФ): так раскладка не может ни затереть ещё не прочитанный
	// источник, ни попасть под зануление остатков, а моды, слипшиеся в один бин
	// (сдвиг меньше бина), складываются — вместе со своими фазами.
	mFftRe.SetCount(mN);
	mFftIm.SetCount(mN);
	FillZeros(mFftRe.AsRange());
	FillZeros(mFftIm.AsRange());
	for(size_t k = 1; k <= kMax; k++)
	{
		const float kf = float(k);
		const size_t src = k*mPeriods;
		float gain = 1.0f;
		if(pickupHz > 0)
		{
			const float f = kf*freq;
			gain = 1.0f/(1.0f + Math::Pow(f/pickupHz, 4.0f));
		}
		if(pickup > 0) gain *= tiltExp == 1.0f ? kf : Math::Pow(kf, tiltExp);
		float re = mSpecRe[src]*gain;
		float im = mSpecIm[src]*gain;
		if(hasComb)
		{
			// Комб по моду k: H = 1 − depth_k·e^(−i2πkq), depth_k считается с
			// апертурой (sinc(0) = 1, поэтому при нулевой апертуре это прежний
			// комб с бесконечными нулями). Комплексное умножение — 6 операций.
			float depth = combDepth;
			if(combAp > 0)
			{
				const float x = combAp*kf;
				depth *= Math::Sin(x)/x;
			}
			const float ph = 2.0f*float(Math::PI)*kf*combQ;
			const float c = Math::Cos(ph);
			const float s = Math::Sin(ph);
			const float reOld = re;
			re -= depth*(reOld*c + im*s);
			im -= depth*(im*c - reOld*s);
		}
		const size_t dst = size_t(Math::Round(kf*float(mPeriods)*Math::Sqrt(1.0f + inharm*kf*kf)));
		if(dst*2 == mN)
		{
			// Бин Найквиста у вещественного сигнала вещественный: мнимую часть
			// туда писать нельзя — сигнал стал бы комплексным.
			mFftRe[dst] += re;
			continue;
		}
		mFftRe[dst] += re;
		mFftIm[dst] += im;
		if(dst > 0)
		{
			mFftRe[mN - dst] += re;
			mFftIm[mN - dst] -= im;
		}
	}
	for(size_t b = 0; b < mN; b++)
	{
		mSpecRe[b] = mFftRe[b];
		mSpecIm[b] = mFftIm[b];
	}

	// Нормировка на RMS слышимых мод: тогда Scale — уровень, одинаковый для
	// всех нот, независимо от геометрии комба и точки щипка (в прежней нормировке
	// на сумму амплитуд ВСЕХ бинов уровень гулял на ±18 дБ по клавишам).
	// Парсеваль: для вещественного сигнала X_k = X_{mN−k}, а обратный БПФ делит
	// на mN, поэтому rms = sqrt(2·Σ|X_k|²)/mN.
	{
		// Считаем по ВСЕМ бинам: ненулевые — только занятые моды, а делить на mN
		// нужно всё равно (Парсеваль для полного буфера, см. ниже).
		float power = 0;
		for(size_t b = 1; b < mN/2; b++)
			power += mSpecRe[b]*mSpecRe[b] + mSpecIm[b]*mSpecIm[b];
		const float rms = Math::Sqrt(2.0f*power)/float(mN);
		if(rms > 0)
		{
			const float norm = 1.0f/rms;
			for(size_t k = 0; k < mN; k++) { mSpecRe[k] *= norm; mSpecIm[k] *= norm; }
		}
	}

	// ==== СПИСОК МОД И ЗАКОН ПОТЕРЬ — ЗАКОН KS (Updates 106-113) ===========
	// Гармоника k теряет за ОДИН ПРОХОД петли столько же, сколько в петле KS:
	// |H(ω_k)| однополюсника H = (1−sf)/(1−sf·z⁻¹). За секунду проходов f0,
	// поэтому rate_k = −f0·ln|H(ω_k)|. Сверху — общий экспоненциальный спад
	// e^{−ExpCoeff·t} (у steel 1.0). RateCap ограничивает скорость потерь
	// (1/с): у KS верх гаснет ∝ f²·f0 и под клипером остаётся почти синус, а
	// предел оставляет верх живым ровно настолько, чтобы нелинейности было что
	// резать (0 — чистый KS). Внутреннее трение (DampSlope, Update 110) — добавка,
	// пропорциональная частоте гармоники ОТНОСИТЕЛЬНО основного тона: основной
	// тон и сустейн ноты не трогаются, а гармоники расходятся с ним по скорости.
	//
	// В списке — только ЗАНЯТЫЕ бины (их в разы меньше, чем mN), и из слипшихся
	// берётся младшая по номеру мода: их затухания различаются на доли процента.
	// Коэффициент считается за ОДИН БУФЕР, то есть за mPeriods периодов.
	mModeBin.SetCount(kMax + 1);
	mModeDecay.SetCount(kMax + 1);
	mModeK.SetCount(kMax + 1);
	mModeCount = 0;
	float rate1 = 0;
	for(size_t k = 1; k <= kMax; k++)
	{
		const float kf = float(k);
		const size_t dst = size_t(Math::Round(kf*float(mPeriods)*Math::Sqrt(1.0f + inharm*kf*kf)));
		if(mModeCount > 0 && size_t(mModeBin[mModeCount - 1]) == dst) continue;
		const float f = kf*freq;
		const float w = 2.0f*float(Math::PI)*f/float(sampleRate);
		const float h = (1.0f - sf)/Math::Sqrt(1.0f - 2.0f*sf*Math::Cos(w) + sf*sf);
		float rate = -freq*Math::Log(h) + expCoeff;
		if(dampSlope > 0) rate += dampSlope*(f - freq)*0.001f;
		if(rateCap > 0 && rate > rateCap) rate = rateCap;
		if(k == 1) rate1 = rate;
		mModeBin[mModeCount] = uint(dst);
		mModeDecay[mModeCount] = Math::Exp(-rate*float(mPeriods)*precisePeriod/float(sampleRate));
		mModeK[mModeCount] = kf;
		mModeCount++;
	}

	// Форма периода 0 = обратный БПФ спектра щипка = сам щипок (round-trip,
	// InplaceInverseFFT нормализует делением на mN). Кладём её в оба буфера:
	// на первом периоде кроссфейда нет (mHavePrev == false), а при первом
	// advancePeriod() Cpp::Swap сделает её «предыдущей».
	mWaveA.SetCount(mN);
	mWaveB.SetCount(mN);
	mFftRe.SetCount(mN);
	mFftIm.SetCount(mN);
	for(size_t i = 0; i < mN; i++) { mFftRe[i] = mSpecRe[i]; mFftIm[i] = mSpecIm[i]; }
	Audio::InplaceInverseFFT(mFftRe.AsRange(), mFftIm.AsRange());
	for(size_t i = 0; i < mN; i++) { mWaveA[i] = mFftRe[i]; mWaveB[i] = mFftRe[i]; }

	// Спад звука живёт в спектре (advancePeriod), поэтому в аудио-тракте
	// уровень постоянный: mVolume — только счётчик индикатора UI, он спадает по
	// скорости фундаментала (тогда индикатор показывает реальное затухание
	// ноты). mOutGain — уровень после клипера.
	mExpStep = rate1 <= 0 ? 1.0f : Math::Exp(-rate1/float(sampleRate));
	mVolume = volume;
	mOutGain = scale;
	// Струна нормирована по RMS, поэтому velocity — сила удара прямо: слабый
	// удар не доходит до клипа, сильный режет.
	mPreGain = volume;
	// Перегруз (см. RenderInto): нормировка клипера так, чтобы наклон при
	// малом сигнале остался единичным (асимметрия сдвигает рабочую точку).
	// СУСТЕЙН КАНАЛА (Update 115): см. комментарий у mCompGain в заголовке.
	// 20 мс сглаживания: цель пересчитывается раз в БУФЕР (от 6 мс на C6 до 97 мс
	// на E2), и без сглаживания шаги были бы слышны как ступеньки громкости.
	mCompMax = sustainMax > 1.0f ? sustainMax : 1.0f;
	mCompGain = 1.0f;
	mCompTarget = 1.0f;
	mCompInv = 1.0f;
	mCompInvTarget = 1.0f;
	mCompCoef = 1.0f - Math::Exp(-1.0f/(0.02f*float(sampleRate)));
	mCompRef = 0.0f;
	mCompPeak = 0.0f;
	mDrive = drive;
	mBias = bias;
	mBiasShift = bias/(1.0f + Math::Abs(bias));
	mDriveNorm = (1.0f + Math::Abs(bias))*(1.0f + Math::Abs(bias));
	// Разделительный конденсатор после клипера (Update 114). r = e^(−2π·fc/SR):
	// эквивалент однополюсного ФВЧ y = x − x₁ + r·y₁ с срезом fc. 0 — выключен.
	mHpX1 = mHpY1 = 0;
	mHpR = postHpHz > 0
		? Math::Exp(-2.0f*float(Math::PI)*Math::Clamp(postHpHz, 1.0f, 2000.0f)/float(sampleRate))
		: 0.0f;
	// Кабинет/микрофон: резонансный ФНЧ 2-го порядка (RBJ lowpass) — горб Q на
	// резонансе диффузора и спад 12 дБ/окт выше него. Q ≤ 0.707 — гладкий
	// спад без горба (для сравнения), toneCutoff ≤ 0 — кабинет отключён.
	mCabPoles = cabPoles >= 1.5f ? 2 : 1;
	mCabX1 = mCabX2 = mCabY1 = mCabY2 = 0;
	mCabX1b = mCabX2b = mCabY1b = mCabY2b = 0;
	if(toneCutoff > 0)
	{
		const float fc = Math::Clamp(toneCutoff, 20.0f, float(sampleRate)*0.45f);
		const float w0 = 2.0f*float(Math::PI)*fc/float(sampleRate);
		const float cw = Math::Cos(w0);
		const float sw = Math::Sin(w0);
		const float alpha = sw/(2.0f*Math::Max(cabQ, 0.1f));
		const float invA0 = 1.0f/(1.0f + alpha);
		mCabB0 = (1.0f - cw)*0.5f*invA0;
		mCabB1 = (1.0f - cw)*invA0;
		mCabB2 = mCabB0;
		mCabA1 = -2.0f*cw*invA0;
		mCabA2 = (1.0f - alpha)*invA0;
	}
	else
	{
		mCabB0 = 1.0f;
		mCabB1 = mCabB2 = mCabA1 = mCabA2 = 0;
	}
	mShaped = drive > 0;
	// ТЕМБР-СТЕК ПОСЛЕ КАБИНЕТА (Update 126): однополюсная полка ВЧ. См.
	// комментарий у GuitarTweaks.PresenceDb. Присутствие 2-4 кГц у банка на
	// низах ВЫШЕ фундаментала (E3 атака: −0.2 дБ отн. h1), и до этой правки
	// единственным источником такой яркости был клипер — отсюда «или тёмно,
	// или дребезжит». Полка даёт ту же яркость линейно, то есть без нового
	// насыщения: замер .scratch/u126-tone-post.mjs (+12 дБ @2.5 кГц) — полоса
	// 2-4 кГц в атаке E5 −10.4 → −2.5 дБ (банк −2.7) и flat% 19.4 → 6.0
	// (банк 9.0); E6 −5.9 → +1.2 (банк +6.3), flat% 24.2 → 6.1 (банк 9.1).
	mPresenceOn = presenceHz > 0 && presenceDb != 0;
	mPresenceGain = 0;
	mPresenceLp = 0;
	mPresenceLpState = 0;
	if(mPresenceOn)
	{
		const float fc = Math::Clamp(presenceHz, 20.0f, float(sampleRate)*0.45f);
		mPresenceLp = 1.0f - Math::Exp(-2.0f*float(Math::PI)*fc/float(sampleRate));
		mPresenceGain = Math::Pow(10.0f, presenceDb/20.0f) - 1.0f;
	}
	// ФОРМАНТНЫЙ ТРАКТ (Update 150): три секции RBJ с ФИКСИРОВАННЫМИ (по Гц)
	// частотами — полка низа, пик присутствия, полка верха. Числа берутся НЕ по
	// регионам: это окраска самого тракта, одинаковая на всех клавишах.
	// Выключенное звено (все усиления нулевые) не считается совсем — рендер
	// остаётся прежним бит-в-бит, как и обещает комментарий у mToneOn.
	mToneOn = tone.Low.Db != 0.0f || tone.Peak.Db != 0.0f || tone.High.Db != 0.0f;
	mT1X1 = mT1X2 = mT1Y1 = mT1Y2 = 0;
	mT2X1 = mT2X2 = mT2Y1 = mT2Y2 = 0;
	mT3X1 = mT3X2 = mT3Y1 = mT3Y2 = 0;
	if(mToneOn)
	{
		ToneBiquad(mT1B0, mT1B1, mT1B2, mT1A1, mT1A2, tone.Low.Hz, tone.Low.Db, tone.Low.Q, 0, sampleRate);
		ToneBiquad(mT2B0, mT2B1, mT2B2, mT2A1, mT2A2, tone.Peak.Hz, tone.Peak.Db, tone.Peak.Q, 1, sampleRate);
		ToneBiquad(mT3B0, mT3B1, mT3B2, mT3A1, mT3A2, tone.High.Hz, tone.High.Db, tone.High.Q, 2, sampleRate);
	}
	else
	{
		mT1B0 = mT2B0 = mT3B0 = 1.0f;
		mT1B1 = mT1B2 = mT1A1 = mT1A2 = 0.0f;
		mT2B1 = mT2B2 = mT2A1 = mT2A2 = 0.0f;
		mT3B1 = mT3B2 = mT3A1 = mT3A2 = 0.0f;
	}
#ifdef INTRA_UI_METERS
	mInvInitialVolume = mVolume > 0 ? 1.0f/mVolume : 0.0f;
#endif
}

void SpectralStringSampler::advancePeriod()
{
	// Затухание — в спектре: каждая мода за БУФЕР домножается на свой
	// коэффициент затухания (Updates 106-113), после чего обратный БПФ даёт форму
	// следующего буфера. ЧАСТОТЫ мод не трогаются вообще: они впечатаны в бины при
	// создании голоса — в этом и состоит настоящая негармоничность (Update 113).
	//
	// Цикл идёт по СПИСКУ МОД (mModeBin/mModeDecay/mModeK), а не по всем mN бинам,
	// а зеркальный бин получается СОПРЯЖЕНИЕМ: эрмитова симметрия (и
	// вещественность сигнала) сохраняется точно, без второй тригонометрии.
	const size_t n = mN;
	// СУСТЕЙН КАНАЛА (Update 115): цель усиления на входе клипера — опорный пик
	// ноты, делённый на пик текущего буфера; после клипера — ровно обратное.
	// Пока нота на опорном уровне (атака), усиление 1 и ничего не меняется.
	if(mCompMax > 1.0f)
	{
		if(mCompRef <= 0.0f) mCompRef = mCompPeak;
		float target = 1.0f;
		if(mCompPeak > 1.0e-7f && mCompRef > mCompPeak)
		{
			target = mCompRef/mCompPeak;
			if(target > mCompMax) target = mCompMax;
		}
		mCompTarget = target;
		mCompInvTarget = 1.0f/target;
		mCompPeak = 0.0f;
	}
	// БИЕНИЯ ПОЛЯРИЗАЦИЙ (Update 112). Угол модуляции моды k — k·mBeatAngle, где
	// mBeatAngle растёт на 2π·BeatSplit·mPeriods за БУФЕР, поэтому скорость
	// биения моды k равна k·BeatSplit·f₀ ∝ еë частоте (как у пары поляризаций или
	// удвоенной дорожки, где расстройка тоже относительная). 2.399963 — константа
	// сдвига: она разводит начальные фазы биения разных мод (без таблиц и без
	// ветвлений).
	//
	// ВАЖНО: спектр НАКОПИТЕЛЬНЫЙ (каждый буфер домножается на предыдущий),
	// поэтому в него нельзя вносить саму модуляцию — тогда она ПЕРЕМНОЖАЛАСЬ бы
	// буфер за буфером (замер .scratch/guitar-beatsweep112.mjs: глубина 0.05
	// давала полную амплитудную модуляцию — это не биения, а тремolo). Вносится
	// ОТНОШЕНИЕ коэффициентов текущего и предыдущего буфера: тогда амплитуда
	// моды следует за 1 + depth·cos в любой момент времени, а не накапливает его.
	// Коэффициент нормирован так, что в начале ноты (фаза 0) он равен 1 — форма
	// щипка и уровень не меняются.
	if(mBeatDepth > 0)
	{
		mBeatPhase += mBeatPhaseStep;
		if(mBeatPhase >= 1.0f) mBeatPhase -= 1.0f;
	}
	const float beatDepth = mBeatDepth;
	const float beatAngle = 2.399963f + 2.0f*float(Math::PI)*mBeatPhase;
	const float beatStep = mBeatStep;
	const size_t count = mModeCount;
	for(size_t j = 0; j < count; j++)
	{
		const size_t bin = size_t(mModeBin[j]);
		float d = mModeDecay[j];
		if(beatDepth > 0)
		{
			const float kn = mModeK[j];
			const float a = kn*beatAngle;
			d *= (1.0f + beatDepth*Math::Cos(a))/(1.0f + beatDepth*Math::Cos(a - kn*beatStep));
		}
		const float re = mSpecRe[bin]*d;
		const float im = mSpecIm[bin]*d;
		mSpecRe[bin] = re;
		mSpecIm[bin] = im;
		if(bin*2 == n)
		{
			// Бин Найквиста (буфер — степень двойки): у вещественного сигнала он
			// вещественный, пара к нему не строится.
			continue;
		}
		mSpecRe[n - bin] = re;
		mSpecIm[n - bin] = -im;
	}
	for(size_t k = 0; k < n; k++)
	{
		mFftRe[k] = mSpecRe[k];
		mFftIm[k] = mSpecIm[k];
	}
	Audio::InplaceInverseFFT(mFftRe.AsRange(), mFftIm.AsRange());
	// Пинг-понг: старый текущий период становится предыдущим (для кроссфейда),
	// новый записывается в освободившийся буфер.
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

size_t SpectralStringSampler::GenerateStereoWithEnvelope(Span<float> ioDstLeft,
	Span<float> ioDstRight, const EnvelopeSegment& envelope)
{
	const size_t n = Math::Min(ioDstLeft.Length(), ioDstRight.Length());
	float* dstL = ioDstLeft.Data();
	float* dstR = ioDstRight.Data();
	RenderEnvelope gain(envelope);
	RenderInto(n, [dstL, dstR, &gain](float v) mutable
	{
		const float s = v*gain.NextGain();
		*dstL++ += s;
		*dstR++ += s;
	});
	return n;
}

INTRA_WARNING_POP
