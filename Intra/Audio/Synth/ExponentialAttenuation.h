#pragma once

#include "Cpp/Warnings.h"
#include "Cpp/Features.h"

#include "Types.h"

#include "Utils/Span.h"

#include "Math/Math.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio { namespace Synth {

//Копирует src в dst с затуханием, пока не кончится либо dst, либо src.
void ExponentialAttenuate(Span<float>& dst, CSpan<float>& src, float& exp, float ek);
void ExponentialAttenuateAdd(Span<float>& dst, CSpan<float>& src, float& exp, float ek);
void ExponentialLinearAttenuate(Span<float>& dst, CSpan<float>& src, float& exp, float ek, float& u, float du);
void ExponentialLinearAttenuateAdd(Span<float>& dst, CSpan<float>& src, float& exp, float ek, float& u, float du);

inline void ExponentialAttenuate(Span<float>& inOutSamples, float& exp, float ek)
{
	CSpan<float> src = inOutSamples;
	ExponentialAttenuate(inOutSamples, src, exp, ek);
}

struct ExponentAttenuator
{
	float Factor, FactorStep;

	ExponentAttenuator(null_t=null): Factor(1), FactorStep(1) {}
	ExponentAttenuator(float startVolume, float expCoeff, uint sampleRate):
		Factor(startVolume), FactorStep(Math::Exp(-expCoeff/float(sampleRate))) {}

	void operator()(Span<float> inOutSamples);
	void operator()(Span<float> dstSamples, CSpan<float> srcSamples, bool add);

	void SkipSamples(size_t count);
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
