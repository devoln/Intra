#pragma once

#include "Cpp/Warnings.h"

#include "Math/SineRange.h"

#include "Utils/Span.h"

#include "Utils/FixedArray.h"

#include "Types.h"
#include "Filter.h"
#include "WaveTable.h"
#include "Envelope.h"
#include "ExponentialAttenuation.h"
#include "Sampler.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

//! Класс, использующийся для синтеза большинства нот.
//! В целях выжать максимальную производительность, он пытается выполнять синтез за один проход.
//! В силу этого он берёт на себя довольно много ответственности и получился довольно сложным и громоздким.
//! Это базовый класс, который использует только внешние волновые таблицы, и,
//! соответственно, не выделяет внешней памяти, и не поддерживает изменение спектра со временем
class WaveTableSampler: public Sampler
{
protected:
	//Указывает на актуальные данные семплов
	const float* mSampleFragmentStart;
	uint mSampleFragmentLength;

	forceinline CSpan<float> SampleFragment() const
	{return {mSampleFragmentStart, mSampleFragmentLength};}
	
	forceinline CSpan<float> SampleFragment(size_t startIndex, size_t maxCount) const
	{return SampleFragment().Drop(startIndex).Take(maxCount);}

	//Смещение левого канала относительно начала периода mSampleFragment
	float mFragmentOffset;

	//Целая часть смещения правого канала относительно периода mRightSampleFragment
	uint mRightFragmentOffset;

	//Скорость воспроизведения семпла mSampleFragment
	float mRate;

	//Объединяет в себе все факторы, влияющие на громкость ноты, кроме Envelope, панорамы и реверберации.
	//Factor - текущий множитель амплитуды для mSampleFragment.
	//FactorStep - множитель, на который умножается амплитуда - либо каждый семпл,
	//либо каждый проход по фрагменту - второй вариант встречается у наследника WaveFormSampler,
	//который в некоторых случаях может заранее наложить экспоненциальное затухание на хранимые в нём семплы
	ExponentAttenuator mExpAtten;

	//Множители, на которые умножается каждый семпл при записи в соответствующий канал
	float mLeftMultiplier, mRightMultiplier, mReverbMultiplier;

	//Осциллятор скорости воспроизведения, которая рассчитывается как mRate*(1 + mFreqOscillator.value)
	Math::SineRange<float> mFreqOscillator;

	//Огибающая ноты, например ADSR. Не включает в себя экспоненциальное затухание, оно накладывается после этого.
	Envelope mEnvelope;

public:
	WaveTableSampler(null_t=null) {}

	WaveTableSampler(CSpan<float> periodicWave, float rate, float expCoeff,
		float volume, float vibratoDeltaPhase, float vibratoValue, const Envelope& envelope, size_t channelDeltaSamples);

	//TODO: убрать
	forceinline bool OwnDataArray() const noexcept {return false;}

	bool OwnExponentialAttenuatedDataArray() const noexcept {return false;}

	bool Generate(SamplerTaskDispatcher& taskDispatcher) override;

	void MultiplyPitch(float freqMultiplier) final
	{
		mRate *= freqMultiplier;
		if(Math::Abs(mRate - 1) < 0.0001f) mRate = 1;
	}

	void MultiplyVolume(float volumeMultiplier) final
	{
		mExpAtten.Factor *= volumeMultiplier;
	}

	void SetPan(float newPan) final
	{
		mRightMultiplier = (newPan + 1) / 2;
		mLeftMultiplier = 1 - mRightMultiplier;
	}

	void SetReverbCoeff(float newCoeff) final
	{
		mReverbMultiplier = newCoeff;
	}

	void NoteRelease() final
	{
		mEnvelope.StartLastSegment();
	}

private:
	void generateWithDefaultRate(Span<float> dstLeft, Span<float> dstRight, Span<float> dstReverb);

	void generateWithVaryingRate(Span<float> dstLeft, Span<float> dstRight, Span<float> dstReverb);

	template<bool FreqOsc, bool Adsr>
	void generateWithVaryingRate(Span<float> dstLeft, Span<float> dstRight, Span<float> dstReverb);
};

struct WaveTableTask
{

};

//! Семплер, который расширяет возможности базового класса,
//! позволяя хранить волновую таблицу у себя, таким образом, она может изменяться и эволюционировать
//! Недостаток - выделение памяти, копирование и удаление, что сильно снижает эффективность.
//! TODO: выделение волновых таблиц из пула должны ускорить это, но время на копирование всё равно будет ограничивать размер волновой таблицы
class WaveFormSampler: public WaveTableSampler
{
	//Множитель, используемый для усреднения семплов на каждом проходе по массиву в алгоритме Карплюс-Стронга
	float mSmoothingFactor;
	
	float mLastFragmentSample = 0;

	//Если mSmoothingFactor != 0, он эволюционирует на каждом проходе, и хранит свою предыдущую копию в одной из двух половин.
	FixedArray<float> mSampleFragmentData;

