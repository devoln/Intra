#pragma once


#include "Intra/Math/SineRange.h"

#include "Intra/Range/Span.h"

#include "Utils/FixedArray.h"
#include "Utils/Optional.h"

#include "Types.h"
#include "Filter.h"
#include "WaveTable.h"
#include "Envelope.h"
#include "ExponentialAttenuation.h"
#include "Sampler.h"
#include "Instrument.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

/// Modulation oscillator with depth wobble. A fixed-depth sine sounds deeper
/// and more mechanical than the same rms depth reached by irregular
/// modulation, so the carrier depth drifts between (1 - Jitter) and
/// (1 + Jitter) at JitterFreq (0.55 Hz by default, deliberately unrelated to
/// the 4-6 Hz vibrato and to anything in the note spectrum). The wobble is a
/// phase accumulator with a smoothed triangle, not a second SineRange: at
/// sub-Hz rates SineRange degenerates into a linear ramp.
/// Jitter == 0 gives bit-identical output (the multiplier is exactly 1).
struct WobbleOscillator
{
	Intra::SineRange<float> Carrier;
	/// Harmonics of the modulation shape; they run on the same phase as the
	/// carrier, so the shape becomes impulsive, like the bank's modulator.
	Intra::SineRange<float> Carrier2;
	Intra::SineRange<float> Carrier3;
	Intra::SineRange<float> Carrier4;
	Intra::SineRange<float> Carrier5;
	float Harmonic2 = 0;
	float Harmonic3 = 0;
	float Harmonic4 = 0;
	float Harmonic5 = 0;
	/// 1/(1 + h2 + h3 + h4 + h5): keeps the shape within ±1, while the
	/// instrument compensates the rms depth with (1+Σh)/sqrt(1+Σh²).
	float Norm = 1;
	float WobblePhase = 0;  // radians, [0, 2π)
	float WobbleStep = 0;   // radians/sample
	float Value = 0;
	float Jitter = 0;

	WobbleOscillator(decltype(nullptr) = nullptr) {}

	WobbleOscillator(float amplitude, float phase, float deltaPhase,
		float jitter, float jitterDeltaPhase,
		float harm2 = 0, float harm3 = 0, float harm4 = 0, float harm5 = 0):
		Carrier(amplitude, phase, deltaPhase),
		Carrier2(amplitude, phase, 2.0f*deltaPhase),
		Carrier3(amplitude, phase, 3.0f*deltaPhase),
		Carrier4(amplitude, phase, 4.0f*deltaPhase),
		Carrier5(amplitude, phase, 5.0f*deltaPhase),
		Harmonic2(harm2), Harmonic3(harm3), Harmonic4(harm4), Harmonic5(harm5),
		Norm(1.0f/(1.0f + harm2 + harm3 + harm4 + harm5)),
		WobbleStep(jitterDeltaPhase),
		Value(amplitude), Jitter(jitter) {}

	INTRA_FORCEINLINE float Next()
	{
		const float twoPi = 2.0f*float(PI);
		WobblePhase += WobbleStep;
		if(WobblePhase >= twoPi) WobblePhase -= twoPi;
		const float p = WobblePhase*(1.0f/float(PI));       // 0..2
		const float t = p < 1.0f? p: 2.0f - p;              // 0..1 triangle
		const float w = ((3.0f - 2.0f*t)*t)*t*2.0f - 1.0f;  // smoothed, ±1
		float s = Carrier.Next();
		// With all harmonics zero the branches are not taken and Norm is
		// exactly 1, so the output is bit-identical to a plain sine.
		if(Harmonic2 != 0) s += Harmonic2*Carrier2.Next();
		if(Harmonic3 != 0) s += Harmonic3*Carrier3.Next();
		if(Harmonic4 != 0) s += Harmonic4*Carrier4.Next();
		if(Harmonic5 != 0) s += Harmonic5*Carrier5.Next();
		return s*Norm*Value*(1.0f + Jitter*w);
	}
};

