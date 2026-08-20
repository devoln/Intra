#pragma once

#include <Cpp/Warnings.h>
#include <Utils/Span.h>
#include <Funal/Delegate.h>

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

class IGenericSampler
{
public:
	virtual ~IGenericSampler() {}
	virtual size_t GenerateMono(Span<float> ioDst, Span<float> ioDstReverb) = 0;
	size_t GenerateMono(Span<float> ioDst) {return GenerateMono(ioDst, nullptr);}
	virtual size_t GenerateStereo(Span<float> ioDst, Span<float> ioDstRight, Span<float> ioDstReverb) = 0;
	virtual size_t GenerateStereo(Span<float> ioDstLeft, Span<float> ioDstRight) {return GenerateStereo(ioDstLeft, ioDstRight, nullptr);}
	virtual void NoteRelease() {}
	virtual void MultiplyPitch(float freqMultiplier) {(void)freqMultiplier;}
};

typedef Unique<IGenericSampler> GenericSamplerRef;

/// Адаптер: оборачивает функтор с сигнатурой Span<float>(Span<float>, bool)
/// в интерфейс IGenericSampler (используется для физических моделей ударных).
template<typename F> class FunctorGenericSampler: public IGenericSampler
{
	F mFunctor;
public:
	explicit FunctorGenericSampler(F f): mFunctor(Move(f)) {}

	size_t GenerateMono(Span<float> ioDst, Span<float> ioDstReverb) override
	{
		(void)ioDstReverb;
		Span<float> rest = mFunctor(ioDst, false);
		return ioDst.Length() - rest.Length();
	}

	size_t GenerateStereo(Span<float> ioDst, Span<float> ioDstRight, Span<float> ioDstReverb) override
	{
		(void)ioDstRight; (void)ioDstReverb;
		Span<float> rest = mFunctor(ioDst, false);
		return ioDst.Length() - rest.Length();
	}
};

/// Генератор семплов.
/// @param[in,out] inOutSamples Массив, содержащий обрабатываемые семплы.
typedef Funal::CopyableMutableDelegate<void(
	Span<float> inOutSamples
)> GenericGenerator;

/// Модификатор семплов.
/// @param[in,out] inOutSamples Массив, содержащий обрабатываемые семплы.
typedef Funal::CopyableMutableDelegate<void(
	Span<float> inOutSamples
)> GenericModifier;

/// Инструмент - источник семплеров нот.
typedef Funal::CopyableDelegate<GenericSamplerRef(
	float freq, float volume, unsigned sampleRate
)> GenericInstrument;

/// Ударный инструмент - источник семплеров нот.
typedef Funal::Delegate<GenericSamplerRef(
	float volume, unsigned sampleRate
)> GenericDrumInstrument;

/// Фабрика модификаторов - источник модификаторов семплов.
typedef Funal::CopyableDelegate<GenericModifier(
	float freq, float volume, unsigned sampleRate
)> GenericModifierFactory;

INTRA_WARNING_POP
