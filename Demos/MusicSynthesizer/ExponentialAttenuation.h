#pragma once

#include <Core/Warnings.h>
#include <Core/Features.h>
#include <Core/Span.h>
#include <Math/Math.h>

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

struct ExponentAttenuator
{
	float Factor, FactorStep;

	INTRA_FORCEINLINE ExponentAttenuator(decltype(null)=null): Factor(1), FactorStep(1) {}
	INTRA_FORCEINLINE ExponentAttenuator(float startVolume, float expCoeff, unsigned sampleRate):
		Factor(startVolume), FactorStep(Exp(-expCoeff/float(sampleRate))) {}

	static INTRA_FORCEINLINE ExponentAttenuator FromFactorAndStep(float factor, float factorStep)
	{
		ExponentAttenuator result;
		result.Factor = factor;
		result.FactorStep = factorStep;
		return result;
	}

	INTRA_FORCEINLINE void SkipSamples(size_t count)
	{Factor *= PowInt(FactorStep, int(count));}

	INTRA_FORCEINLINE ExponentAttenuator operator*=(const ExponentAttenuator& rhs)
	{
		Factor *= rhs.Factor;
		FactorStep *= rhs.FactorStep;
		return *this;
	}
};

struct ExponentAttenuatorFactory
{
	float StartVolume;
	float ExpCoeff;

	ExponentAttenuatorFactory(decltype(null)=null): StartVolume(1), ExpCoeff(0) {}
	ExponentAttenuatorFactory(float startVolume, float expCoeff):
		StartVolume(startVolume), ExpCoeff(expCoeff) {}

	ExponentAttenuator operator()(float freq, float volume, unsigned sampleRate) const
	{
		(void)freq; (void)volume;
		return ExponentAttenuator(StartVolume, ExpCoeff, sampleRate);
	}

	INTRA_FORCEINLINE bool operator==(decltype(null)) const noexcept {return StartVolume == 1 && ExpCoeff == 0;}
	INTRA_FORCEINLINE bool operator!=(decltype(null)) const noexcept {return !operator==(null);}
	INTRA_FORCEINLINE explicit operator bool() const noexcept {return operator!=(null);}
};

INTRA_WARNING_POP
