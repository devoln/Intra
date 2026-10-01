#pragma once


#include "Intra/Math/SineRange.h"

#include "Intra/Range/Span.h"

#include "Utils/FixedArray.h"
#include "Utils/Optional.h"

#include "Container/Sequential/Array.h"

#include "Types.h"
#include "Filter.h"
#include "SteppedSineRange.h"
#include "WaveTable.h"
#include "Envelope.h"
#include "ExponentialAttenuation.h"
#include "Sampler.h"
#include "Instrument.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

/// Samples between two recursion steps of a per-sample modulation carrier
/// (the vibrato block of the block kernel is 16 too). Coarse enough to keep
/// the SineRange recursion away from its sub-Hz degeneracy, cheap enough to
/// beat Math::Sin: measured 0.75 ns/call against 5.2 for a phase accumulator.
static constexpr unsigned kWobbleBlockSamples = 16;

/// Modulation oscillator with depth wobble. A fixed-depth sine sounds deeper
/// and more mechanical than the same rms depth reached by irregular
/// modulation, so the carrier depth drifts between (1 - Jitter) and
/// (1 + Jitter) at JitterFreq (0.55 Hz by default, deliberately unrelated to
/// the 4-6 Hz vibrato and to anything in the note spectrum). The wobble is a
/// smoothed triangle; the carrier itself is a SteppedSineRange, because at
/// sub-Hz rates a per-sample SineRange degenerates into a linear ramp.
/// Jitter == 0 gives bit-identical output (the multiplier is exactly 1).
struct WobbleOscillator
{
	/// Carrier and its shape harmonics, all SteppedSineRange (see
	/// SteppedSineRange.h). They are NOT per-sample recursions: the two-term
	/// recursion 2·cos(dphi)·s2 − s1 degenerates into a LINEAR RAMP once
	/// 2·cos(dphi) rounds to exactly 2 in float32 (dphi²/2 below half an ulp
	/// of 1.0, dphi² < 2⁻²⁴ ≈ 6e-8 — measured 1.87 Hz at 48 kHz, 1.71 Hz at
	/// 44.1 kHz) — the vibrato then becomes a constant pitch rise, ±200 cents
	/// over seconds (Update 226, пан-флейта C5: узел 1.86 Гц, у владельца
	/// «нота уезжает на октаву»). Stepping the same recursion once per 16
	/// samples moves that threshold down to 0.12 Hz and keeps it cheaper than
	/// the per-sample Math::Sin a phase accumulator needs (u236h).
	SteppedSineRange Carrier;
	/// Harmonics of the modulation shape; they run on the same phase as the
	/// carrier, so the shape becomes impulsive, like the bank's modulator.
	SteppedSineRange Carrier2, Carrier3, Carrier4, Carrier5;
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

	/// blockSamples is the caller's step: 1 (or 0) for per-sample consumers,
	/// the vibrato block for the block kernel (which steps the oscillator once
	/// per block and holds the value).
	WobbleOscillator(float amplitude, float phase, float deltaPhase,
		float jitter, float jitterDeltaPhase,
		unsigned blockSamples = kWobbleBlockSamples,
		float harm2 = 0, float harm3 = 0, float harm4 = 0, float harm5 = 0):
		Carrier(amplitude, phase, deltaPhase, blockSamples),
		Carrier2(amplitude, phase, 2.0f*deltaPhase, blockSamples),
		Carrier3(amplitude, phase, 3.0f*deltaPhase, blockSamples),
		Carrier4(amplitude, phase, 4.0f*deltaPhase, blockSamples),
		Carrier5(amplitude, phase, 5.0f*deltaPhase, blockSamples),
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
		if(Harmonic2 != 0) s += Harmonic2*Carrier2.Next();
		if(Harmonic3 != 0) s += Harmonic3*Carrier3.Next();
		if(Harmonic4 != 0) s += Harmonic4*Carrier4.Next();
		if(Harmonic5 != 0) s += Harmonic5*Carrier5.Next();
		return s*Norm*Value*(1.0f + Jitter*w);
	}
};

