#pragma once

#include "WaveTableSampler.h"

/// �������, ������� ��������� ����������� �������� ������,
/// �������� ������� �������� ������� � ����, ����� �������, ��� ����� ���������� � ����������������.
/// ���������� - ��������� ������, ����������� � ��������, ��� ������ ������� �������������.
/// TODO: ��������� �������� ������ �� ���� ������ �������� ���, �� ����� �� ����������� �� ����� ����� ������������ ������ �������� �������.
class WaveFormSampler: public WaveTableSampler
{
	//���������, ������������ ��� ���������� ������� �� ������ ������� �� ������� � ��������� �������-�������
	float mSmoothingFactor;

	float mLastFragmentSample = 0;

	//���� mSmoothingFactor != 0, �� �������������� �� ������ �������, � ������ ���� ���������� ����� � ����� �� ���� �������.
	FixedArray<float> mSampleFragmentData;

	//��������� �� ������ ������ ������� (������ ��� ��, ��� � mSampleFragment) ��� ���������� �� ���� ������� ������. ��� mSmoothingFactor == 0 ��������� � mSampleFragment
	unsigned mRightSampleFragmentStartIndex;

	//���� ������� �������� ������, ��� �� � �������� ����� ����� ������ �������� � ���� ������,
	//��� �� ��������� ��������� ����������� ���������������� ��������� ����������������� ���������
	INTRA_FORCEINLINE bool canDataMutate() const { return mSmoothingFactor != 0; }

	//���������� �����������, �� ������������� �������� ������ ����� ������� ��� ������� - ���������� ������
	INTRA_FORCEINLINE bool HasStereoMutatedData() const { return mSmoothingFactor != 0; }

	typedef void(*WaveForm)(const void* params, Span<float> dst, float freq, float volume, unsigned sampleRate);

	template<typename F> static void WaveFormWrapper(const void* params,
		Span<float> dst, float freq, float volume, unsigned sampleRate)
	{
		(*static_cast<const F*>(params))(dst, freq, volume, sampleRate);
	}


	void prepareInternalData(const void* params, WaveForm wave,
		float freq, float volume, unsigned sampleRate, bool goodPeriod, bool prepareToStereoDataMutation);

	//���� ������ �������, �������� � ���� �������� �� �������� � �������� ����� �������� � ������������ �����������,
	//����� ��������� ����������� ����������������� ���������, ������� ��� �������������� �� ��� ������.
	//����� ������������ ���� ����� ������ ������ ������ �������� ��������� �� ���������
	void preattenuateExponential(float expCoeff, unsigned sampleRate);

	INTRA_FORCEINLINE bool isExponentialPreattenuated() const { return !canDataMutate(); }

	WaveFormSampler(const void* params, WaveForm wave,
		float attenuationPerSample, float volume,
		float freq, unsigned sampleRate, float vibratoFrequency, float vibratoValue,
		float smoothingFactor, const Envelope& envelope);

	//TODO: ������
	INTRA_FORCEINLINE bool OwnDataArray() const noexcept { return true; }
	INTRA_FORCEINLINE bool IsAttenuatableDataArray() const noexcept { return mSmoothingFactor == 0; }

public:
	void MoveConstruct(void* dst) override { new(dst) WaveFormSampler(Cpp::Move(*this)); }
	bool OwnExponentialAttenuatedDataArray() const noexcept override { return mSmoothingFactor == 0; }

	template<typename F, typename = Meta::EnableIf<
		Meta::IsCallable<F, Span<float>, float, float, unsigned>::_
		>> INTRA_FORCEINLINE WaveFormSampler(const F& wave,
			float expCoeff, float volume, float freq, unsigned sampleRate,
			float vibratoFrequency, float vibratoValue, float smoothingFactor, const Envelope& envelope = Envelope::Constant()):
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
	EnvelopeFactory Envelope = EnvelopeFactory::Constant(1);

	void MoveConstruct(void* dst) override {new(dst) WaveInstrument(*this);}
	Sampler& CreateSampler(float freq, float volume, unsigned sampleRate,
		SamplerContainer& dst, uint16* oIndex = nullptr) const override;

	/// Временный call-интерфейс для вложенного использования в NoteSampler.
	WaveFormSampler operator()(float freq, float volume, unsigned sampleRate) const;
};
