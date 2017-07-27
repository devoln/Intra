#pragma once

#include "Cpp/Warnings.h"
#include "Cpp/InfNan.h"

#include "Utils/Span.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio { namespace Synth {

struct AdsrAttenuator
{
	size_t AttackSamples;
	size_t DecaySamples;
	float SustainVolume;
	size_t ReleaseSamples;
	float U, DU;

	AdsrAttenuator(null_t=null);
	AdsrAttenuator(float attackTime, float decayTime,
		float sustainVolume, float releaseTime, uint sampleRate);

	//! ��������� ��������� � ��������� �������
	void operator()(Span<float> inOutSamples);

	//! ���������� ���� �������� ����������� ������� � ������ release,
	//! ��� �������� � ������ ����� � �����, ���� ��� ��� ������������.
	void NoteRelease();

	//! ������� ������� �������� ���������� �� ������� ��������� ���� �� ����.
	size_t SamplesLeft() const noexcept {return SustainVolume == -1? (U == 0? 0: ReleaseSamples): ~size_t();}

	//! ������� ������� �������� ���������� �� �������� � ���������� ��������� ���������.
	size_t CurrentStateSamplesLeft() const;

	//! ������� �������, ��� ������� ��� �������� ��������� � numSamples ������� � ��������������� ������� ������� U.
	//! ���� ����� ���������, ����� ������������ ��������� ������� ����� �������� ��������� ���������� �����.
	void SamplesProcessedExternally(size_t numSamples);

	forceinline explicit operator bool() const noexcept {return U > -1;}
	forceinline bool operator==(null_t) const noexcept {return !bool(*this);}
	forceinline bool operator!=(null_t) const noexcept {return bool(*this);}

private:
	void beginAttack();
	void attack(Span<float>& inOutSamples);
	void beginDecay();
	void decay(Span<float>& inOutSamples);
	void beginSustain();
	void beginRelease();
	void release(Span<float>& inOutSamples);
};

struct AdsrAttenuatorFactory
{
	float AttackTime;
	float DecayTime;
	float SustainVolume;
	float ReleaseTime;

	AdsrAttenuatorFactory(null_t=null):
		AttackTime(0), DecayTime(0), SustainVolume(1), ReleaseTime(Cpp::Infinity) {}

	AdsrAttenuatorFactory(float attackTime, float decayTime, float sustainVolume, float releaseTime):
		AttackTime(attackTime), DecayTime(decayTime), SustainVolume(sustainVolume), ReleaseTime(releaseTime) {}

	AdsrAttenuator operator()(float freq, float volume, uint sampleRate) const
	{
		(void)freq; (void)volume;
		return AdsrAttenuator(AttackTime, DecayTime, SustainVolume, ReleaseTime, sampleRate);
	}

	bool operator==(null_t) const noexcept {return AttackTime == 0 && DecayTime == 0 && SustainVolume == 1 && ReleaseTime == Cpp::Infinity;}
	forceinline bool operator!=(null_t) const noexcept {return !operator==(null);}
	forceinline explicit operator bool() const noexcept {return operator!=(null);}
};

}}}

INTRA_WARNING_POP
