#pragma once

#include "WaveTableSampler.h"

//! Семплер, который расширяет возможности базового класса,
//! позволяя хранить волновую таблицу у себя, таким образом, она может изменяться и эволюционировать.
//! Недостаток - выделение памяти, копирование и удаление, что сильно снижает эффективность.
//! TODO: выделение волновых таблиц из пула должны ускорить это, но время на копирование всё равно будет ограничивать размер волновой таблицы.
class WaveFormSampler: public WaveTableSampler
{
	//Множитель, используемый для усреднения семплов на каждом проходе по массиву в алгоритме Карплюс-Стронга
	float mSmoothingFactor;

	float mLastFragmentSample = 0;

	//Если mSmoothingFactor != 0, он эволюционирует на каждом проходе, и хранит свою предыдущую копию в одной из двух половин.
	FixedArray<float> mSampleFragmentData;

	//Указывает на начало данных семплов (размер тот же, что и mSampleFragment) для отстающего по фазе правого канала. При mSmoothingFactor == 0 совпадает с mSampleFragment
	unsigned mRightSampleFragmentStartIndex;

	//Если природа семплера такова, что он в процессе своей жизни меняет хранимые у себя семплы,
	//что не позволяет применить оптимизацию предварительного наложения экспоненциального затухания
	INTRA_FORCEINLINE bool canDataMutate() const { return mSmoothingFactor != 0; }

	//Аналогично предыдущему, но дополнительно содержит старую копию семплов для второго - отстающего канала
	INTRA_FORCEINLINE bool HasStereoMutatedData() const { return mSmoothingFactor != 0; }

	typedef void(*WaveForm)(const void* params, Span<float> dst, float freq, float volume, unsigned sampleRate);

	template<typename F> static void WaveFormWrapper(const void* params,
		Span<float> dst, float freq, float volume, unsigned sampleRate)
	{
		(*static_cast<const F*>(params))(dst, freq, volume, sampleRate);
	}


	CSpan<float> prepareInternalData(const void* params, WaveForm wave,
		float freq, float volume, unsigned sampleRate, bool goodPeriod, bool prepareToStereoDataMutation);

	//Если данные семплов, хранимые в этом семплере не меняются в процессе жизни семплера и периодически повторяются,
	//можно применить оптимизацию экспоненциального затухания, наложив его предварительно на эти данные.
	//Тогда впоследствии надо будет только каждый проход умножать громкость на константу
	void preattenuateExponential(float expCoeff, unsigned sampleRate);

	INTRA_FORCEINLINE bool isExponentialPreattenuated() const { return !canDataMutate(); }

	WaveFormSampler(const void* params, WaveForm wave,
		float attenuationPerSample, float volume,
		float freq, unsigned sampleRate, float vibratoFrequency, float vibratoValue,
		float smoothingFactor, const Envelope& envelope);

	//TODO: убрать
	INTRA_FORCEINLINE bool OwnDataArray() const noexcept { return true; }
	INTRA_FORCEINLINE bool IsAttenuatableDataArray() const noexcept { return mSmoothingFactor == 0; }

public:
	void MoveConstruct(void* dst) override { new(dst) WaveFormSampler(Cpp::Move(*this)); }

	template<typename F, typename = Meta::EnableIf<
		Meta::IsCallable<F, Span<float>, float, float, unsigned>::_
		>> INTRA_FORCEINLINE WaveFormSampler(const F& wave,
			float expCoeff, float volume, float freq, unsigned sampleRate,
			float vibratoFrequency, float vibratoValue, float smoothingFactor, const Envelope& envelope = null):
		WaveFormSampler(&wave, WaveFormWrapper<F>, expCoeff, volume,
			freq, sampleRate, vibratoFrequency, vibratoValue, smoothingFactor, envelope)
	{}


};

typedef CopyableDelegate<void(Span<float> dst, float freq, float volume, unsigned sampleRate)> WaveForm;

struct SineWaveForm
{
	void operator()(Span<float> dst, float freq, float volume, unsigned sampleRate) const;
};

struct SawtoothWaveForm
{
	float UpdownRatio;
	void operator()(Span<float> dst, float freq, float volume, unsigned sampleRate) const;
};

struct PulseWaveForm
{
	float UpdownRatio;
	void operator()(Span<float> dst, float freq, float volume, unsigned sampleRate) const;
};

struct WhiteNoiseWaveForm
{
	void operator()(Span<float> dst, float freq, float volume, unsigned sampleRate) const;
};

struct GuitarWaveForm
{
	float Demp;
	void operator()(Span<float> dst, float freq, float volume, unsigned sampleRate) const;
};

class WaveInstrument: public Instrument
{
	WaveForm Wave = SineWaveForm();
	float Scale = 0;
	float ExpCoeff = 0;
	float FreqMultiplier = 1;
	unsigned Octaves = 1;
	float VibratoFrequency = 0;
	float VibratoValue = 0;
	float SmoothingFactor = 0;
	EnvelopeFactory Envelope = EnvelopeFactory::Constant(1);

	Sampler& CreateSampler(float freq, float volume, unsigned sampleRate,
		SamplerContainer& dst, uint16* oIndex = null) const override;
};