/// FluidSynth's lfo, verbatim (Update 234, src/rvoice/fluid_lfo.h): the value
/// starts at 0, walks a straight line by one increment per step and reflects
/// into ±1 — a TRIANGLE; FluidSynth has no sine lfo at all. It does not move
/// until the delay has passed, and its steps sit on a fixed grid from
/// note-on: FluidSynth advances it once per FLUID_BUFSIZE (64) samples and
/// holds the pitch inside the whole buffer (fluid_rvoice_write), and that
/// grid is exactly what the rendered bank carries.
struct Sf2TriangleLfo
{
	/// Current value, ±1 (the depth of the destination is applied by the callers).
	float Value = 0;
	/// Per-step increment: 4*StepSamples*Frequency/sampleRate (fluid_voice.c).
	float Increment = 0;
	/// Samples between two steps: 64 = FluidSynth's FLUID_BUFSIZE. Per-sample
	/// consumers (noise layers) pass 1 — the same line, sampled finer.
	unsigned StepSamples = 64;
	/// Absolute sample index of the next step. The delay is rounded UP to this
	/// grid, exactly like FluidSynth checks the step's own tick count.
	unsigned NextStep = 0;

	/// Sets up the vib LFO of one note. Frequency is the freqVibLFO gene in Hz,
	/// delaySeconds the decoded delayVibLFO timecents (preset and instrument
	/// raw values ADD, so the seconds MULTIPLY).
	void Init(float frequency, float sampleRate, float delaySeconds, unsigned stepSamples)
	{
		// FluidSynth truncates output_rate*time, then compares tick counts.
		InitIncrement(4.0f*float(stepSamples)*Math::Max(frequency, 0.0f)/sampleRate,
			unsigned(Math::Max(delaySeconds, 0.0f)*sampleRate), stepSamples);
	}

	/// Same, for callers that already hold the per-step increment and the delay
	/// in samples (the wavetable path scales both once per note).
	void InitIncrement(float increment, unsigned delaySamples, unsigned stepSamples)
	{
		StepSamples = stepSamples;
		Increment = increment;
		NextStep = ((delaySamples + stepSamples - 1)/stepSamples)*stepSamples;
	}

	/// Value held during the step that starts at absolute sample index
	/// `elapsed`: steps every grid point up to it. Frozen at 0 before the delay
	/// (fluid_lfo_calc returns without moving).
	INTRA_FORCEINLINE float ValueAt(unsigned elapsed)
	{
		while(NextStep <= elapsed)
		{
			Value += Increment;
			if(Value > 1.0f)
			{
				Increment = -Increment;
				Value = 2.0f - Value;
			}
			else if(Value < -1.0f)
			{
				Increment = -Increment;
				Value = -2.0f - Value;
			}
			NextStep += StepSamples;
		}
		return Value;
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
	/// SF2 triangle lfo (see Vibrato::Sf2): FluidSynth's vib LFO, verbatim.
	/// When Sf2 is set the sine fields above are ignored.
	bool Sf2 = false;
	float Sf2Increment = 0;    // per step (the sampler runs it on kSf2LfoBlock samples)
	float Sf2Cents = 0;        // vibLfoToPitch gene, cents (triangle peak)
	float Sf2DelaySamples = 0; // decoded delayVibLFO timecents, in samples
};

/// One additive core harmonic of a vocal preset. The reference bank's sustained
/// movement is per-harmonic: every overtone breathes at its own rate, depth and
/// phase (measured by `am54cores.mjs`, see kVoiceCore* in InstrumentLibrary.cpp),
/// while a wave table can only carry one movement for all of them (a line baked
/// into the table either lands on the dft grid and stops moving, or falls off it
/// and starts rippling at the table's repeat rate). The first cores therefore
/// leave the table and become plain sine oscillators with their own AM.
struct VoiceCorePartial
{
	/// Amplitude in the same units the table's lines use (see BuildVoiceTable).
	float Amplitude = 0;
	/// Carrier phase step, radians per sample (the sampler never sees a rate).
	float CarrierDelta = 0;
	/// Initial carrier phase, radians.
	float CarrierPhase = 0;
	/// Relative depth of the amplitude modulation: value*(1 + Depth*sin()).
	float Depth = 0;
	/// Modulation phase step, radians per sample.
	float AmDelta = 0;
	/// Modulation phase, radians (measured: the cores must NOT breathe in
	/// lockstep — that is exactly what the ear heard as a mechanical pump).
	float AmPhase = 0;
};

/// Number of additive cores a preset may have (h1..h8).
static constexpr size_t kVoiceCoreMax = 8;

/// Additive core set of ONE note: built at note-on from the measured AM table
/// of the preset and the note's own profile amplitudes.
struct VoiceCoreSet
{
	unsigned Count = 0;
	VoiceCorePartial Partials[kVoiceCoreMax];
};

/// Builds the additive core set of a note for a preset slot (defined in
/// InstrumentLibrary.cpp, where the measured data lives; called from
/// WaveTableInstrument::operator()). slot is the preset index in kVoicePresets,
/// scale is WaveTable::CoreScale of the note's own table, so the cores land in
/// exactly the units the table used for the same lines.
noinline void BuildVoiceCores(size_t slot, float freq, unsigned sampleRate, float scale, VoiceCoreSet& dst);

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
	/// Additive cores (empty in the common case: the table then carries every
	/// overtone and the note renders bit-identically to before).
	VoiceCoreSet Cores;