/// Vibrato of one layer, already scaled to the sample rate (the sampler never
/// sees a rate): DeltaPhase = 2π·Frequency/sampleRate, delay/ramp in samples.
/// Value is the relative read-rate deviation (FM), Tremolo the relative volume
/// deviation of the same lfo (AM), Jitter the depth wobble, Harm2..5 the shape
/// harmonics, BlockSamples the number of samples per lfo step (0 = the lfo is
/// read once per sample).
struct VibratoParams
{
	float DeltaPhase = 0;
	float Value = 0;
	float Tremolo = 0;
	float DelaySamples = 0;
	float RampSamples = 0;
	float Jitter = 0;
	float JitterDeltaPhase = 0;
	float Harm2 = 0;
	float Harm3 = 0;
	float Harm4 = 0;
	float Harm5 = 0;
	unsigned BlockSamples = 0;
};

/// Everything one WaveTableSampler needs to start a note. Vibrato is absent in
/// the common case, which keeps the constant-rate fast path.
struct WaveTableSamplerParams
{
	float Rate = 1;
	/// Per-sample attenuation factor (exp(-coefficient/sampleRate)).
	float AttenuationPerSample = 0;
	float Volume = 1;
	size_t ChannelDeltaSamples = 0;
	Envelope Envelope;
	Utils::Optional<VibratoParams> Vibrato;

	/// Builds the whole struct at once. Utils::Optional does not set its
	/// "has value" flag in operator= (it is set by the constructors only), so
	/// everything here must be constructed, never assigned field by field.
	static WaveTableSamplerParams Make(float rate, float attenuationPerSample, float volume,
		size_t channelDeltaSamples, const struct Envelope& envelope,
		Utils::Optional<VibratoParams> vibrato = null)
	{
		return WaveTableSamplerParams{rate, attenuationPerSample, volume, channelDeltaSamples,
			envelope, Move(vibrato)};
	}
};

/// Base sampler for wave-table notes: it reads external tables only, allocates
/// nothing of its own and keeps a fixed spectrum for the whole note, which is
/// what lets it render in a single pass.
class WaveTableSampler: public Sampler
{
protected:
	/// Points at the currently used sample data.
	const float* mSampleFragmentStart;
	unsigned mSampleFragmentLength;

	INTRA_FORCEINLINE Span<const float> SampleFragment() const
	{return {mSampleFragmentStart, mSampleFragmentLength};}
	
	INTRA_FORCEINLINE Span<const float> SampleFragment(size_t startIndex, size_t maxCount) const
	{return SampleFragment().Drop(startIndex).Take(maxCount);}

	/// Left channel offset from the start of the mSampleFragment period.
	float mFragmentOffset;

	/// Integer part of the right channel offset from mRightSampleFragment.
	unsigned mRightFragmentOffset;

	/// Playback speed of mSampleFragment.
	float mRate;

	/// Every volume factor except the envelope, pan and reverb. The exponent
	/// attenuator mirrors what WaveFormSampler may bake into its own data.
	ExponentAttenuator mExpAtten;

	/// Per-channel multipliers.
	float mLeftMultiplier, mRightMultiplier;

	/// Read-rate oscillator: speed is mRate*(1 + oscillator value).
	WobbleOscillator mFreqOscillator;

	/// Amplitudes of the FM and AM of that lfo (the oscillator itself is
	/// normalized to ±1). Tremolo == 0 keeps the previous behaviour.
	float mVibratoValue = 0;
	float mVibratoTremolo = 0;

	/// Vibrato with a non-zero depth runs through the scalar per-sample
	/// kernel instead of the constant-rate SIMD kernels.
	bool mHasVibrato = false;

	/// Samples per lfo step. 0 is the per-sample path; N > 0 keeps the read
	/// rate constant inside a block of N samples, so the constant-rate kernels
	/// apply and the lfo is stepped once per block.
	unsigned mVibratoBlock = 0;

	/// Vibrato gate: its depth is multiplied by a ramp that grows from 0 to 1
	/// over [Delay, Delay + Ramp], so a note does not start with full vibrato.
	float mVibratoDelaySamples = 0;
	float mVibratoRampSamples = 0;
	unsigned mElapsedSamples = 0;

	/// Note envelope, e.g. ADSR; exponential attenuation is applied on top.
	Envelope mEnvelope;

public:
	WaveTableSampler(decltype(nullptr)=nullptr) {}

	WaveTableSampler(Span<const float> periodicWave, const WaveTableSamplerParams& params);

