#pragma once



#include "Intra/Math/SineRange.h"

#include "Intra/Range/Span.h"

#include "Utils/FixedArray.h"

#include "Types.h"
#include "Filter.h"
#include "WaveTable.h"
#include "Envelope.h"
#include "ExponentialAttenuation.h"
#include "Sampler.h"
#include "Instrument.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

/// Осциллятор модуляции с «дрожанием» (Update 66). Владелец: «а может оно быть
/// другой формы, например, не синусом, а мягче?». Замер (широкая полоса
/// 300-4000 Гц, 2 с сустейна): у банка модуляция НЕрегулярна — индекс
/// тональности (когерентный пик / (rms·√2)) 0.66-0.70 против 1.00 у чистого
/// синуса, а когерентная частота гуляет между блоками 2.7-6.1 Гц. Жёсткий
/// синус фиксированной глубины читается как механическое качание и звучит
/// ГЛУБЖЕ, чем такая же по rms нерегулярная модуляция. Здесь к несущей
/// к несущей добавлено медленное «дрожание» ГЛУБИНЫ от (1−Jitter) до
/// (1+Jitter) с частотой JitterFreq (по умолчанию 0.55 Гц — заведомо не
/// кратная ни 4-6 Гц вибрато, ни чему-либо в спектре ноты). Дрожание
/// считается фазовым аккумулятором со сглаженным треугольником: SineRange
/// на такой частоте вырождается в линейный рост (Update 66b).
/// При Jitter == 0 выход побитово прежний (множитель ровно 1).
struct WobbleOscillator
{
	Intra::SineRange<float> Carrier;
	// Update 70: гармоники формы модуляции (см. Vibrato::Harm2). Строятся на
	// ТОЙ ЖЕ фазе, что и несущая, поэтому форма модуляции становится
	// «импульсной» (у банка 3-я и 5-я гармоники пан-флейты равны основной).
	Intra::SineRange<float> Carrier2;
	Intra::SineRange<float> Carrier3;
	Intra::SineRange<float> Carrier4;
	Intra::SineRange<float> Carrier5;
	float Harmonic2 = 0;
	float Harmonic3 = 0;
	float Harmonic4 = 0;
	float Harmonic5 = 0;
	// 1/(1 + h2 + h3 + h4 + h5) — форма остаётся в ±1, а RMS-глубину инструмент
	// компенсирует множителем (1+Σh)/sqrt(1+Σh²) в Value/Tremolo.
	float Norm = 1;
	// Update 66b: медленное «дрожание» глубины — фазовый аккумулятор, а НЕ
	// второй SineRange. SineRange на 0.55 Гц вырождается (см. заголовок
	// патча): 2·cos(dphi) округляется до ровно 2.0 и рекуррентность даёт
	// линейный рост вместо синуса. Фаза копится в меньшем числе, поэтому
	// точность не теряется никогда.
	float WobblePhase = 0;  // рад, [0, 2π)
	float WobbleStep = 0;   // рад/семпл
	float Value = 0;
	float Jitter = 0;

	WobbleOscillator(decltype(nullptr) = nullptr) {}

	WobbleOscillator(float amplitude, float phase, float deltaPhase,
		float jitter, float jitterDeltaPhase,
		float harm2 = 0, float harm3 = 0, float harm4 = 0, float harm5 = 0):
		Carrier(amplitude, phase, deltaPhase),
		Carrier2(amplitude, phase, 2.0f*deltaPhase),
		Carrier3(amplitude, phase, 3.0f*deltaPhase),
		Carrier4(amplitude, phase, 4.0f*deltaPhase),
		Carrier5(amplitude, phase, 5.0f*deltaPhase),
		Harmonic2(harm2), Harmonic3(harm3), Harmonic4(harm4), Harmonic5(harm5),
		Norm(1.0f/(1.0f + harm2 + harm3 + harm4 + harm5)),
		WobbleStep(jitterDeltaPhase),
		Value(amplitude), Jitter(jitter) {}

	INTRA_FORCEINLINE float Next()
	{
		const float twoPi = 2.0f*float(PI);
		WobblePhase += WobbleStep;
		if(WobblePhase >= twoPi) WobblePhase -= twoPi;
		const float p = WobblePhase*(1.0f/float(PI));       // 0..2
		const float t = p < 1.0f? p: 2.0f - p;              // 0..1 (треугольник)
		const float w = ((3.0f - 2.0f*t)*t)*t*2.0f - 1.0f;  // сглаженный, ±1
		float s = Carrier.Next();
		// Update 70: при всех Harmonic_* == 0 ветви не исполняются, а Norm
		// равен ровно 1 — выход побитово прежний.
		if(Harmonic2 != 0) s += Harmonic2*Carrier2.Next();
		if(Harmonic3 != 0) s += Harmonic3*Carrier3.Next();
		if(Harmonic4 != 0) s += Harmonic4*Carrier4.Next();
		if(Harmonic5 != 0) s += Harmonic5*Carrier5.Next();
		return s*Norm*Value*(1.0f + Jitter*w);
	}
};

