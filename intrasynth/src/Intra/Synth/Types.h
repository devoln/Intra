#pragma once

#include <Cpp/Warnings.h>
#include <Utils/Span.h>
#include <Funal/Delegate.h>
#include <Math/Math.h>
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
	float ReverbWet = 0.0f;   // 0..2, direct UI/C++ amount; 1.0 = normal 100%, zero skips the effect
};
static_assert(sizeof(RenderParams) == 1*sizeof(float), "RenderParams ABI must stay a single float");

// Note-on noteParams are resolved above the sampler layer. Raw MIDI velocity
// stops here; samplers receive only a dimensionless strike coordinate and a
// ready linear note gain.
struct NoteOnParams
{
	float Strike = 1.0f;
	float Gain = 1.0f;
};

// Current Titanic build: force the accepted v^2 law at compile time. This is
// intentionally a separate fast path so the current WASM pays no descriptor,
// switch or unused alternate-curve code. The optional runtime mode below keeps
// a compact bank-style velocity modulator for future reference banks.
INTRA_FORCEINLINE NoteOnParams ResolveForcedNoteOnParams(byte velocity)
{
	const float strike = float(velocity)*(1.0f/127.0f);
	const float x = float(velocity)*0.01f;
	return NoteOnParams{strike, (0.80848074f*x)*x};
}

#ifdef INTRA_RUNTIME_VELOCITY_MODULATORS
// Compact velocity-only bank modulator. The source is implicitly note-on
// velocity, so the redundant source index is omitted. Amount is signed 16-bit;
// source flags pack curve/direction/polarity. Destination is kept separate so
// a future bank importer can route the
// same source law without changing the note/sampler API.
enum class VelocityModCurve: byte {Linear, Concave, Convex, Switch, LegacySquare};
enum class VelocityModDestination: byte {AttenuationCentibels, LinearGain};
struct VelocityModulator
{
	int16 Amount = 960;
	byte SourceFlags = byte(VelocityModCurve::LegacySquare);
	VelocityModDestination Destination = VelocityModDestination::LinearGain;
};
static_assert(sizeof(VelocityModulator) == 4, "VelocityModulator must stay compact");

INTRA_FORCEINLINE float VelocityCurveConcave(float x)
{
	if(x <= 0.0f) return 0.0f;
	if(x >= 1.0f) return 1.0f;
	return Math::Clamp(-0.1809560341f*Math::Log(1.0f - x), 0.0f, 1.0f);
}
INTRA_FORCEINLINE float VelocityCurveConvex(float x)
{
	if(x <= 0.0f) return 0.0f;
	if(x >= 1.0f) return 1.0f;
	return Math::Clamp(1.0f + 0.1809560341f*Math::Log(x), 0.0f, 1.0f);
}

INTRA_FORCEINLINE NoteOnParams ResolveVelocityNoteOnParams(const VelocityModulator& mod, byte velocity)
{
	const float strike = float(velocity)*(1.0f/127.0f);
	const VelocityModCurve curve = VelocityModCurve(mod.SourceFlags & 7u);
	if(curve == VelocityModCurve::LegacySquare) return ResolveForcedNoteOnParams(velocity);
	const bool negative = (mod.SourceFlags & 8u) != 0;
	const bool bipolar = (mod.SourceFlags & 16u) != 0;
	const float pos = float(velocity)*(1.0f/128.0f);
	float x = negative ? (127.0f/128.0f - pos) : pos;
	if(bipolar) x = -1.0f + 2.0f*x;
	if(curve == VelocityModCurve::Switch)
		x = bipolar ? (x >= 0.0f ? 1.0f : -1.0f) : (x >= 0.5f ? 1.0f : 0.0f);
	else if(curve != VelocityModCurve::Linear)
	{
		const float sign = x < 0.0f ? -1.0f : 1.0f;
		const float a = Math::Clamp((x < 0.0f ? -x : x)*(128.0f/127.0f), 0.0f, 1.0f);
		x = sign*(curve == VelocityModCurve::Concave ? VelocityCurveConcave(a) : VelocityCurveConvex(a));
	}
	const float value = float(mod.Amount)*x;
	if(mod.Destination == VelocityModDestination::AttenuationCentibels)
		return NoteOnParams{strike, Math::Exp(-0.01151292546f*value)};
	return NoteOnParams{strike, value};
}
#endif

INTRA_FORCEINLINE NoteOnParams ResolveDefaultNoteOnParams(byte velocity)
{
#ifdef INTRA_RUNTIME_VELOCITY_MODULATORS
	return ResolveVelocityNoteOnParams(VelocityModulator{}, velocity);
#else
	return ResolveForcedNoteOnParams(velocity);
#endif
}

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
	/// One-time/static amplitude scaling (instrument calibration) or live gain.
	/// Implementations must not rebuild timbral/seed state.
	virtual void MultiplyVolume(float volumeMultiplier) {(void)volumeMultiplier;}
	/// MIDI channel pan. Generic samplers that own a stereo image can apply it
	/// as an outer balance layer; mono/irrelevant samplers keep the no-op.
	virtual void SetPan(float newPan) {(void)newPan;}
	/// Ready linear dynamic scale used only for irreversible pruning decisions.
	/// The actual output gain is owned by the outer note sampler.
	virtual void SetPruneGain(float gain) {(void)gain;}
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

/// Generic note source which needs resolved note-on parameters at construction.
/// This is the path for physical models whose timbre depends on strike; raw
/// MIDI velocity never reaches the sampler object.
typedef Funal::CopyableDelegate<GenericSamplerRef(
	float freq, float volume, unsigned sampleRate, const NoteOnParams& noteParams
)> DynamicGenericInstrument;

/// Ударный инструмент - источник семплеров нот.
typedef Funal::Delegate<GenericSamplerRef(
	float volume, unsigned sampleRate
)> GenericDrumInstrument;

/// Фабрика модификаторов - источник модификаторов семплов.
typedef Funal::CopyableDelegate<GenericModifier(
	float freq, float volume, unsigned sampleRate
)> GenericModifierFactory;

INTRA_WARNING_POP
