#pragma once

#include <Core/Warnings.h>

#include <Core/Span.h>
#include <Core/Debug.h>

#include <Math/Math.h>

#include "Types.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

enum class FilterType: byte {LowPass, HighPass, End};

struct FilterCoeffs
{
	//! Коэффициенты двух предыдущих входных значений.
	float A1, A2;

	//! Коэффициенты двух предыдущих выходов фильтра.
	float B1, B2;

	//! Коэффициент перед текущим обрабатываемом семплом.
	float C;

	static FilterCoeffs Calculate(float rezAmount, float cutoffRatio, FilterType type);

	bool operator==(decltype(null)) const {return A1 == 0 && A2 == 0 && B1 == 0 && B2 == 0 && C == 1;}
	INTRA_FORCEINLINE bool operator!=(decltype(null)) const {return !operator==(null);}
};

struct Filter: FilterCoeffs
{
	//! Предыдущие два входных значения.
	float PrevSrc = 0, PrevSrc2 = 0;

	//! Предыдущие два выхода фильтра.
	float PrevSample = 0, PrevSample2 = 0;

	Filter(decltype(null)=null): FilterCoeffs{0, 0, 0, 0, 1} {}

	Filter(const FilterCoeffs& coeffs): FilterCoeffs(coeffs) {}

	Filter(float rezAmount, float cutoffFreq, unsigned sampleRate, FilterType type):
		Filter(rezAmount, cutoffFreq/float(sampleRate), type) {}

	Filter(float rezAmount, float cutoffRatio, FilterType type):
		FilterCoeffs(FilterCoeffs::Calculate(rezAmount, cutoffRatio, type)) {}

	Filter(float a1, float a2, float b1, float b2, float c): FilterCoeffs{a1, a2, b1, b2, c} {}

	void operator()(Span<float> inOutSamples);
};

struct FilterFactory
{
	FilterCoeffs Coeffs;

	//! В интервале [Sqrt(2); ~0.1)
	float RezAmount;

	//! Порог частоты в интервале (~0; sampleRate/2).
	//! или отношение порога частоты к частоте дискретизации со знаком '-' в интервале (-1/2; 0).
	//! В первом случае поле Copy игнорируется.
	//! Во втором случае предполагается, что Copy содержит готовый фильтр.
	float CutoffFrequency;

	FilterType Type;

	FilterFactory(decltype(null)=null): Coeffs({0,0,0,0,1}),
		RezAmount(0), CutoffFrequency(0), Type(FilterType::End) {}

	FilterFactory(const FilterCoeffs& coeffs): Coeffs(coeffs),
		RezAmount(0), CutoffFrequency(0), Type(FilterType::End) {}

	static FilterFactory FromCutoffRatio(float rezAmount, float cutoffRatio, FilterType type)
	{
		INTRA_DEBUG_ASSERT(cutoffRatio > 0 && cutoffRatio < 0.5f);
		FilterFactory result;
		result.Coeffs = Filter(rezAmount, cutoffRatio, type);
		result.RezAmount = rezAmount;
		result.CutoffFrequency = -cutoffRatio;
		result.Type = type;
		return result;
	}

	FilterFactory(float rezAmount, float cutoffFrequency, FilterType type):
		RezAmount(rezAmount), CutoffFrequency(cutoffFrequency), Type(type) {}

	INTRA_FORCEINLINE bool operator==(decltype(null)) const {return Coeffs == null && Type == FilterType::End;}
	INTRA_FORCEINLINE bool operator!=(decltype(null)) const {return !operator==(null);}

	Filter operator()(float freq, float volume, unsigned sampleRate) const
	{
		(void)freq; (void)volume;
		if(CutoffFrequency <= 0) return Filter(Coeffs);
		return Filter(RezAmount, CutoffFrequency, sampleRate, Type);
	}
};


struct DriveEffect
{
	float K;
	DriveEffect(float k=0): K(k) {}

	void operator()(Span<float> inOutSamples);
	INTRA_FORCEINLINE DriveEffect operator()(float freq, float volume, unsigned sampleRate) const
	{
		(void)freq; (void)volume; (void)sampleRate;
		return *this;
	}
};

struct ResonanceFilter
{
	float DeltaPhase, QFactor;
	float PrevSample, S;

	ResonanceFilter(decltype(null)=null): DeltaPhase(0), QFactor(0), PrevSample(0), S(0) {}

	ResonanceFilter(float dphi, float qfactor):
		DeltaPhase(dphi), QFactor(qfactor), PrevSample(0), S(0) {}

	INTRA_FORCEINLINE bool operator==(decltype(null)) const noexcept {return DeltaPhase == 0;}
	INTRA_FORCEINLINE bool operator!=(decltype(null)) const noexcept {return !operator==(null);}
	INTRA_FORCEINLINE explicit operator bool() const noexcept {return !operator==(null);}

	INTRA_FORCEINLINE float operator()(float sample)
	{
		sample += S*DeltaPhase + PrevSample;
		PrevSample = sample;
		S -= sample*DeltaPhase;
		S *= QFactor;
		return sample;
	}

