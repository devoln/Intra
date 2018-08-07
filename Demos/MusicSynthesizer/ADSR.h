#pragma once

#include <Cpp/Warnings.h>
#include <Cpp/InfNan.h>
#include <Utils/Span.h>

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

struct ExponentAttenuator;

struct AdsrAttenuator
{
	size_t AttackSamples;
	size_t DecaySamples;
	float SustainVolume;
	size_t ReleaseSamples;
	bool Linear;
	
	//! Текущее значение амплитуды
	float U;

	//! Если Linear, то это значение, которое вычитается из U на каждом шаге.
	//! Иначе - множитель, на который умножается U на каждом шаге.
	float DU;

	AdsrAttenuator(null_t=null);
	AdsrAttenuator(float attackTime, float decayTime,
		float sustainVolume, float releaseTime, uint sampleRate, bool linear=false);

	//! Применить огибающую к следующим семплам
	void operator()(Span<float> inOutSamples);

	//! Применить огибающую к srcSamples и записать\прибавить результат к dstSamples
	void operator()(Span<float> dstSamples, CSpan<float> srcSamples);

	//! Применить огибающую к srcSamples и записать\прибавить результат к dstSamples.
	//! Дополнительно умножает на коэффициент за один проход.
	void operator()(Span<float> dstSamples, CSpan<float> srcSamples, float coeff);

	//! Применить огибающую к srcSamples и записать\прибавить результат к dstSamples.
	//! Дополнительно умножает на коэффициент и накладывает экспоненциальное затухание за один проход.
	void operator()(Span<float> dstSamples, CSpan<float> srcSamples, float coeff, ExponentAttenuator& exp);

	//! Отпускание ноты означает немедленный переход к стадии release,
	//! что приводит к отмене атаки и спада, если они ещё продолжаются.
	void NoteRelease();

	//! Сколько семплов осталось обработать до полного затухания ноты до нуля.
	size_t SamplesLeft() const noexcept {return SustainVolume == -1? (U == 0? 0: ReleaseSamples): ~size_t();}

	//! Сколько семплов осталось обработать до перехода к следующему состоянию огибающей.
	size_t CurrentStateSamplesLeft() const;

	//! Указать объекту, что внешний код применил затухание к numSamples семплам и соответствующим образом обновил U.
	//! Этот вызов необходим, чтобы восстановить инвариант объекта после внешнего изменения внутренних полей.
	void SamplesProcessedExternally(size_t numSamples);

	forceinline explicit operator bool() const noexcept {return U > -1;}
	forceinline bool operator==(null_t) const noexcept {return !bool(*this);}
	forceinline bool operator!=(null_t) const noexcept {return bool(*this);}

	forceinline bool IsNoteReleased() const noexcept {return SustainVolume == -1;}
	forceinline bool IsInfinite() const noexcept {return ReleaseSamples == ~size_t();}

private:
	void beginAttack();
	void attack(Span<float>& dstSamples, CSpan<float>& srcSamples, ExponentAttenuator& exp);
	void beginDecay();
	void decay(Span<float>& dstSamples, CSpan<float>& srcSamples, ExponentAttenuator& exp);
	void beginSustain();
	void beginRelease();
	void release(Span<float>& dstSamples, CSpan<float>& srcSamples, ExponentAttenuator& exp);
	void sustain(Span<float>& dstSamples, CSpan<float>& srcSamples, ExponentAttenuator& exp);

	void work(Span<float> dst, CSpan<float>& srcSamples, ExponentAttenuator& exp);
};

struct AdsrAttenuatorFactory
{
	float AttackTime;
	float DecayTime;
	float SustainVolume;
	float ReleaseTime;
	bool Linear;

	AdsrAttenuatorFactory(null_t=null):
		AttackTime(0), DecayTime(0), SustainVolume(1), ReleaseTime(Cpp::Infinity), Linear(false) {}

	AdsrAttenuatorFactory(float attackTime, float decayTime, float sustainVolume, float releaseTime, bool linear=false):
		AttackTime(attackTime), DecayTime(decayTime), SustainVolume(sustainVolume), ReleaseTime(releaseTime), Linear(linear) {}

	AdsrAttenuator operator()(float freq, float volume, uint sampleRate) const
	{
		(void)freq; (void)volume;
		return AdsrAttenuator(AttackTime, DecayTime, SustainVolume, ReleaseTime, sampleRate, Linear);
	}

	bool operator==(null_t) const noexcept {return AttackTime == 0 && DecayTime == 0 && SustainVolume == 1 && ReleaseTime == Cpp::Infinity;}
	forceinline bool operator!=(null_t) const noexcept {return !operator==(null);}
	forceinline explicit operator bool() const noexcept {return operator!=(null);}
};

INTRA_WARNING_POP