	/// Current vibrato gate value for the sample with the given index.
	INTRA_FORCEINLINE float VibratoGateAt(unsigned elapsed) const
	{
		if(mVibratoRampSamples > 0)
		{
			if(elapsed >= mVibratoDelaySamples)
			{
				const float g = (float(elapsed) - mVibratoDelaySamples)/mVibratoRampSamples;
				return g < 1.0f ? g : 1.0f;
			}
			return 0;
		}
		return elapsed >= mVibratoDelaySamples ? 1.0f : 0.0f;
	}
	INTRA_FORCEINLINE float VibratoGate() const {return VibratoGateAt(mElapsedSamples);}
	INTRA_FORCEINLINE float VibratoGateStep() const
	{
		return mVibratoRampSamples > 0 ? 1.0f/mVibratoRampSamples : 0.0f;
	}

	void MoveConstruct(void* dst) override {new(dst) WaveTableSampler(Move(*this));}

	virtual bool OwnExponentialAttenuatedDataArray() const noexcept {return false;}

	bool Generate(SamplerTaskContainer& dstTasks, size_t offsetInSamples, size_t numSamples) override;

	/// Direct rendering used by nested samplers (NoteSampler).
	/// GenerateMono returns the untouched remainder (nullptr if the buffer is full).
	Span<float> GenerateMono(Span<float> ioDst);
	size_t GenerateStereo(Span<float> dstLeft, Span<float> dstRight);

	void MultiplyPitch(float freqMultiplier) final
	{
		mRate *= freqMultiplier;
		if(Abs(mRate - 1) < 0.0001f) mRate = 1;
	}

	void MultiplyVolume(float volumeMultiplier) final {mExpAtten.Factor *= volumeMultiplier;}

	void SetPan(float newPan) final
	{
		mRightMultiplier = (newPan + 1) / 2;
		mLeftMultiplier = 1 - mRightMultiplier;
	}

	void NoteRelease() final {mEnvelope.StartLastSegment();}

#ifdef INTRA_UI_METERS
	/// Note envelope level for the web-UI indicator (see Sampler::GetLevel).
	float GetLevel() const override {return mEnvelope.CurrentSegment.Volume;}
#endif

private:
	size_t renderDirect(Span<float> dstLeft, Span<float> dstRight);

	/// Renders one envelope segment at a constant read rate. Both the plain
	/// layers (rate for the whole segment) and the block vibrato (rate from the
	/// lfo once per block) go through it, so kernel selection lives in one place
	/// instead of a copy per branch. An empty dstRight means mono.
	noinline size_t renderConstantRate(Span<float> dstLeft, Span<float> dstRight,
		const EnvelopeSegment& segment, float& ioOffsetL, float& ioOffsetR,
		size_t channelDelta);

	void generateWithDefaultRate(SamplerTaskContainer& dstTasks, size_t offsetInSamples, size_t numSamples);

	void generateWithVaryingRate(SamplerTaskContainer& dstTasks, size_t offsetInSamples, size_t numSamples);

	template<bool FreqOsc, bool Adsr>
	void generateWithVaryingRateTask(Span<float> dstLeft, Span<float> dstRight);
};

struct WaveTableCache
{
	typedef Delegate<WaveTable(float freq, unsigned sampleRate)> GeneratorType;
	mutable Array<WaveTable> Tables;
	GeneratorType Generator;
	bool AllowMipmaps = false;

	WaveTable& Get(float freq, unsigned sampleRate) const;

	WaveTableCache() {}
	WaveTableCache(const WaveTableCache&) = delete;
	WaveTableCache& operator=(const WaveTableCache&) = delete;
	WaveTableCache(WaveTableCache&&) = default;
	WaveTableCache& operator=(WaveTableCache&&) = default;
};

/// Vibrato of one note. Value is the relative read-rate deviation (0.005 ≈
/// ±8.7 cents), Delay/Ramp are in seconds. Tremolo is a simultaneous amplitude
/// modulation of the same lfo: what "breathes" in a flute is the breath (a
/// NoiseSampler layer), while the table tone itself stays steady.
struct Vibrato
{
	float Frequency = 0; // Hz
	float Value = 0;
	float Tremolo = 0;
	float Delay = 0; // s
	float Ramp = 0;  // s
	/// Depth wobble: 0 is a pure sine, 0.5 drifts by ±50 % at JitterFreq.
	float Jitter = 0;
	float JitterFreq = 0.55f; // Hz
	/// Amplitudes of the 2nd..5th harmonics of the modulation shape (0 = pure
	/// sine). The bank's modulator is impulsive, and an impulsive shape at the
	/// same base rate reads as a much faster tremolo. The engine normalizes the
	/// shape to ±1, so an instrument that sets harmonics must scale Value and
	/// Tremolo by (1+Σh)/sqrt(1+Σh²) to keep the rms depth.
	float Harm2 = 0;
	float Harm3 = 0;
	float Harm4 = 0;
	float Harm5 = 0;
};