	/// Таблица ОНСЕТА с теми же фазами, что у сустейн-таблицы: до Duration
	/// играет с весом OnsetWeight, затем Crossfade секунд линейно уходит в ноль.
	/// Память указателя принадлежит WaveTableInstrument (кеш на инструмент),
	/// семплер только читает фрагмент, поэтому указатель живёт дольше ноты.
	const float* OnsetSamples = nullptr;
	size_t OnsetLength = 0;
	/// Длительность онсета и кроссфейда в СЕМПЛАХ этой ноты.
	unsigned OnsetHold = 0;
	unsigned OnsetFade = 0;
	/// Громкость онсет-слоя (см. OnsetTableDesc.Weight): таблица нормируется
	/// по своей Σ, поэтому уровень онсета нельзя задать профилем — только
	/// этим множителем. Он может быть больше 1.
	float OnsetWeight = 0;

	/// Вторая ступень онсета (Update 236, флейты Titanic): «вспышка» обертонов
	/// ПОСЛЕ тёмного старта. Та же длина и те же фазы линий, что у сустейна.
	/// Вес 0 до OnsetDelay2, линейный подъём до OnsetWeight2 за OnsetRise2
	/// семплов, затем линейный уход в ноль за OnsetFade2. Нулевая длина =
	/// ступени нет (все прочие инструменты рендерятся по-прежнему).
	const float* OnsetSamples2 = nullptr;
	size_t OnsetLength2 = 0;
	unsigned OnsetDelay2 = 0;
	unsigned OnsetRise2 = 0;
	unsigned OnsetFade2 = 0;
	float OnsetWeight2 = 0;

