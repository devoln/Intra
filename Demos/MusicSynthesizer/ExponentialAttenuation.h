#pragma once

#include <Core/Warnings.h>
#include <Core/Features.h>
#include <Core/Span.h>
#include <Math/Math.h>

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

struct ExponentAttenuator
{
	float Factor, FactorStep;

	forceinline ExponentAttenuator(null_t=null): Factor(1), FactorStep(1) {}
	forceinline ExponentAttenuator(float startVolume, float expCoeff, uint sampleRate):
		Factor(startVolume), FactorStep(Math::Exp(-expCoeff/float(sampleRate))) {}

	static forceinline ExponentAttenuator FromFactorAndStep(float factor, float factorStep)
	{
		ExponentAttenuator result;
		result.Factor = factor;
		result.FactorStep = factorStep;
		return result;
	}

	forceinline void SkipSamples(size_t count)
	{Factor *= Math::PowInt(FactorStep, int(count));}

	forceinline ExponentAttenuator operator*=(const ExponentAttenuator& rhs)
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

	ExponentAttenuatorFactory(null_t=null): StartVolume(1), ExpCoeff(0) {}
	ExponentAttenuatorFactory(float startVolume, float expCoeff):
		StartVolume(startVolume), ExpCoeff(expCoeff) {}

	ExponentAttenuator operator()(float freq, float volume, uint sampleRate) const
	{
		(void)freq; (void)volume;
		return ExponentAttenuator(StartVolume, ExpCoeff, sampleRate);
	}

	forceinline bool operator==(null_t) const noexcept {return StartVolume == 1 && ExpCoeff == 0;}
	forceinline bool operator!=(null_t) const noexcept {return !operator==(null);}
	forceinline explicit operator bool() const noexcept {return operator!=(null);}
};

INTRA_WARNING_POP