	//Указывает на начало данных семплов (размер тот же, что и mSampleFragment) для отстающего по фазе правого канала. При mSmoothingFactor == 0 совпадает с mSampleFragment
	uint mRightSampleFragmentStartIndex;

	//Если природа семплера такова, что он в процессе своей жизни меняет хранимые у себя семплы,
	//что не позволяет применить оптимизацию предварительного наложения экспоненциального затухания
	forceinline bool canDataMutate() const {return mSmoothingFactor != 0;}

	//Аналогично предыдущему, но дополнительно содержит старую копию семплов для второго - отстающего канала
	forceinline bool HasStereoMutatedData() const {return mSmoothingFactor != 0;}

	typedef void(*WaveForm)(const void* params, Span<float> dst, float freq, float volume, uint sampleRate);

	template<typename F> static void WaveFormWrapper(const void* params,
		Span<float> dst, float freq, float volume, uint sampleRate)
	{(*static_cast<const F*>(params))(dst, freq, volume, sampleRate);}


	CSpan<float> prepareInternalData(const void* params, WaveForm wave,
		float freq, float volume, uint sampleRate, bool goodPeriod, bool prepareToStereoDataMutation);

	//Если данные семплов, хранимые в этом семплере не меняются в процессе жизни семплера и периодически повторяются,
	//можно применить оптимизацию экспоненциального затухания, наложив его предварительно на эти данные.
	//Тогда впоследствии надо будет только каждый проход умножать громкость на константу
	void preattenuateExponential(float expCoeff, uint sampleRate);

	forceinline bool isExponentialPreattenuated() const {return !canDataMutate();}

	WaveFormSampler(const void* params, WaveForm wave,
		float attenuationPerSample, float volume,
		float freq, uint sampleRate, float vibratoFrequency, float vibratoValue,
		float smoothingFactor, const Envelope& envelope);

	//TODO: убрать
	forceinline bool OwnDataArray() const noexcept {return true;}
	forceinline bool IsAttenuatableDataArray() const noexcept {return mSmoothingFactor == 0;}

public:
	template<typename F, typename = Meta::EnableIf<
		Meta::IsCallable<F, Span<float>, float, float, uint>::_
		>> forceinline WaveFormSampler(const F& wave,
			float expCoeff, float volume, float freq, uint sampleRate,
			float vibratoFrequency, float vibratoValue, float smoothingFactor, const Envelope& envelope = null):
		WaveFormSampler(&wave, WaveFormWrapper<F>, expCoeff, volume,
			freq, sampleRate, vibratoFrequency, vibratoValue, smoothingFactor, envelope) {}


};

typedef CopyableDelegate<void(Span<float> dst, float freq, float volume, uint sampleRate)> WaveForm;

struct SineWaveForm
{
	void operator()(Span<float> dst, float freq, float volume, uint sampleRate) const;
};

struct SawtoothWaveForm
{
	float UpdownRatio;
	void operator()(Span<float> dst, float freq, float volume, uint sampleRate) const;
};

struct PulseWaveForm
{
	float UpdownRatio;
	void operator()(Span<float> dst, float freq, float volume, uint sampleRate) const;
};

struct WhiteNoiseWaveForm
{
	void operator()(Span<float> dst, float freq, float volume, uint sampleRate) const;
};

struct GuitarWaveForm
{
	float Demp;
	void operator()(Span<float> dst, float freq, float volume, uint sampleRate) const;
};

struct WaveInstrument
{
	WaveForm Wave = SineWaveForm();
	float Scale = 0;
	float ExpCoeff = 0;
	float FreqMultiplier = 1;
	uint Octaves = 1;
	float VibratoFrequency = 0;
	float VibratoValue = 0;
	float SmoothingFactor = 0;
	EnvelopeFactory Envelope = EnvelopeFactory::Constant(1);

	WaveFormSampler operator()(float freq, float volume, uint sampleRate) const;
};

struct WaveTableCache
{
	typedef Delegate<WaveTable(float freq, uint sampleRate)> GeneratorType;
	mutable Array<WaveTable> Tables;
	GeneratorType Generator;
	bool AllowMipmaps = false;

	WaveTable& Get(float freq, uint sampleRate) const;

	WaveTableCache() {}
	WaveTableCache(const WaveTableCache&) = delete;
	WaveTableCache& operator=(const WaveTableCache&) = delete;
	WaveTableCache(WaveTableCache&&) = default;
	WaveTableCache& operator=(WaveTableCache&&) = default;
};

struct WaveTableInstrument
{
	WaveTableCache* Tables = null;
	float ExpCoeff = 0;
	float VolumeScale = 0;
	float VibratoFrequency = 0;
	float VibratoValue = 0;
	EnvelopeFactory Envelope = EnvelopeFactory::Constant(1);

	WaveTableSampler operator()(float freq, float volume, uint sampleRate) const;
};

INTRA_WARNING_POP