/// Shared vibrato lfo of one note (same math as WaveTableSampler: a SineRange
/// gated by Delay/Ramp). A layer that starts with the note body and calls
/// Next() once per output sample stays in phase with it, so skirts, breath and
/// bloom breathe together with the tone instead of beating against it.
struct VibratoLfo
{
	WobbleOscillator Oscillator;
	float Value = 0;
	float Tremolo = 0;
	/// Normalized (±1) lfo value of the current sample, gate included; the AM
	/// branch (NoiseSampler) reads it right after Next().
	float LastGated = 0;
	float DelaySamples = 0;
	float RampSamples = 0;
	unsigned ElapsedSamples = 0;
	bool Active = false;

	void Init(const Vibrato& v, unsigned sampleRate)
	{
		if((v.Value == 0.0f && v.Tremolo == 0.0f) || v.Frequency == 0.0f) return;
		// The oscillator is normalized to ±1: Value is applied in Next().
		Oscillator = WobbleOscillator(1.0f, 0.0f,
			2.0f*float(PI)*v.Frequency/float(sampleRate), v.Jitter,
			2.0f*float(PI)*v.JitterFreq/float(sampleRate),
			v.Harm2, v.Harm3, v.Harm4, v.Harm5);
		Value = v.Value;
		Tremolo = v.Tremolo;
		DelaySamples = v.Delay*float(sampleRate);
		RampSamples = v.Ramp*float(sampleRate);
		Active = true;
	}

	INTRA_FORCEINLINE float Gate() const
	{
		if(RampSamples > 0)
		{
			if(float(ElapsedSamples) >= DelaySamples)
			{
				const float g = (float(ElapsedSamples) - DelaySamples)/RampSamples;
				return g < 1.0f ? g : 1.0f;
			}
			return 0;
		}
		return float(ElapsedSamples) >= DelaySamples ? 1.0f : 0.0f;
	}

	/// Relative rate deviation of one sample (0 = no vibrato). Also stores the
	/// normalized (±1) lfo value in LastGated for the AM branch.
	INTRA_FORCEINLINE float Next()
	{
		LastGated = Oscillator.Next()*Gate();
		ElapsedSamples++;
		return LastGated*Value;
	}
};

struct WaveTableInstrument
{
	WaveTableCache* Tables = nullptr;
	float ExpCoeff = 0;
	float VolumeScale = 0;
	float VibratoFrequency = 0;
	float VibratoValue = 0;
	EnvelopeFactory Envelope = EnvelopeFactory::Constant(1);

	/// Optional per-segment vibrato profile: when set, VibratoFrequency and
	/// VibratoValue are ignored and the vibrato is computed from the note
	/// frequency (register-dependent vibrato, e.g. a flute).
	Funal::CopyableDelegate<Vibrato(float freq)> VibratoProfile;

	/// Optional per-segment envelope profile: when set, Envelope is overridden
	/// and the segment shape is computed from the note frequency.
	Funal::CopyableDelegate<EnvelopeFactory(float freq)> EnvelopeProfile;

	/// Vibrato appearance: delay, ramp, depth wobble and its rate. Defaults
	/// keep the plain behaviour; VibratoProfile overrides them.
	float VibratoDelay = 0;
	float VibratoRamp = 0;
	float VibratoJitter = 0;
	float VibratoJitterFrequency = 0.55f;

	/// Samples per lfo step (0 = per-sample path). Extension field: it is read
	/// only by the block vibrato branch.
	unsigned VibratoBlock = 0;

	/// Read-rate multiplier: a detuned singer reads the same table faster, so
	/// the detune is not quantized by the table's DFT grid. 1 (the default) is
	/// an exact IEEE multiply, so other instruments render bit-identically.
	float FreqScale = 1;

	WaveTableSampler operator()(float freq, float volume, unsigned sampleRate) const;
};

/// Task that generates samples and adds them to the buffer of the given context.

INTRA_WARNING_POP
