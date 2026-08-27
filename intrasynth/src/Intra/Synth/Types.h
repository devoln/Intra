#pragma once

#include <Cpp/Warnings.h>
#include <Utils/Span.h>
#include <Funal/Delegate.h>

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

// Parameters shared by the source-level render path and the WASM ABI. The
// structure is copied into one MidiSynth instance; it is deliberately not a
// global so two sources can be rendered with different settings.
struct RenderParams
{
	float ReverbWet = 0.0f;  // 0..1, master effect amount; zero skips the effect
};
static_assert(sizeof(RenderParams) == sizeof(float), "RenderParams ABI must stay one float");

class IGenericSampler
{
public:
	virtual ~IGenericSampler() {}
	virtual size_t GenerateMono(Span<float> ioDst) = 0;
	virtual size_t GenerateStereo(Span<float> ioDstLeft, Span<float> ioDstRight) = 0;
	virtual void NoteRelease() {}
	virtual void MultiplyPitch(float freqMultiplier) {(void)freqMultiplier;}
	/// Pass source-level render parameters to samplers that have a note-level
	/// parameter (currently the measured piano stereo tilt). Master effects are
	/// handled by MidiSynth and are ignored by these samplers.
	virtual void SetRenderParams(const RenderParams& params) {(void)params;}
};

typedef Unique<IGenericSampler> GenericSamplerRef;

/// Адаптер: оборачивает функтор с сигнатурой Span<float>(Span<float>, bool)
/// в интерфейс IGenericSampler (используется для физических моделей ударных).
template<typename F> class FunctorGenericSampler: public IGenericSampler
{
	F mFunctor;
public:
	explicit FunctorGenericSampler(F f): mFunctor(Move(f)) {}

	size_t GenerateMono(Span<float> ioDst) override
	{
		Span<float> rest = mFunctor(ioDst, false);
		return ioDst.Length() - rest.Length();
	}

	size_t GenerateStereo(Span<float> ioDst, Span<float> ioDstRight) override
	{
		(void)ioDstRight;
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
