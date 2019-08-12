#pragma once

#include "WaveTableSampler.h"

//! �������, ������� ��������� ����������� �������� ������,
//! �������� ������� �������� ������� � ����, ����� �������, ��� ����� ���������� � ����������������.
//! ���������� - ��������� ������, ����������� � ��������, ��� ������ ������� �������������.
//! TODO: ��������� �������� ������ �� ���� ������ �������� ���, �� ����� �� ����������� �� ����� ����� ������������ ������ �������� �������.
class WaveFormSampler: public WaveTableSampler
{
	//���������, ������������ ��� ���������� ������� �� ������ ������� �� ������� � ��������� �������-�������
	float mSmoothingFactor;

	float mLastFragmentSample = 0;

	//���� mSmoothingFactor != 0, �� �������������� �� ������ �������, � ������ ���� ���������� ����� � ����� �� ���� �������.
	FixedArray<float> mSampleFragmentData;

	//��������� �� ������ ������ ������� (������ ��� ��, ��� � mSampleFragment) ��� ���������� �� ���� ������� ������. ��� mSmoothingFactor == 0 ��������� � mSampleFragment
	uint mRightSampleFragmentStartIndex;

	//���� ������� �������� ������, ��� �� � �������� ����� ����� ������ �������� � ���� ������,
	//��� �� ��������� ��������� ����������� ���������������� ��������� ����������������� ���������
	forceinline bool canDataMutate() const { return mSmoothingFactor != 0; }

	//���������� �����������, �� ������������� �������� ������ ����� ������� ��� ������� - ���������� ������
	forceinline bool HasStereoMutatedData() const { return mSmoothingFactor != 0; }

	typedef void(*WaveForm)(const void* params, Span<float> dst, float freq, float volume, uint sampleRate);

	template<typename F> static void WaveFormWrapper(const void* params,
		Span<float> dst, float freq, float volume, uint sampleRate)
	{
		(*static_cast<const F*>(params))(dst, freq, volume, sampleRate);
	}


	CSpan<float> prepareInternalData(const void* params, WaveForm wave,
		float freq, float volume, uint sampleRate, bool goodPeriod, bool prepareToStereoDataMutation);

	//���� ������ �������, �������� � ���� �������� �� �������� � �������� ����� �������� � ������������ �����������,
	//����� ��������� ����������� ����������������� ���������, ������� ��� �������������� �� ��� ������.
	//����� ������������ ���� ����� ������ ������ ������ �������� ��������� �� ���������
	void preattenuateExponential(float expCoeff, uint sampleRate);

	forceinline bool isExponentialPreattenuated() const { return !canDataMutate(); }

	WaveFormSampler(const void* params, WaveForm wave,
		float attenuationPerSample, float volume,
		float freq, uint sampleRate, float vibratoFrequency, float vibratoValue,
		float smoothingFactor, const Envelope& envelope);

	//TODO: ������
	forceinline bool OwnDataArray() const noexcept { return true; }
	forceinline bool IsAttenuatableDataArray() const noexcept { return mSmoothingFactor == 0; }

public:
	void MoveConstruct(void* dst) override { new(dst) WaveFormSampler(Cpp::Move(*this)); }

	template<typename F, typename = Meta::EnableIf<
		Meta::IsCallable<F, Span<float>, float, float, uint>::_
		>> forceinline WaveFormSampler(const F& wave,
			float expCoeff, float volume, float freq, uint sampleRate,
			float vibratoFrequency, float vibratoValue, float smoothingFactor, const Envelope& envelope = null):
		WaveFormSampler(&wave, WaveFormWrapper<F>, expCoeff, volume,
			freq, sampleRate, vibratoFrequency, vibratoValue, smoothingFactor, envelope)
	{}


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

class WaveInstrument: public Instrument
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

	Sampler& CreateSampler(float freq, float volume, uint sampleRate,
		SamplerContainer& dst, ushort* oIndex = null) const override;
};
