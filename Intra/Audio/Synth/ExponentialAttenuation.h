#pragma once

#include "Cpp/Warnings.h"
#include "Cpp/Features.h"

#include "Types.h"

#include "Utils/Span.h"

#include "Math/Math.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio { namespace Synth {

//Копирует src в dst с затуханием, пока не кончится либо dst, либо src.
void ExponentialAttenuate(Span<float>& dst, CSpan<float> src, float& exp, float ek);
void ExponentialAttenuateAdd(Span<float>& dst, CSpan<float> src, float& exp, float ek);

class ExponentAttenuator
{
	float mFactor, mFactorStep;
public:
	ExponentAttenuator(float startVolume, float expCoeff, uint sampleRate):
		mFactor(startVolume), mFactorStep(Math::Exp(-expCoeff/sampleRate)) {}

	void operator()(Span<float> inOutSamples);
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

}}}

INTRA_WARNING_POP
