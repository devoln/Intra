#pragma once

#include <Cpp/Warnings.h>
#include <Utils/Span.h>
#include <Funal/Delegate.h>
#include "Envelope.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

// Parameters shared by the source-level render path and the WASM ABI. The
// structure is copied into one MidiSynth instance; it is deliberately not a
// global so two sources can be rendered with different settings. The ABI is a
// fixed array of floats written from JS (see SourceSetParams in
// EmscriptenInterface.cpp): keep this struct flat and float-only so sizeof
// matches the wire layout.
struct RenderParams
{
	float ReverbWet = 0.0f;   // 0..1, master effect amount; zero skips the effect
};
static_assert(sizeof(RenderParams) == 1*sizeof(float), "RenderParams ABI must stay a single float");

struct RenderEnvelope
{
	float Exp;
	float ExpStep;
	float Linear;
	float LinearStep;

	INTRA_FORCEINLINE explicit RenderEnvelope(const EnvelopeSegment& segment):
		Exp(segment.Exp.Factor), ExpStep(segment.Exp.FactorStep),
		Linear(segment.Linear.Factor), LinearStep(segment.Linear.FactorStep) {}

	INTRA_FORCEINLINE float NextGain()
	{
		const float result = Exp*Linear;
		// ExponentialLinearAttenuate's SIMD implementation evaluates eight
		// consecutive gains in parallel; it does not hold one gain for the
		// whole vector. Keep the source-level sink semantically identical.
		Exp *= ExpStep;
		Linear += LinearStep;
		return result;
	}
};

class IGenericSampler
{
public:
	virtual ~IGenericSampler() {}
	virtual size_t GenerateMono(Span<float> ioDst) = 0;
	virtual size_t GenerateStereo(Span<float> ioDstLeft, Span<float> ioDstRight) = 0;

	/// Opt-in path for applying the note-level ADSR before adding to the shared
	/// mix. Sources that do not implement it must stay on NoteSampler's isolated
	/// scratch-buffer path.
	virtual bool SupportsEnvelopeRender() const {return false;}
	virtual size_t GenerateStereoWithEnvelope(Span<float> ioDstLeft,
		Span<float> ioDstRight, const EnvelopeSegment& envelope)
	{
		(void)ioDstLeft;
		(void)ioDstRight;
		(void)envelope;
		return 0;
	}

	virtual void NoteRelease() {}
	virtual void MultiplyPitch(float freqMultiplier) {(void)freqMultiplier;}
	/// MIDI channel pan. Generic samplers that own a stereo image can apply it
	/// as an outer balance layer; mono/irrelevant samplers keep the no-op.
	virtual void SetPan(float newPan) {(void)newPan;}
	/// Raw MIDI key velocity, normalized to [0; 1]. Sources that model
	/// velocity-dependent timbre can keep it separate from channel volume.
	virtual void SetVelocity(float velocity01) {(void)velocity01;}
	/// Pass source-level render parameters to samplers that have a note-level
	/// parameter (currently the measured piano stereo tilt). Master effects are
	/// handled by MidiSynth and are ignored by these samplers.
	virtual void SetRenderParams(const RenderParams& params) {(void)params;}

#ifdef INTRA_UI_METERS
	/// Уровень огибающей ноты (0..1) для индикатора громкости в веб-UI, либо -1,
	/// если семплер уровня не измеряет (ударные, шум, физические модели) — тогда
	/// индикатор ведёт себя как раньше (ровный уровень). Спрашивается редко
	/// (SourceGetNoteLevels, ~100 мс), на семпл расходов нет. Собирается только
	/// с -DINTRA_UI_METERS. Замер стоимости замены на поле — см. Sampler::GetLevel.
	virtual float GetLevel() const {return -1.0f;}
#endif
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