/// Класс, использующийся для синтеза большинства нот.
/// В целях выжать максимальную производительность, он пытается выполнять синтез за один проход.
/// В силу этого он берёт на себя довольно много ответственности и получился довольно сложным и громоздким.
/// Это базовый класс, который использует только внешние волновые таблицы, и,
/// соответственно, не выделяет внешней памяти, и не поддерживает изменение спектра со временем
class WaveTableSampler: public Sampler
{
protected:
	//Указывает на актуальные данные семплов
	const float* mSampleFragmentStart;
	unsigned mSampleFragmentLength;

	INTRA_FORCEINLINE Span<const float> SampleFragment() const
	{return {mSampleFragmentStart, mSampleFragmentLength};}
	
	INTRA_FORCEINLINE Span<const float> SampleFragment(size_t startIndex, size_t maxCount) const
	{return SampleFragment().Drop(startIndex).Take(maxCount);}

	//Смещение левого канала относительно начала периода mSampleFragment
	float mFragmentOffset;

	//Целая часть смещения правого канала относительно периода mRightSampleFragment
	unsigned mRightFragmentOffset;

	//Скорость воспроизведения семпла mSampleFragment
	float mRate;

	//Объединяет в себе все факторы, влияющие на громкость ноты, кроме Envelope, панорамы и реверберации.
	//Factor - текущий множитель амплитуды для mSampleFragment.
	//FactorStep - множитель, на который умножается амплитуда - либо каждый семпл,
	//либо каждый проход по фрагменту - второй вариант встречается у наследника WaveFormSampler,
	//который в некоторых случаях может заранее наложить экспоненциальное затухание на хранимые в нём семплы
	ExponentAttenuator mExpAtten;

	//Множители, на которые умножается каждый семпл при записи в соответствующий канал
	float mLeftMultiplier, mRightMultiplier;

	//Осциллятор скорости воспроизведения, которая рассчитывается как mRate*(1 + mFreqOscillator.value).
	//Обёртка с дрожанием глубины (Update 66), интерфейс Next() тот же.
	WobbleOscillator mFreqOscillator;

	// Амплитуда ЧМ (прежний vibratoValue) и амплитуда АМ того же LFO
	// (Update 64c): осциллятор нормирован до ±1, поэтому масштабы вынесены
	// сюда. Tremolo = 0 — поведение прежнее, байт-в-байт.
	float mVibratoValue = 0;
	float mVibratoTremolo = 0;

	//Вибрато включено (амплитуда и частота ненулевые) — рендер идёт через
	//скалярное вибрато-ядро вместо SIMD-ядер с постоянной скоростью.
	bool mHasVibrato = false;

	// Задержка появления вибрато (в сэмплах от начала ноты) и длительность
	// плавного входа (в сэмплах): глубина вибрато умножается на gate(t),
	// линейно растущий от 0 до 1 на отрезке [Delay, Delay+Ramp]. При Ramp == 0
	// вибрато включается мгновенно после Delay. Так флейта «дышит» не с первой
	// миллисекунды атаки, а постепенно, как в живом исполнении.
	float mVibratoDelaySamples = 0;
	float mVibratoRampSamples = 0;
	unsigned mElapsedSamples = 0;

	//Огибающая ноты, например ADSR. Не включает в себя экспоненциальное затухание, оно накладывается после этого.
	Envelope mEnvelope;

public:
	WaveTableSampler(decltype(nullptr)=nullptr) {}

	WaveTableSampler(Span<const float> periodicWave, float rate, float expCoeff,
		float volume, float vibratoDeltaPhase, float vibratoValue, const Envelope& envelope,
		size_t channelDeltaSamples, float vibratoDelaySamples = 0, float vibratoRampSamples = 0,
		float vibratoTremolo = 0, float vibratoJitter = 0, float vibratoJitterDeltaPhase = 0,
		float vibratoHarm2 = 0, float vibratoHarm3 = 0,
		float vibratoHarm4 = 0, float vibratoHarm5 = 0);

	// Текущее значение gate-множителя вибрато для сэмпла с индексом elapsed
	// (0 до Delay, затем линейный вход до 1 за Ramp сэмплов).
	INTRA_FORCEINLINE float VibratoGate() const
	{
		if(mVibratoRampSamples > 0)
		{
			if(mElapsedSamples >= mVibratoDelaySamples)
			{
				const float g = (mElapsedSamples - mVibratoDelaySamples)/mVibratoRampSamples;
				return g < 1.0f ? g : 1.0f;
			}
			return 0;
		}
		return mElapsedSamples >= mVibratoDelaySamples ? 1.0f : 0.0f;
	}
	INTRA_FORCEINLINE float VibratoGateStep() const
	{
		return mVibratoRampSamples > 0 ? 1.0f/mVibratoRampSamples : 0.0f;
	}

