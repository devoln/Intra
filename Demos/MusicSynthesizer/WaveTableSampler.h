#pragma once



#include "Math/SineRange.h"

#include "Core/Range/Span.h"

#include "Utils/FixedArray.h"

#include "Types.h"
#include "Filter.h"
#include "WaveTable.h"
#include "Envelope.h"
#include "ExponentialAttenuation.h"
#include "Sampler.h"
#include "Instrument.h"

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
	Intra::Math::SineRange<float> mFreqOscillator;

	//Огибающая ноты, например ADSR. Не включает в себя экспоненциальное затухание, оно накладывается после этого.
	Envelope mEnvelope;

public:
	WaveTableSampler(null_t=null) {}

	WaveTableSampler(CSpan<float> periodicWave, float rate, float expCoeff,
		float volume, float vibratoDeltaPhase, float vibratoValue, const Envelope& envelope, size_t channelDeltaSamples);

	void MoveConstruct(void* dst) override {new(dst) WaveTableSampler(Move(*this));}

	bool OwnExponentialAttenuatedDataArray() const noexcept {return false;}

	bool Generate(SamplerTaskContainer& dstTasks, size_t offsetInSamples, size_t numSamples) override;

	void MultiplyPitch(float freqMultiplier) final
	{
		mRate *= freqMultiplier;
		if(Math::Abs(mRate - 1) < 0.0001f) mRate = 1;
	}

	void MultiplyVolume(float volumeMultiplier) final {mExpAtten.Factor *= volumeMultiplier;}

	void SetPan(float newPan) final
	{
		mRightMultiplier = (newPan + 1) / 2;
		mLeftMultiplier = 1 - mRightMultiplier;
	}

	void SetReverbCoeff(float newCoeff) final {mReverbMultiplier = newCoeff;}
	void NoteRelease() final {mEnvelope.StartLastSegment();}

private:
	void generateWithDefaultRate(SamplerTaskContainer& dstTasks, size_t offsetInSamples, size_t numSamples);

	void generateWithVaryingRate(SamplerTaskContainer& dstTasks, size_t offsetInSamples, size_t numSamples);

	template<bool FreqOsc, bool Adsr>
	void generateWithVaryingRateTask(Span<float> dstLeft, Span<float> dstRight, Span<float> dstReverb);
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

//! Задача по генерации семплов, которая прибавляет суммирует сгенерированные значения в буфер указанного контекста.
class WaveSamplerTask
{
public:
	enum Flag: byte
	{
		LeftChannel = 1, RightChannel = 2, ReverbChannel = 4,
		ChannelMask = LeftChannel|RightChannel|ReverbChannel
	};

	ushort OffsetInSamples, NumSamples;

	SamplerTask(size_t offsetInSamples, size_t numSamples):
		OffsetInSamples(ushort(offsetInSamples)), NumSamples(ushort(numSamples))
	{}

	virtual ~SamplerTask() {}
	virtual void MoveConstruct(void* dst) = 0;
	virtual void operator()(SamplerTaskContext& stc) = 0;
};

INTRA_WARNING_POP