	void operator()(Span<float> inOutSamples);
};

struct ResonanceFilterFactory
{
	float Frequency;
	float QFactor;

	INTRA_FORCEINLINE ResonanceFilterFactory(decltype(null) = null): Frequency(0), QFactor(0) {}
	INTRA_FORCEINLINE ResonanceFilterFactory(float frequency, float qfactor):
		Frequency(frequency), QFactor(qfactor) {}

	INTRA_FORCEINLINE bool operator==(decltype(null)) const noexcept {return Frequency == 0;}
	INTRA_FORCEINLINE bool operator!=(decltype(null)) const noexcept {return !operator==(null);}
	INTRA_FORCEINLINE explicit operator bool() const noexcept {return !operator==(null);}

	ResonanceFilter operator()(float freq, float volume, unsigned sampleRate) const
	{
		(void)volume;
		return {(Frequency<0? -freq*Frequency: Frequency)*2*float(PI)/float(sampleRate), QFactor};
	}
};

struct DynamicResonanceFilter
{
	float DeltaPhase;
	float InvQFactor;
	float InvQFactorStep;
	float PrevSample, S;

	DynamicResonanceFilter(decltype(null)=null): DeltaPhase(0), InvQFactor(2), InvQFactorStep(0), PrevSample(0), S(0) {}

	DynamicResonanceFilter(float dphi, float invQFactor, float invQFactorStep):
		DeltaPhase(dphi), InvQFactor(invQFactor), PrevSample(0), S(0), InvQFactorStep(invQFactorStep) {}

	INTRA_FORCEINLINE bool operator==(decltype(null)) const noexcept {return DeltaPhase == 0;}
	INTRA_FORCEINLINE bool operator!=(decltype(null)) const noexcept {return !operator==(null);}
	INTRA_FORCEINLINE explicit operator bool() const noexcept {return !operator==(null);}

	float operator()(float sample)
	{
		sample += S*DeltaPhase + PrevSample;
		PrevSample = sample;
		S -= sample*DeltaPhase;
		S *= 1 - 1/InvQFactor;
		InvQFactor += InvQFactorStep;
		return sample;
	}

	void operator()(Span<float> inOutSamples);
};

struct DynamicResonanceFilterFactory
{
	float Frequency;
	float InvQFactor;
	float InvQFactorSpeed;

	INTRA_FORCEINLINE DynamicResonanceFilterFactory(decltype(null) = null): Frequency(0), InvQFactor(0), InvQFactorSpeed(0) {}
	INTRA_FORCEINLINE DynamicResonanceFilterFactory(float frequency, float invQFactor, float invQFactorSpeed):
		Frequency(frequency), InvQFactor(invQFactor), InvQFactorSpeed(invQFactorSpeed) {}

	INTRA_FORCEINLINE bool operator==(decltype(null)) const noexcept {return Frequency == 0;}
	INTRA_FORCEINLINE bool operator!=(decltype(null)) const noexcept {return !operator==(null);}
	INTRA_FORCEINLINE explicit operator bool() const noexcept {return !operator==(null);}

	DynamicResonanceFilter operator()(float freq, float volume, unsigned sampleRate) const
	{
		(void)volume;
		return {(Frequency < 0? -freq*Frequency: Frequency)*2*float(PI)/float(sampleRate), InvQFactor, InvQFactorSpeed/float(sampleRate)};
	}
};

struct SoftHighPassFilter
{
	float K;
	float S;
	SoftHighPassFilter(float k = 0): K(k), S(0) {}

	void operator()(Span<float> inOutSamples);
};

struct SoftHighPassFilterFactory
{
	float CutoffFrequency;

	INTRA_FORCEINLINE SoftHighPassFilterFactory(decltype(null)=null): CutoffFrequency(0) {}
	INTRA_FORCEINLINE SoftHighPassFilterFactory(float frequency):
		CutoffFrequency(frequency) {}

	INTRA_FORCEINLINE bool operator==(decltype(null)) const noexcept {return CutoffFrequency == 0;}
	INTRA_FORCEINLINE bool operator!=(decltype(null)) const noexcept {return !operator==(null);}
	INTRA_FORCEINLINE explicit operator bool() const noexcept {return !operator==(null);}

	SoftHighPassFilter operator()(float freq, float volume, unsigned sampleRate) const
	{
		(void)volume;
		const float f = (CutoffFrequency < 0? -freq: 1)*CutoffFrequency;
		const float w =  2*float(PI)*f;
		return {1 / (1 + w/float(sampleRate))};
	}
};

struct NormalizeEffect
{
	float Volume;
	float AbsMax;

	NormalizeEffect(float volume = 1): Volume(volume), AbsMax(0) {}

	INTRA_FORCEINLINE NormalizeEffect operator()(float freq, float volume, unsigned sampleRate) const
	{
		(void)freq; (void)volume; (void)sampleRate;
		return *this;
	}

	void operator()(Span<float> inOutSamples);
};

INTRA_WARNING_POP