	void MoveConstruct(void* dst) override {new(dst) WaveTableSampler(Move(*this));}

	virtual bool OwnExponentialAttenuatedDataArray() const noexcept {return false;}

	bool Generate(SamplerTaskContainer& dstTasks, size_t offsetInSamples, size_t numSamples) override;

	// Прямой рендер, используемый вложенными семплерами (NoteSampler).
	// GenerateMono возвращает необработанный остаток (nullptr, если буфер заполнен).
	Span<float> GenerateMono(Span<float> ioDst);
	size_t GenerateStereo(Span<float> dstLeft, Span<float> dstRight);

	void MultiplyPitch(float freqMultiplier) final
	{
		mRate *= freqMultiplier;
		if(Abs(mRate - 1) < 0.0001f) mRate = 1;
	}

	void MultiplyVolume(float volumeMultiplier) final {mExpAtten.Factor *= volumeMultiplier;}

	void SetPan(float newPan) final
	{
		mRightMultiplier = (newPan + 1) / 2;
		mLeftMultiplier = 1 - mRightMultiplier;
	}

	void NoteRelease() final {mEnvelope.StartLastSegment();}

#ifdef INTRA_UI_METERS
	/// Уровень огибающей ноты для индикатора в веб-UI (см. Sampler::GetLevel).
	float GetLevel() const override {return mEnvelope.CurrentSegment.Volume;}
#endif

private:
	size_t renderDirect(Span<float> dstLeft, Span<float> dstRight);

	void generateWithDefaultRate(SamplerTaskContainer& dstTasks, size_t offsetInSamples, size_t numSamples);

	void generateWithVaryingRate(SamplerTaskContainer& dstTasks, size_t offsetInSamples, size_t numSamples);

	template<bool FreqOsc, bool Adsr>
	void generateWithVaryingRateTask(Span<float> dstLeft, Span<float> dstRight);
};

struct WaveTableCache
{
	typedef Delegate<WaveTable(float freq, unsigned sampleRate)> GeneratorType;
	mutable Array<WaveTable> Tables;
	GeneratorType Generator;
	bool AllowMipmaps = false;

	WaveTable& Get(float freq, unsigned sampleRate) const;

	WaveTableCache() {}
	WaveTableCache(const WaveTableCache&) = delete;
	WaveTableCache& operator=(const WaveTableCache&) = delete;
	WaveTableCache(WaveTableCache&&) = default;
	WaveTableCache& operator=(WaveTableCache&&) = default;
};

/// Параметры вибрато для одной ноты. Value — относительное отклонение
/// скорости чтения (как VibratoValue: 0.005 = ±0.5 % ≈ ±8.7 центов);
/// Delay/Ramp — в секундах, плавное появление после атаки.
/// Tremolo (Update 64) — ОДНОВРЕМЕННАЯ амплитудная модуляция того же LFO:
/// относительное отклонение громкости (0.35 = ±35 % ≈ +2.6/−3.1 дБ). Владелец
/// слышал вибрато флейт как «завывание привидения», а замер полосы h1 банка
/// (.scratch/hvib.mjs) дал у флейты 43 когерентную АМ 2.7–3.4 дБ при том, что
/// чистой ЧМ-линии там нет вовсе: у оригинала «дрожит» дыхание, а не высота.
/// Дыхание — слой NoiseSampler, ему АМ и достаётся: у вейвтейбла вычислитель
/// не тронут, поэтому тон остаётся РОВНЫМ (что владелец и просил).
struct Vibrato
{
	float Frequency = 0; // Гц
	float Value = 0;
	float Tremolo = 0;
	float Delay = 0; // с
	float Ramp = 0;  // с
	/// Разброс ГЛУБИНЫ модуляции (Update 66): 0 — чистый синус, 0.5 — глубина
	/// плавает ±50 % с частотой JitterFreq. У банка модуляция нерегулярна
	/// (индекс тональности 0.66-0.70 против 1.0 у синуса).
	float Jitter = 0;
	float JitterFreq = 0.55f; // Гц
	/// Update 70: доли 2..5-й гармоник ФОРМЫ модуляции (0 — чистый синус).
	/// У банка модулятор не синус, и ВСЕ его линии — целые кратные основной
	/// (C4: 4.04 Гц 0.145 + 12.11 0.152 + 20.19 0.128, то есть 1:1:0.9 по
	/// амплитуде; C5: 5.55 + 11.02 + 22.04; C6: 8.50 + 17.08 — 2-я на −5.7 дБ).
	/// Такая «импульсная» форма при той же основной частоте слышится как
	/// заметно более частое тремоло: эффективная скорость (спектральный
	/// центроид) у банка на C4-C6 ровно 10.3-12.6 Гц, а у чистого синуса
	/// 4-8.6 Гц. Движок нормирует форму до ±1, поэтому задавший гармоники
	/// инструмент должен домножить Value и Tremolo на
	/// (1+Σh)/sqrt(1+Σh²): RMS-глубина тогда не меняется, растёт пик.
	float Harm2 = 0;
	float Harm3 = 0;
	float Harm4 = 0;
	float Harm5 = 0;
};

