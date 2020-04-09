#pragma once

#include <Core/Warnings.h>
#include <Core/Span.h>
#include <Funal/Delegate.h>

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

class IGenericSampler
{
public:
	virtual ~IGenericSampler() {}
	virtual size_t GenerateMono(Span<float> ioDst, Span<float> ioDstReverb) = 0;
	size_t GenerateMono(Span<float> ioDst) {return GenerateMono(ioDst, null);}
	virtual size_t GenerateStereo(Span<float> ioDst, Span<float> ioDstRight, Span<float> ioDstReverb) = 0;
	virtual size_t GenerateStereo(Span<float> ioDstLeft, Span<float> ioDstRight) {GenerateStereo(ioDstLeft, ioDstRight, null);}
	virtual void NoteRelease() {}
	virtual void MultiplyPitch(float freqMultiplier) {(void)freqMultiplier;}
};

typedef Unique<IGenericSampler> GenericSamplerRef;

//! Генератор семплов.
//! @param[in,out] inOutSamples Массив, содержащий обрабатываемые семплы.
typedef Funal::CopyableMutableDelegate<void(
	Span<float> inOutSamples
)> GenericGenerator;

//! Модификатор семплов.
//! @param[in,out] inOutSamples Массив, содержащий обрабатываемые семплы.
typedef Funal::CopyableMutableDelegate<void(
	Span<float> inOutSamples
)> GenericModifier;

//! Инструмент - источник семплеров нот.
typedef Funal::CopyableDelegate<GenericSamplerRef(
	float freq, float volume, unsigned sampleRate
)> GenericInstrument;

//! Ударный инструмент - источник семплеров нот.
typedef Funal::CopyableDelegate<GenericSamplerRef(
	float volume, unsigned sampleRate
)> GenericDrumInstrument;

//! Фабрика модификаторов - источник модификаторов семплов.
typedef Funal::CopyableDelegate<GenericModifier(
	float freq, float volume, unsigned sampleRate
)> GenericModifierFactory;

INTRA_WARNING_POP
