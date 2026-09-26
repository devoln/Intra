#pragma once

#include "WaveTableSampler.h"

/// Семплер, который расширяет возможности базового класса,
/// позволяя хранить волновую таблицу у себя, таким образом, она может изменяться и эволюционировать.
/// Недостаток - выделение памяти, копирование и удаление, что сильно снижает эффективность.
/// TODO: выделение волновых таблиц из пула должны ускорить это, но время на копирование всё равно будет ограничивать размер волновой таблицы.
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


	void prepareInternalData(const void* params, WaveForm wave,
		float freq, float volume, unsigned sampleRate, bool goodPeriod, bool prepareToStereoDataMutation);

	//Если данные семплов, хранимые в этом семплере не меняются в процессе жизни семплера и периодически повторяются,
	//можно применить оптимизацию экспоненциального затухания, наложив его предварительно на эти данные.
	//Тогда впоследствии надо будет только каждый проход умножать громкость на константу
	void preattenuateExponential(float expCoeff, unsigned sampleRate);

	INTRA_FORCEINLINE bool isExponentialPreattenuated() const { return !canDataMutate(); }

	// The choir/voice ensemble vibrato gained delay, entry and jitter. WaveFormSampler had none of it: vibrato was an instant pure sine from the start of the note, which the owner heard as a mechanical tremolo in ChoirAahs. Zero defaults keep the previous behaviour.
	WaveFormSampler(const void* params, WaveForm wave,
		float attenuationPerSample, float volume,
		float freq, unsigned sampleRate, float vibratoFrequency, float vibratoValue,
		float smoothingFactor, const Envelope& envelope,
		float vibratoDelay = 0, float vibratoRamp = 0,
		float vibratoJitter = 0, float vibratoJitterFrequency = 0.55f);

	//TODO: убрать
	INTRA_FORCEINLINE bool OwnDataArray() const noexcept { return true; }
	INTRA_FORCEINLINE bool IsAttenuatableDataArray() const noexcept { return mSmoothingFactor == 0; }

public:
	void MoveConstruct(void* dst) override { new(dst) WaveFormSampler(Cpp::Move(*this)); }

	bool OwnExponentialAttenuatedDataArray() const noexcept override { return mSmoothingFactor == 0; }

	template<typename F, typename = Meta::EnableIf<
		Meta::IsCallable<F, Span<float>, float, float, unsigned>::_
		>> INTRA_FORCEINLINE WaveFormSampler(const F& wave,
			float expCoeff, float volume, float freq, unsigned sampleRate,
			float vibratoFrequency, float vibratoValue, float smoothingFactor, const Envelope& envelope = Envelope::Constant(),
			float vibratoDelay = 0, float vibratoRamp = 0,
			float vibratoJitter = 0, float vibratoJitterFrequency = 0.55f):
		WaveFormSampler(&wave, WaveFormWrapper<F>, expCoeff, volume,
			freq, sampleRate, vibratoFrequency, vibratoValue, smoothingFactor, envelope,
			vibratoDelay, vibratoRamp, vibratoJitter, vibratoJitterFrequency)
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

struct WaveInstrument: public Instrument
{
	WaveForm Wave = SineWaveForm();
	float Scale = 0;
	float ExpCoeff = 0;
	float FreqMultiplier = 1;
	unsigned Octaves = 1;
	float VibratoFrequency = 0;
	float VibratoValue = 0;
	float SmoothingFactor = 0;
	// Gradual vibrato onset (s) and depth jitter (0 = a pure sine as before), needed by the choir ensemble: an instant sine from note-on read as mechanical tremolo.
	float VibratoDelay = 0;
	float VibratoRamp = 0;
	float VibratoJitter = 0;
	float VibratoJitterFrequency = 0.55f;
	EnvelopeFactory Envelope = EnvelopeFactory::Constant(1);

	void MoveConstruct(void* dst) override {new(dst) WaveInstrument(*this);}
	Sampler& CreateSampler(float freq, float volume, unsigned sampleRate,
		const NoteOnParams& noteParams, SamplerContainer& dst, uint16* oIndex = nullptr) const override;

	/// Временный call-интерфейс для вложенного использования в NoteSampler.
	WaveFormSampler operator()(float freq, float volume, unsigned sampleRate) const;
};