/// Общий LFO вибрато одной ноты (Update 59). Математика ровно та же, что у
/// WaveTableSampler: SineRange с амплитудой Value, фазой 0 и шагом
/// 2π·Frequency/sampleRate плюс gate(Delay/Ramp). Слой, стартующий вместе с
/// телом ноты и получающий по одному вызову Next() на каждый выходной сэмпл,
/// остаётся с ним В ФАЗЕ — поэтому юбки/дыхание/блум «дышат» вместе с тоном.
/// До этого вибрато было только у вейвтейбла: жёсткие линии дыхания и блума
/// бились с уходящим тоном (владелец слышал «гудение на C4, как будто насос
/// работает, но с юбкой»), а сам тон стоял на месте — «не слышу вибрато».
struct VibratoLfo
{
	WobbleOscillator Oscillator;
	float Value = 0;
	float Tremolo = 0;
	// Нормированное (±1) значение LFO текущего сэмпла с учётом гейта — его
	// читает АМ-ветвь (NoiseSampler) сразу после Next().
	float LastGated = 0;
	float DelaySamples = 0;
	float RampSamples = 0;
	unsigned ElapsedSamples = 0;
	bool Active = false;

	void Init(const Vibrato& v, unsigned sampleRate)
	{
		if((v.Value == 0.0f && v.Tremolo == 0.0f) || v.Frequency == 0.0f) return;
		// Осциллятор нормирован до ±1: масштаб Value применяется в Next().
		Oscillator = WobbleOscillator(1.0f, 0.0f,
			2.0f*float(PI)*v.Frequency/float(sampleRate), v.Jitter,
			2.0f*float(PI)*v.JitterFreq/float(sampleRate),
			v.Harm2, v.Harm3, v.Harm4, v.Harm5);
		Value = v.Value;
		Tremolo = v.Tremolo;
		DelaySamples = v.Delay*float(sampleRate);
		RampSamples = v.Ramp*float(sampleRate);
		Active = true;
	}

	INTRA_FORCEINLINE float Gate() const
	{
		if(RampSamples > 0)
		{
			if(float(ElapsedSamples) >= DelaySamples)
			{
				const float g = (float(ElapsedSamples) - DelaySamples)/RampSamples;
				return g < 1.0f ? g : 1.0f;
			}
			return 0;
		}
		return float(ElapsedSamples) >= DelaySamples ? 1.0f : 0.0f;
	}

	/// Относительное отклонение скорости на один сэмпл (0 = без вибрато).
	/// Заодно кладёт нормированное (±1) значение LFO в LastGated для АМ-ветви.
	INTRA_FORCEINLINE float Next()
	{
		LastGated = Oscillator.Next()*Gate();
		ElapsedSamples++;
		return LastGated*Value;
	}
};

struct WaveTableInstrument
{
	WaveTableCache* Tables = nullptr;
	float ExpCoeff = 0;
	float VolumeScale = 0;
	float VibratoFrequency = 0;
	float VibratoValue = 0;
	EnvelopeFactory Envelope = EnvelopeFactory::Constant(1);

	// Необязательный посегментный профиль вибрато: если задан, VibratoFrequency/
	// VibratoValue игнорируются, а параметры считаются по частоте ноты
	// (регистровая зависимость — флейта).
	Funal::CopyableDelegate<Vibrato(float freq)> VibratoProfile;

	// Необязательный посегментный профиль огибающей по частоте ноты: если
	// задан, Envelope перекрывается, а форма (длительности/уровни сегментов)
	// считается по частоте — регистровая зависимость атаки/раздува
	// (записанное начало ноты флейты).
	Funal::CopyableDelegate<EnvelopeFactory(float freq)> EnvelopeProfile;

	WaveTableSampler operator()(float freq, float volume, unsigned sampleRate) const;
};

/// Задача по генерации семплов, которая прибавляет суммирует сгенерированные значения в буфер указанного контекста.
INTRA_WARNING_POP