	/// Builds the whole struct at once. Utils::Optional does not set its
	/// "has value" flag in operator= (it is set by the constructors only), so
	/// everything here must be constructed, never assigned field by field.
	static WaveTableSamplerParams Make(float rate, float attenuationPerSample, float volume,
		size_t channelDeltaSamples, const struct Envelope& envelope,
		Utils::Optional<VibratoParams> vibrato = null, const VoiceCoreSet& cores = VoiceCoreSet(),
		const float* onsetSamples = nullptr, size_t onsetLength = 0,
		unsigned onsetHold = 0, unsigned onsetFade = 0, float onsetWeight = 0,
		const float* onsetSamples2 = nullptr, size_t onsetLength2 = 0,
		unsigned onsetDelay2 = 0, unsigned onsetRise2 = 0, unsigned onsetFade2 = 0,
		float onsetWeight2 = 0)
	{
		return WaveTableSamplerParams{rate, attenuationPerSample, volume, channelDeltaSamples,
			envelope, Move(vibrato), cores, onsetSamples, onsetLength, onsetHold, onsetFade, onsetWeight,
			onsetSamples2, onsetLength2, onsetDelay2, onsetRise2, onsetFade2, onsetWeight2};
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

	/// SF2 triangle lfo of this note (see Vibrato::Sf2). Sf2On selects it and
	/// the sine oscillator above is then never stepped; Sf2Cents is the
	/// vibLfoToPitch depth applied as an exact 2^(value*cents/1200).
	Sf2TriangleLfo mSf2Lfo;
	bool mSf2On = false;
	float mSf2Cents = 0;

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

	/// Additive cores of this note: the overtone cores the table does NOT carry,
	/// each with its own amplitude modulation (see VoiceCorePartial). Empty for
	/// every layer that keeps all of its overtones in the table, and then the
	/// note renders bit-identically to before.
	VoiceCoreSet mVoiceCores;

	/// Таблица онсета (см. OnsetTableDesc): читается с теми же offset/rate,
	/// что и сустейн, вес mOnsetWeight держится mOnsetHold семплов, затем
	/// линейно уходит в ноль за mOnsetFade. Нулевая длина = онсета нет.
	const float* mOnsetSamples = nullptr;
	size_t mOnsetLength = 0;
	unsigned mOnsetHold = 0;
	unsigned mOnsetFade = 0;
	float mOnsetWeight = 0;

	/// Вторая ступень онсета (см. WaveTableSamplerParams.OnsetSamples2): вес
	/// 0 до задержки, линейный подъём, затем линейный уход в ноль.
	const float* mOnsetSamples2 = nullptr;
	size_t mOnsetLength2 = 0;
	unsigned mOnsetDelay2 = 0;
	unsigned mOnsetRise2 = 0;
	unsigned mOnsetFade2 = 0;
	float mOnsetWeight2 = 0;

	/// Вес онсета в семпле ноты с номером elapsed: 0 — онсета нет (или он уже
	/// отыгран). Одна формула на все три пути рендера (vibrato / constant rate).
	INTRA_FORCEINLINE float OnsetWeightAt(unsigned elapsed) const
	{
		if(mOnsetLength == 0 || mOnsetWeight == 0.0f) return 0.0f;
		if(elapsed < mOnsetHold) return mOnsetWeight;
		if(mOnsetFade == 0 || elapsed >= mOnsetHold + mOnsetFade) return 0.0f;
		return mOnsetWeight*float(mOnsetHold + mOnsetFade - elapsed)/float(mOnsetFade);
	}

	INTRA_FORCEINLINE float OnsetWeightAt2(unsigned elapsed) const
	{
		if(mOnsetLength2 == 0 || mOnsetWeight2 == 0.0f) return 0.0f;
		if(elapsed < mOnsetDelay2) return 0.0f;
		const unsigned t = elapsed - mOnsetDelay2;
		if(mOnsetRise2 == 0 || t >= mOnsetRise2)
		{
			if(mOnsetFade2 == 0 || t >= mOnsetRise2 + mOnsetFade2) return 0.0f;
			return mOnsetWeight2*float(mOnsetRise2 + mOnsetFade2 - t)/float(mOnsetFade2);
		}
		return mOnsetWeight2*float(t)/float(mOnsetRise2);
	}

	/// Carrier of one core: a plain per-sample SineRange, unit amplitude (the
	/// note envelope carries the level). The carriers are the note's harmonics
	/// h1..h8 — CarrierDelta is (i+1)*freq radians per sample — so they run at
	/// audio rates, the exact range that recursion is meant for; a block step
	/// would alias them (unlike the sub-audio consumers, see SineRange's own
	/// working-range note). Nothing is allocated per note.
	SineRange<float> mCoreOsc[kVoiceCoreMax];
	/// Modulation phase of one core (accumulated, evaluated per sub-block).
	float mCoreAmPhase[kVoiceCoreMax];
	/// Pitch multiplier of the cores (MultiplyPitch); 1 is the common case.
	float mCorePitch = 1.0f;

	/// Builds the core carriers from mVoiceCores (called at note-on and after a
	/// pitch change; the modulation phase is not reset here).
	void buildCoreOscillators();

	/// Adds the additive cores of ONE chunk into dst. The envelope coefficients
	/// are the ones the table path uses for the same chunk (exp*expStep^i and
	/// lin + linStep*i), so the cores follow the note's envelope exactly and no
	/// second envelope state has to be kept.
	void addCorePartials(Span<float> dstLeft, Span<float> dstRight,
		float exp, float expStep, float lin, float linStep);

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
		if(mVoiceCores.Count)
		{
			mCorePitch *= freqMultiplier;
			buildCoreOscillators();
		}
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

/// Отдельная таблица ОНСЕТА: тело ноты со спектром первых сотен миллисекунд.
/// У записанных сэмплов банка он другой: основной тон выше, отдельные обертоны
/// подняты, другие приглушены (формантный сдвиг). Таблица строится с теми же
/// фазами, что у сустейн-таблицы, и читается параллельно с ней с весом,
/// который уходит по времени (см. WaveTableSamplerParams.OnsetSamples).
struct OnsetTableDesc
{
	/// Секунды от note-on: столько времени онсет держит вес Weight.
	float Duration = 0;
	/// Секунды линейного ухода онсета в ноль.
	float Crossfade = 0;
	/// Громкость онсета на Duration. Таблица нормируется по СВОЕЙ сумме
	/// амплитуд, поэтому её абсолютный уровень от профиля не зависит: профиль
	/// задаёт только форму, этот множитель — во сколько раз онсет громче тела
	/// по каждой линии в среднем (больше 1 — нормально, слой аддитивный).
	float Weight = 1;
	/// Амплитуды гармоник онсета, h1..h16 (та же нормировка, что у сустейн-
	/// таблицы). Фазы не задаются: линия берёт фазу своего бина, то есть ровно
	/// ту же, что у сустейна (см. SustainPhaseOfBin), поэтому кроссфейд двух
	/// таблиц — плавная смена спектра, а не лотерея интерференции.
	/// Амплитуда может быть отрицательной: линия в онсете ТИШЕ сустейна и
	/// обязана складываться с ним в противофазе.
	Array<float> Harmonics;

	/// Вторая ступень (Update 236, флейта 43/115): спектр «вспышки» обертонов
	/// ПОСЛЕ тёмного старта. Механизм тот же (фазы линии — из бина сустейна,
	/// амплитуды могут быть отрицательными), но вес свой: 0 до Delay2, рост до
	/// Weight2 за Rise2 секунд, затем линейный уход в ноль за Fade2. Пустой
	/// Harmonics2 или Rise2 == 0 = ступени нет.
	float Delay2 = 0;
	float Rise2 = 0;
	float Fade2 = 0;
	float Weight2 = 0;
	Array<float> Harmonics2;
};

struct WaveTableCache
{
	typedef Delegate<WaveTable(float freq, unsigned sampleRate)> GeneratorType;
	mutable Array<WaveTable> Tables;
	GeneratorType Generator;
	bool AllowMipmaps = false;

	WaveTable& Get(float freq, unsigned sampleRate) const;

	/// Таблица онсета: амплитуды линий заданы профилем (см. OnsetTableDesc),
	/// фазы берутся из бинов так же, как у сустейна. Кеш общий с обычными
	/// таблицами по механике, но таблиц здесь ровно столько же, сколько зон
	/// регистра: профиль обязан быть функцией только частоты.
	/// slot различает НЕСКОЛЬКО профилей на одной частоте (у флейт их два:
	/// тёмный старт и вспышка обертонов). Без него второй профиль получал бы
	/// таблицу первого; отдельный кеш на профиль решал бы ту же задачу, но
	/// стоил бы конструирования ещё одного WaveTableCache у КАЖДОГО
	/// инструмента (замер: 3 620 байт кода).
	WaveTable& Get(float freq, unsigned sampleRate, Span<const float> harmonics, unsigned slot) const;

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
	/// SF2 lfo, played exactly as FluidSynth plays it (Update 234): a TRIANGLE
	/// that stays frozen for Delay seconds and then steps once per 64 samples
	/// (FLUID_BUFSIZE). Cents is the vibLfoToPitch gene fed through ct2hz_real,
	/// so the read rate is multiplied by 2^(value*Cents/1200). Delay is in
	/// SECONDS — the decoded timecents of the preset and instrument zones
	/// MULTIPLY (their raw values add). When Sf2 is set the sine fields above
	/// are ignored: FluidSynth has no ramp, no jitter and no shape harmonics.
	bool Sf2 = false;
	float Sf2Frequency = 0; // Hz, the freqVibLFO gene
	float Sf2Cents = 0;
	float Sf2Delay = 0;
};

/// Shared vibrato lfo of one note (same math as WaveTableSampler: a
/// WobbleOscillator gated by Delay/Ramp). A layer that starts with the note
/// body and calls Next() once per output sample stays in phase with it, so
/// skirts, breath and bloom breathe together with the tone instead of beating
/// against it.
struct VibratoLfo
{
	WobbleOscillator Oscillator;
	/// SF2 triangle lfo of the same note (see Vibrato::Sf2). The wavetable tone
	/// runs it once per 64-sample step; this per-sample consumer runs it every
	/// sample, which is the same line sampled finer (below 0.1 cent apart).
	Sf2TriangleLfo Triangle;
	bool Sf2 = false;
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
		if(v.Sf2)
		{
			// FluidSynth's vib LFO: triangle, hard delay, no ramp and no jitter.
			if(v.Sf2Cents == 0.0f) return;
			Triangle.Init(v.Sf2Frequency, float(sampleRate), v.Sf2Delay, 1);
			// Linear read-rate deviation of the triangle peak. FluidSynth feeds
			// the cents through ct2hz_real (an exponential), but at ±14 cents the
			// difference between 2^(x/1200) and a straight line is under 0.06
			// cent — far below the audible and measurable threshold.
			Value = Math::Pow(2.0f, v.Sf2Cents/1200.0f) - 1.0f;
			Active = true;
			Sf2 = true;
			return;
		}
		if((v.Value == 0.0f && v.Tremolo == 0.0f) || v.Frequency == 0.0f) return;
		// The oscillator is normalized to ±1: Value is applied in Next().
		// This consumer reads the lfo once per output sample, so the carrier
		// interpolates between its recursion steps.
		Oscillator = WobbleOscillator(1.0f, 0.0f,
			2.0f*float(PI)*v.Frequency/float(sampleRate), v.Jitter,
			2.0f*float(PI)*v.JitterFreq/float(sampleRate),
			kWobbleBlockSamples,
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
		if(Sf2)
		{
			LastGated = Triangle.ValueAt(ElapsedSamples);
			ElapsedSamples++;
			return LastGated*Value;
		}
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
	/// Amplitude depth of the same lfo (see Vibrato::Tremolo). A pad whose
	/// bank model breathes gets a small value; the read rate stays at 1 and
	/// only the amplitude moves.
	float VibratoTremolo = 0;
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

	/// Additive cores slot + 1 (0 = the table carries every overtone). The set
	/// itself is built per note by BuildVoiceCores (see VoiceCoreSet): the bank's
	/// movement is per-harmonic, and a wave table cannot carry that.
	uint8 Cores = 0;

	/// Таблица ОНСЕТА (см. OnsetTableDesc): строится на первый note-on
	/// частоты (кеш внутри WaveTableInstrument::OnsetTables), фазы линий те же,
	/// что у сустейн-таблицы, поэтому кроссфейд не гасит гармоники. nullptr =
	/// обычное поведение (одна таблица на всю ноту).
	Funal::CopyableDelegate<OnsetTableDesc(float freq)> OnsetProfile;

	/// Кеш таблиц онсета (живёт рядом с Tables: тот же срок, тот же владелец).
	/// Обе ступени онсета (тёмный старт и вспышка, см. OnsetTableDesc) лежат в
	/// ЭТОМ кеше и различаются полем слота (ProfileSlot). Отдельный второй кеш
	/// был бы честнее на вид, но его конструирование оплачивает каждый
	/// инструмент библиотеки (≈90 штук внутри одной функции) — 3 620 байт
	/// кода на пустом месте.
	mutable WaveTableCache OnsetTables;

	/// Read-rate multiplier: a detuned singer reads the same table faster, so
	/// the detune is not quantized by the table's DFT grid. 1 (the default) is
	/// an exact IEEE multiply, so other instruments render bit-identically.
	float FreqScale = 1;

	/// Note-dependent detune, given as a FORMULA rather than a table: with
	/// FreqScaleRefHz above 0 the read rate is 2^(FreqScaleCents*FreqScaleRefHz/
	/// freq/1200) and FreqScale is ignored. A read offset of d cents beats
	/// harmonic k at k*f0*d/1731 Hz, so d ~ 1/f is exactly a constant beat rate
	/// in hertz - the reference bank's sustained movement is a fixed rate, not a
	/// fixed detune, and a fixed offset doubles the rate per octave. 0 (the
	/// default) leaves FreqScale in charge and renders bit-identically.
	float FreqScaleCents = 0;
	float FreqScaleRefHz = 0;

	WaveTableSampler operator()(float freq, float volume, unsigned sampleRate) const;
};

/// Task that generates samples and adds them to the buffer of the given context.

INTRA_WARNING_POP
