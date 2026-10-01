#include "WaveTableSampler.h"
#include "ComputeKernels.h"
#include "ExponentialAttenuation.h"
#include "FixedRateTask.h"
#include "NormalRateTask.h"

#include "Generators/Sawtooth.h"
#include "Generators/Square.h"
#include "Generators/Pulse.h"
#include "Generators/WhiteNoise.h"

#include "Audio/AudioBuffer.h"

#include "Intra/Range/Span.h"

#include "Funal/Bind.h"

#include "Intra/Math/Math.h"

#include "Simd/Simd.h"

#include "Intra/Range/Mutation/Copy.h"
#include "Intra/Range/Mutation/Fill.h"
#include "Intra/Range/Mutation/Transform.h"

#include "Container/Sequential/Array.h"

#include "Random/FastUniform.h"

#include "WaveTableGeneration.h"

#ifdef INTRA_PROBE_NAN
#include <stdio.h>
#endif

// Phase step of the block vibrato: one Next() call must advance the phase by a
// whole block of the per-sample path, otherwise the vibrato would slow down by
// exactly the block length.
static inline float vibratoBlockScale(unsigned blockSamples)
{
	return blockSamples > 1? float(blockSamples): 1.0f;
}

// Vibrato of a layer, already scaled to the sample rate. Absent (null) when
// the layer has no vibrato at all, which keeps the constant-rate path.
static Utils::Optional<VibratoParams> tableVibratoParams(const Vibrato& vib,
	unsigned sampleRate, unsigned blockSamples)
{
	// FluidSynth's lfo buffer (fluidsynth_priv.h): the gene formulas of
	// fluid_voice.c are per THIS many samples and the pitch is held inside the
	// whole buffer, so the SF2 lfo brings its own step instead of the sine
	// path's kVibratoBlockSamples.
	constexpr unsigned kSf2LfoBlock = 64;
	const bool sf2 = vib.Sf2 && vib.Sf2Cents != 0.0f;
	if(!sf2 && vib.Value == 0.0f && vib.Tremolo == 0.0f && vib.Frequency == 0.0f) return null;
	VibratoParams vp;
	vp.DeltaPhase = 2*float(PI)*Math::Max(vib.Frequency, 0.0f)/float(sampleRate);
	vp.Value = vib.Value;
	vp.Tremolo = vib.Tremolo;
	vp.DelaySamples = vib.Delay*float(sampleRate);
	vp.RampSamples = vib.Ramp*float(sampleRate);
	vp.Jitter = vib.Jitter;
	vp.JitterDeltaPhase = 2*float(PI)*vib.JitterFreq/float(sampleRate);
	vp.Harm2 = vib.Harm2;
	vp.Harm3 = vib.Harm3;
	vp.Harm4 = vib.Harm4;
	vp.Harm5 = vib.Harm5;
	vp.BlockSamples = sf2? kSf2LfoBlock: blockSamples;
	vp.Sf2 = sf2;
	if(sf2)
	{
		vp.Sf2Cents = vib.Sf2Cents;
		vp.Sf2DelaySamples = vib.Sf2Delay*float(sampleRate);
		vp.Sf2Increment = 4.0f*float(kSf2LfoBlock)*Math::Max(vib.Sf2Frequency, 0.0f)/float(sampleRate);
	}
	return Utils::Optional<VibratoParams>(vp);
}

static inline auto waveTableRandGen(Span<const float> periodicWave, float rate, float volume)
{
	return Random::FastUniform<unsigned>(
		1436491347u ^ unsigned(periodicWave.Length()) ^ unsigned(rate*1537) ^ unsigned(volume * 349885300.0f)
	);
}

WaveTableSampler::WaveTableSampler(Span<const float> periodicWave, const WaveTableSamplerParams& params):
	mSampleFragmentStart(periodicWave.Data()),
	mSampleFragmentLength(unsigned(periodicWave.Length())),
	mFragmentOffset(0),
	mRightFragmentOffset(0),
	mRate(params.Rate),
	mExpAtten(ExponentAttenuator::FromFactorAndStep(params.Volume, params.AttenuationPerSample)),
	mLeftMultiplier(0.5f), mRightMultiplier(0.5f),
	mFreqOscillator(1.0f, 0, 0, 0, 0),
	mEnvelope(params.Envelope),
	mVoiceCores(params.Cores),
	mCoreAmPhase{},
	mOnsetSamples(params.OnsetSamples),
	mOnsetLength(params.OnsetLength),
	mOnsetHold(params.OnsetHold),
	mOnsetFade(params.OnsetFade),
		mOnsetWeight(params.OnsetWeight),
		mOnsetSamples2(params.OnsetSamples2),
		mOnsetLength2(params.OnsetLength2),
		mOnsetDelay2(params.OnsetDelay2),
		mOnsetRise2(params.OnsetRise2),
		mOnsetFade2(params.OnsetFade2),
		mOnsetWeight2(params.OnsetWeight2)
{
	const VibratoParams* vib = params.Vibrato != null? &params.Vibrato.Value(): nullptr;
	if(vib != nullptr)
	{
		// A block lfo is stepped once per block, so its phase step covers the block.
		// BlockSamples is also the carrier's own step: the oscillator then returns
		// the exact recursion sample at the block start (0 = per-sample caller).
		mFreqOscillator = WobbleOscillator(1.0f, 0,
			vib->DeltaPhase*vibratoBlockScale(vib->BlockSamples), vib->Jitter,
			vib->JitterDeltaPhase*vibratoBlockScale(vib->BlockSamples),
			vib->BlockSamples,
			vib->Harm2, vib->Harm3, vib->Harm4, vib->Harm5);
		mVibratoValue = vib->Value;
		mVibratoTremolo = vib->Tremolo;
		mSf2On = vib->Sf2;
		mSf2Cents = vib->Sf2Cents;
		if(mSf2On) mSf2Lfo.InitIncrement(vib->Sf2Increment,
			unsigned(Math::Max(vib->Sf2DelaySamples, 0.0f)), vib->BlockSamples);
		mHasVibrato = mSf2On || ((vib->Value != 0.0f || vib->Tremolo != 0.0f) && vib->DeltaPhase != 0.0f);
		mVibratoBlock = vib->BlockSamples;
		if(mHasVibrato && vib->RampSamples > 0)
		{
			mVibratoDelaySamples = vib->DelaySamples;
			mVibratoRampSamples = vib->RampSamples;
		}
		else if(mHasVibrato && vib->DelaySamples > 0)
		{
			//Instant on after a silent delay, no ramp.
			mVibratoDelaySamples = vib->DelaySamples;
		}
	}
	mFragmentOffset = float(waveTableRandGen(periodicWave, params.Rate, params.Volume)(mSampleFragmentLength));
	mRightFragmentOffset = (unsigned(mFragmentOffset) + unsigned(params.ChannelDeltaSamples)) % mSampleFragmentLength;
	if(mVoiceCores.Count) buildCoreOscillators();
}

void WaveTableSampler::buildCoreOscillators()
{
	for(unsigned c = 0; c < mVoiceCores.Count; c++)
	{
		const VoiceCorePartial& p = mVoiceCores.Partials[c];
		// Unit amplitude: the level comes from the note envelope (see
		// addCorePartials), and the modulation is a separate multiplier.
		// The carrier is the harmonic's own audio rate, so it gets the plain
		// SineRange recursion, stepped once per sample (see mCoreOsc).
		mCoreOsc[c] = SineRange<float>(1.0f, p.CarrierPhase, p.CarrierDelta*mCorePitch);
		mCoreAmPhase[c] = p.AmPhase;
	}
}

void WaveTableSampler::addCorePartials(Span<float> dstLeft, Span<float> dstRight,
	float exp, float expStep, float lin, float linStep)
{
	const unsigned n = unsigned(dstLeft.Length());
	if(mVoiceCores.Count == 0 || n == 0) return;
	const bool stereo = !dstRight.Empty() && mRightMultiplier != 0;
	// The modulation is slow (1.4-7 Hz), so it is evaluated once per sub-block
	// and interpolated linearly inside it: over 64 samples (1.5 ms) a 7 Hz sine
	// moves by less than 0.05 rad, and the error of the interpolation stays far
	// below the modulation depth itself.
	constexpr unsigned kAmBlock = 64;
	const float twoPi = 2.0f*float(PI);
	for(unsigned c = 0; c < mVoiceCores.Count; c++)
	{
		const VoiceCorePartial& p = mVoiceCores.Partials[c];
		auto& osc = mCoreOsc[c];
		float amPh = mCoreAmPhase[c];
		// The envelope walk is the one the constant-rate kernel does for the
		// table layers: exp*expStep^i and lin + linStep*i over the chunk.
		float ee = exp, ll = lin;
		unsigned done = 0;
		while(done < n)
		{
			const unsigned block = Min(kAmBlock, n - done);
			const float amA = 1.0f + p.Depth*Math::Sin(amPh);
			amPh += p.AmDelta*float(block);
			while(amPh >= twoPi) amPh -= twoPi;
			const float amStep = (1.0f + p.Depth*Math::Sin(amPh) - amA)/float(block);
			float am = amA;
			auto dl = dstLeft.Drop(done).Take(block);
			Span<float> dr = stereo? dstRight.Drop(done).Take(block): Span<float>();
			for(unsigned i = 0; i < block; i++)
			{
				const float s = osc.Next()*p.Amplitude*ee*ll*am;
				dl[i] += s*mLeftMultiplier;
				if(stereo) dr[i] += s*mRightMultiplier;
				ee *= expStep;
				ll += linStep;
				am += amStep;
			}
			done += block;
		}
		mCoreAmPhase[c] = amPh;
	}
}

void WaveTableSampler::generateWithDefaultRate(SamplerTaskContainer& dstTasks, size_t offsetInSamples, size_t numSamples)
{
	size_t leftOffsetInSamples = offsetInSamples;
	size_t rightOffsetInSamples = offsetInSamples;
	size_t leftSamplesLeft = numSamples;
	size_t rightSamplesLeft = numSamples;
	auto& leftEnvelope = mEnvelope;
	auto rightEnvelope = mEnvelope;
	auto& leftExpAtten = mExpAtten;
	auto rightExpAtten = mExpAtten;
	if(mRightFragmentOffset > size_t(mFragmentOffset))
	{
		if(OwnExponentialAttenuatedDataArray())
			rightExpAtten.Factor /= rightExpAtten.FactorStep;
	}

	while(leftSamplesLeft > 0)
	{
		//За одну итерацию обрабатываем не более одного сегмента огибающей
		auto leftFragment = SampleFragment(size_t(mFragmentOffset), leftSamplesLeft).Take(leftEnvelope.CurrentSegment.SamplesLeft);
		EnvelopeSegment leftSegment = leftEnvelope.CurrentSegment;
		if(!OwnExponentialAttenuatedDataArray())
		{
			//Требуется накладывать экспоненциальное затухание, причём используется сторонний периодический семпл.
			//В этом случае невозможно применить трюк с предварительным наложением экспоненты на периодический семпл.
			//Поэтому честно накладываем экспоненту и ADSR. Класс ADSR умеет делать это всё за один проход.
			leftSegment.Exp *= leftExpAtten;
		}
		if(mLeftMultiplier)
		{
			leftSegment.Exp.Factor *= mLeftMultiplier;
			dstTasks.Add<NormalRateTask>(0, leftOffsetInSamples, leftFragment, leftSegment);
		}

		leftOffsetInSamples += leftFragment.Length();
		leftSamplesLeft -= leftFragment.Length();
		leftEnvelope.CurrentSegment.Advance(leftFragment.Length());
		if(leftEnvelope.CurrentSegment.SamplesLeft == 0) leftEnvelope.StartNextSegment();
		leftExpAtten.SkipSamples(leftFragment.Length());
		mFragmentOffset += float(leftFragment.Length());
		if(mFragmentOffset >= mSampleFragmentLength)
		{
			// Если этот объект имеет собственный фрагмент семплов, то на него уже наложено экспоненциальное затухание.
			// Достаточно уменьшить один общий множитель.
			if(OwnExponentialAttenuatedDataArray()) leftExpAtten.Factor *= leftExpAtten.FactorStep;
			mFragmentOffset = 0;
		}
	}
	if(mRightMultiplier) while(rightSamplesLeft > 0)
	{
		//TODO: RightSampleFragment for WaveFormSampler
		auto rightFragment = SampleFragment(size_t(mRightFragmentOffset), rightSamplesLeft).Take(rightEnvelope.CurrentSegment.SamplesLeft);
		
		const size_t rightSamplesToProcess = rightFragment.Length();

		EnvelopeSegment rightSegment = rightEnvelope.CurrentSegment;
		if(!OwnExponentialAttenuatedDataArray())
		{
			//Требуется накладывать экспоненциальное затухание, причём используется сторонний периодический семпл.
			//В этом случае невозможно применить трюк с предварительным наложением экспоненты на периодический семпл.
			//Поэтому честно накладываем экспоненту и ADSR. Класс ADSR умеет делать это всё за один проход.
			rightSegment.Exp *= rightExpAtten;
		}
		rightSegment.Exp.Factor *= mRightMultiplier;
		dstTasks.Add<NormalRateTask>(0, offsetInSamples, rightFragment, rightSegment);
		
		rightOffsetInSamples += rightFragment.Length();
		rightSamplesLeft -= rightFragment.Length();
		rightEnvelope.CurrentSegment.Advance(rightFragment.Length());
		if(rightEnvelope.CurrentSegment.SamplesLeft == 0) rightEnvelope.StartNextSegment();
		mRightFragmentOffset += float(rightFragment.Length());
		if(mRightFragmentOffset >= mSampleFragmentLength)
		{
			// Если этот объект имеет собственный фрагмент семплов, то на него уже наложено экспоненциальное затухание.
			// Достаточно уменьшить один общий множитель.
			if(OwnExponentialAttenuatedDataArray()) rightExpAtten.Factor *= rightExpAtten.FactorStep;
			mRightFragmentOffset = 0;
		}
	}
}

bool WaveTableSampler::Generate(SamplerTaskContainer& dstTasks, size_t offsetInSamples, size_t numSamples)
{
	if(mEnvelope.CurrentSegment.SamplesLeft == 0) return false;
	// NOTE: varying-rate (vibrato/pitch-bend) rendering is not implemented in this port,
	// so notes are rendered at their base rate.
	generateWithDefaultRate(dstTasks, offsetInSamples, numSamples);
	return mEnvelope.CurrentSegment.SamplesLeft != 0;
}

// Block vibrato render, as the owner asked: the existing resampling split into blocks with a per-block rate.
//
// Inside a block the read rate is constant, so the layer renders with the same kernel as the plain layer (SIMD, four samples) while the LFO moves once per block. Envelopes still step per sample inside the block and advance one block step at its edge, so attack and decay match the per-segment path and only the read rate is quantised.
//
// A 5 Hz LFO on a 16-sample block (0.36 ms) is a pitch step two orders of magnitude below the audible threshold, and the vibrato layer then costs the same as the plain one.
//
// The same code serves the plain layer (block = the whole envelope segment, unchanged) and block vibrato.
size_t WaveTableSampler::renderConstantRate(Span<float> dstLeft, Span<float> dstRight,
	const EnvelopeSegment& segment, float& ioOffsetL, float& ioOffsetR, size_t channelDelta)
{
	const size_t n = dstLeft.Length();
	if(n == 0 || mSampleFragmentLength == 0) return 0;
	const bool stereo = !dstRight.Empty();
	const bool blockVibrato = mHasVibrato && mVibratoBlock > 0;
	// Without vibrato the rate is constant per segment, with vibrato per block.
	const size_t blockLimit = blockVibrato? size_t(mVibratoBlock): n;
	const Span<const float> src = SampleFragment();
	float exp = segment.Exp.Factor;
	const float expStep = segment.Exp.FactorStep;
	float lin = segment.Linear.Factor;
	const float linStep = segment.Linear.FactorStep;
	size_t done = 0;
	while(done < n)
	{
		const size_t block = Min<size_t>(blockLimit, n - done);
		float rate = mRate, tremolo = 1.0f;
		if(blockVibrato)
		{
			// The LFO runs once per block, while the vibrato delay and entry use the true sample number.
			if(mSf2On)
			{
				// FluidSynth's vib LFO, verbatim: the value is queried at the
				// block's own absolute sample index (the 64-sample grid and the
				// frozen delay live inside Sf2TriangleLfo) and the vibLfoToPitch
				// gene is applied the way ct2hz_real applies it — an exact
				// 2^(value*cents/1200) read-rate factor.
				const float tri = mSf2Lfo.ValueAt(mElapsedSamples + unsigned(done));
				rate = mRate*Math::Pow(2.0f, tri*mSf2Cents/1200.0f);
			}
			else
			{
				const float vibNorm = mFreqOscillator.Next()*
					VibratoGateAt(mElapsedSamples + unsigned(done));
				rate = mRate*(1.0f + mVibratoValue*vibNorm);
				tremolo = 1.0f + mVibratoTremolo*vibNorm;
			}
		}
		auto blockLeft = dstLeft.Drop(done).Take(block);
		// Вес онсета на блок (hold → Weight, fade → линейно к 0).
		const float onsetWeight = OnsetWeightAt(mElapsedSamples + unsigned(done));
		const float onsetWeight2 = OnsetWeightAt2(mElapsedSamples + unsigned(done));
		// Позиции второй ступени онсета надо отснять ДО продвижения тела: ядра
		// этой ветки двигают ioOffsetL/R сами.
		const float onset2L = onsetWeight2 != 0.0f ? ioOffsetL : 0.0f;
		const float onset2R = onsetWeight2 != 0.0f ? ioOffsetR : 0.0f;
		if(stereo)
		{
			auto blockRight = dstRight.Drop(done).Take(block);
			if(onsetWeight != 0.0f)
				SynthKernels::AddConstantRateStereoWeighted(blockLeft, blockRight, src, ioOffsetL, ioOffsetR,
					rate, exp, expStep, lin, linStep,
					mLeftMultiplier*tremolo, mRightMultiplier*tremolo, channelDelta,
					onsetWeight, mOnsetSamples, mOnsetLength);
			else
				SynthKernels::AddConstantRateStereo(blockLeft, blockRight, src, ioOffsetL, ioOffsetR,
					rate, exp, expStep, lin, linStep,
					mLeftMultiplier*tremolo, mRightMultiplier*tremolo, channelDelta);
			if(onsetWeight2 != 0.0f)
			{
				const float amp2L = exp*lin*mLeftMultiplier*tremolo*onsetWeight2;
				const float amp2R = exp*lin*mRightMultiplier*tremolo*onsetWeight2;
				float o2L = onset2L, o2R = onset2R;
				SynthKernels::AddInterpolatedConstStereo(blockLeft, blockRight,
					Span<const float>(mOnsetSamples2, mOnsetLength2), o2L, o2R, rate,
					amp2L, amp2R, channelDelta);
			}
		}
		else
		{
			// Without vibrato tremolo is exactly 1.0f, so the mono path is unchanged (it has no panorama).
			SynthKernels::AddConstantRateMono(blockLeft, src, ioOffsetL, rate,
				exp, expStep, lin, linStep, tremolo);            if(onsetWeight != 0.0f)
			{
				// Постоянная скорость, без вибрато: траектория позиции известна
				// заранее (offset + k*rate с обёрткой по длине), поэтому онсет
				// читается в локальной копии с СТАРОГО offset — общую переменную
				// добавлять нельзя (ядро тела уже продвигает её: двойное
				// продвижение циклило фазу онсета — скрежет, Update 225).
				float onsetOffset = ioOffsetL;
				SynthKernels::AddInterpolatedConst(blockLeft, Span<const float>(mOnsetSamples, mOnsetLength),
					onsetOffset, rate, exp*lin*onsetWeight);
			}
			if(onsetWeight2 != 0.0f)
			{
				float onsetOffset2 = onset2L;
				SynthKernels::AddInterpolatedConst(blockLeft, Span<const float>(mOnsetSamples2, mOnsetLength2),
					onsetOffset2, rate, exp*lin*onsetWeight2);
			}
		}
		// Envelopes step over the block (exponentially for exp, linearly for lin); with one block per segment there is no step at all.
		if(block < n)
		{
			exp *= PowInt(expStep, int(block));
			lin += linStep*float(block);
		}
		done += block;
	}
	return done;
}

// Общий прямой рендер стерео/реверба (используется NoteSampler).
// Прибавляет результат в dst (буферы предварительно занулены).
// Возвращает число обработанных семплов.
size_t WaveTableSampler::renderDirect(Span<float> dstLeft, Span<float> dstRight)
{
	const size_t n = dstLeft.Length();
	if(n == 0 || mSampleFragmentLength == 0) return 0;

	const bool hasRight = !dstRight.Empty() && mRightMultiplier != 0;
	const bool preAttenuated = OwnExponentialAttenuatedDataArray();

	const float* frag = mSampleFragmentStart;
	const size_t len = mSampleFragmentLength;
	float leftOffset = mFragmentOffset;
	float rightOffset = float(mRightFragmentOffset);
	float noteFactor = mExpAtten.Factor;
	// Правый канал всегда читает ту же таблицу со сдвигом channelDelta
	// (mRightFragmentOffset = (mFragmentOffset + channelDelta) mod len).
	const size_t channelDelta = (mRightFragmentOffset + len - unsigned(mFragmentOffset)) % len;

	size_t processed = 0;
	while(processed < n)
	{
		if(mEnvelope.CurrentSegment.SamplesLeft == 0) break;
		const size_t chunk = Min(size_t(mEnvelope.CurrentSegment.SamplesLeft), n - processed);

		EnvelopeSegment seg(mEnvelope.CurrentSegment);
		if(!preAttenuated) seg.Exp *= mExpAtten;

		const float exp = seg.Exp.Factor;
		const float expStep = seg.Exp.FactorStep;
		const float lin = seg.Linear.Factor;
		const float linStep = seg.Linear.FactorStep;
		const Span<const float> src(frag, len);
		auto dstLeftChunk = dstLeft.Drop(processed).Take(chunk);
		const float onsetWeight = OnsetWeightAt(mElapsedSamples);
		const float onsetWeight2 = OnsetWeightAt2(mElapsedSamples);
		if(mHasVibrato && mVibratoBlock == 0)
		{
			// Per-segment frequency vibrato: the read rate is modulated by the oscillator (mRate*(1 + vib)), so constant-rate kernels do not apply. VibGate is the gradual vibrato onset (delay plus ramp).
			const float vibGate = VibratoGate();
			const float vibGateStep = VibratoGateStep();
			const auto vibBeforeBody = mFreqOscillator;
			// The onset reads the body position BEFORE the body kernel advances it
			// (AddOnsetVibrato* repeats the trajectory in a local copy). Passing the
			// post-advance value left a one-segment jump between the onset and the
			// body at every envelope segment boundary — audible clicks (Update 226).
			const float leftBeforeBody = leftOffset;
			const float rightBeforeBody = rightOffset;
			if(hasRight)
			{
				auto dstRightChunk = dstRight.Drop(processed).Take(chunk);
				SynthKernels::MultiplyAddVibratoStereo(dstLeftChunk, dstRightChunk, src,
					leftOffset, rightOffset, mRate, exp, expStep, lin, linStep,
					mLeftMultiplier, mRightMultiplier, mFreqOscillator,
					vibGate, vibGateStep, mVibratoValue, mVibratoTremolo);						if(onsetWeight != 0.0f)
						{
							// Ядро тела продвинуло осциллятор вибрато; онсет обязан читаться
							// с ТОЙ ЖЕ его фазой, иначе два слоя одной ноты дышат вразнобой
							// и вибрато удваивается по частоте. Позиции каналов онсет
							// берёт ДО продвижения телом (AddOnsetVibratoStereo сам
							// повторяет траекторию в локальной копии) — продвижение общей
							// позиции вторым ядром циклило фазу онсета (скрежет, Update 225).
							const auto vibAfterBody = mFreqOscillator;
							mFreqOscillator = vibBeforeBody;
							SynthKernels::AddOnsetVibratoStereo(dstLeftChunk, dstRightChunk,
								Span<const float>(mOnsetSamples, mOnsetLength),
								leftBeforeBody, rightBeforeBody, mRate, exp, expStep, lin, linStep,
								mLeftMultiplier, mRightMultiplier, mFreqOscillator, onsetWeight,
								vibGate, vibGateStep, mVibratoValue, mVibratoTremolo);
							if(onsetWeight2 != 0.0f)
							{
								const auto vibBeforeOnset2 = mFreqOscillator;
								mFreqOscillator = vibBeforeBody;
								SynthKernels::AddOnsetVibratoStereo(dstLeftChunk, dstRightChunk,
									Span<const float>(mOnsetSamples2, mOnsetLength2),
									leftBeforeBody, rightBeforeBody, mRate, exp, expStep, lin, linStep,
									mLeftMultiplier, mRightMultiplier, mFreqOscillator, onsetWeight2,
									vibGate, vibGateStep, mVibratoValue, mVibratoTremolo);
								mFreqOscillator = vibBeforeOnset2;
							}
							mFreqOscillator = vibAfterBody;
						}
			}
			else
			{
				SynthKernels::MultiplyAddVibrato(dstLeftChunk, src,
					leftOffset, mRate, exp, expStep,
					lin*mLeftMultiplier, linStep*mLeftMultiplier, mFreqOscillator,					vibGate, vibGateStep, mVibratoValue, mVibratoTremolo);
					if(onsetWeight != 0.0f)
					{
						const auto vibAfterBody = mFreqOscillator;
						mFreqOscillator = vibBeforeBody;							SynthKernels::AddOnsetVibrato(dstLeftChunk,
								Span<const float>(mOnsetSamples, mOnsetLength),
								leftBeforeBody, mRate, exp, expStep,
								lin*mLeftMultiplier, linStep*mLeftMultiplier, mFreqOscillator,
								onsetWeight, vibGate, vibGateStep, mVibratoValue, mVibratoTremolo);
						if(onsetWeight2 != 0.0f)
						{
							const auto vibBeforeOnset2 = mFreqOscillator;
							mFreqOscillator = vibBeforeBody;
							SynthKernels::AddOnsetVibrato(dstLeftChunk,
								Span<const float>(mOnsetSamples2, mOnsetLength2),
								leftBeforeBody, mRate, exp, expStep,
								lin*mLeftMultiplier, linStep*mLeftMultiplier, mFreqOscillator,
								onsetWeight2, vibGate, vibGateStep, mVibratoValue, mVibratoTremolo);
							mFreqOscillator = vibBeforeOnset2;
						}
						mFreqOscillator = vibAfterBody;
					}
			}
		}
		else
		{
			// Shared constant-rate path: the plain layer uses the whole segment as one block, block vibrato uses mVibratoBlock. The onset is added INSIDE this path (same offsets, same rate), so it must not be added again here.
			Span<float> rightChunk;
			if(hasRight) rightChunk = dstRight.Drop(processed).Take(chunk);
			renderConstantRate(dstLeftChunk, rightChunk, seg,
				leftOffset, rightOffset, channelDelta);
		}
		// Additive cores (see VoiceCorePartial): they carry the overtones the
		// table does not have, each with its own measured modulation.
		if(mVoiceCores.Count)
		{
			Span<float> coreRight;
			if(hasRight) coreRight = dstRight.Drop(processed).Take(chunk);
			addCorePartials(dstLeftChunk, coreRight, exp, expStep, lin, linStep);
		}

		mFragmentOffset = leftOffset;
		mRightFragmentOffset = unsigned(rightOffset);

#ifdef INTRA_PROBE_NAN
		{
			static int probeLines = 0;
			if(probeLines < 30)
			{
				float mx = 0;
				for(size_t pi = 0; pi < chunk; pi++)
				{
					float a = dstLeftChunk[pi]; if(a < 0) a = -a; if(a > mx) mx = a;
				}
				if(mx > 1e20f)
				{
					float srcMax = 0;
					for(size_t pi = 0; pi < len; pi++)
					{
						float a = frag[pi]; if(a < 0) a = -a; if(a > srcMax) srcMax = a;
					}
					fprintf(stderr, "[WT] mx=%.3e srcMax=%.3e exp=%.3e expStep=%.3e lin=%.3e linStep=%.3e rate=%.3f len=%zu chunk=%zu segSamplesLeft=%u off=%.3f\n",
						double(mx), double(srcMax), double(exp), double(expStep), double(lin), double(linStep),
						double(mRate), len, chunk, mEnvelope.CurrentSegment.SamplesLeft, double(leftOffset));
					probeLines++;
				}
			}
		}
#endif

		mEnvelope.CurrentSegment.Advance(chunk);
		if(mEnvelope.CurrentSegment.SamplesLeft == 0) mEnvelope.StartNextSegment();
		if(preAttenuated) mExpAtten.Factor = noteFactor;
		else mExpAtten.SkipSamples(chunk);
		mElapsedSamples += unsigned(chunk);

		processed += chunk;
	}
	return processed;
}

Span<float> WaveTableSampler::GenerateMono(Span<float> ioDst)
{
	const size_t n = ioDst.Length();
	if(n == 0 || mSampleFragmentLength == 0) return ioDst;

	// Онсет живёт в обоих путях рендера: вес считается один раз на чанк.
	// (в renderDirect — то же самое, см. там)

	const bool preAttenuated = OwnExponentialAttenuatedDataArray();
	const float* frag = mSampleFragmentStart;
	const size_t len = mSampleFragmentLength;
	float offset = mFragmentOffset;
	float noteFactor = mExpAtten.Factor;
	const float noteStep = mExpAtten.FactorStep;

	size_t processed = 0;
	while(processed < n)
	{
		if(mEnvelope.CurrentSegment.SamplesLeft == 0) break;
		const size_t chunk = Min(size_t(mEnvelope.CurrentSegment.SamplesLeft), n - processed);

		EnvelopeSegment seg(mEnvelope.CurrentSegment);
		if(!preAttenuated) seg.Exp *= mExpAtten;

		const float exp = seg.Exp.Factor;
		const float expStep = seg.Exp.FactorStep;
		const float lin = seg.Linear.Factor;
		const float linStep = seg.Linear.FactorStep;		const Span<const float> src(frag, len);
		const float onsetWeight = OnsetWeightAt(mElapsedSamples);
		const float onsetWeight2 = OnsetWeightAt2(mElapsedSamples);
		if(mHasVibrato && mVibratoBlock == 0)
		{
			const auto vibBeforeBody = mFreqOscillator;
			// Same as renderDirect: the onset needs the PRE-advance position.
			const float offsetBeforeBody = offset;
			SynthKernels::MultiplyAddVibrato(ioDst.Drop(processed).Take(chunk), src,
				offset, mRate, exp, expStep, lin, linStep, mFreqOscillator,
				VibratoGate(), VibratoGateStep(), mVibratoValue, mVibratoTremolo);
			if(onsetWeight != 0.0f)
			{
				// Как в renderDirect: осциллятор отматывается к фазе тела, позиция
				// берётся до продвижения — AddOnsetVibrato сам повторяет траекторию.
				const auto vibAfterBody = mFreqOscillator;
				mFreqOscillator = vibBeforeBody;
				SynthKernels::AddOnsetVibrato(ioDst.Drop(processed).Take(chunk),
					Span<const float>(mOnsetSamples, mOnsetLength),
					offsetBeforeBody, mRate, exp, expStep, lin, linStep, mFreqOscillator,
					onsetWeight, VibratoGate(), VibratoGateStep(),
					mVibratoValue, mVibratoTremolo);
				mFreqOscillator = vibAfterBody;
			}
			if(onsetWeight2 != 0.0f)
			{
				const auto vibBeforeOnset2 = mFreqOscillator;
				mFreqOscillator = vibBeforeBody;
				SynthKernels::AddOnsetVibrato(ioDst.Drop(processed).Take(chunk),
					Span<const float>(mOnsetSamples2, mOnsetLength2),
					offsetBeforeBody, mRate, exp, expStep, lin, linStep, mFreqOscillator,
					onsetWeight2, VibratoGate(), VibratoGateStep(),
					mVibratoValue, mVibratoTremolo);
				mFreqOscillator = vibBeforeOnset2;
			}
		}
		else
		{
			// As in renderDirect: the shared constant-rate path (it adds the onset itself).
			float unusedRightOffset = 0;
			renderConstantRate(ioDst.Drop(processed).Take(chunk), Span<float>(), seg,
				offset, unusedRightOffset, 0);
		}
		if(mVoiceCores.Count)
			addCorePartials(ioDst.Drop(processed).Take(chunk), Span<float>(), exp, expStep, lin, linStep);

		mFragmentOffset = offset;
		mEnvelope.CurrentSegment.Advance(chunk);
		if(mEnvelope.CurrentSegment.SamplesLeft == 0) mEnvelope.StartNextSegment();
		if(preAttenuated) mExpAtten.Factor = noteFactor;
		else mExpAtten.SkipSamples(chunk);
		mElapsedSamples += unsigned(chunk);

		processed += chunk;
	}
	if(processed < n) return ioDst.Drop(processed);
	return nullptr;
}

size_t WaveTableSampler::GenerateStereo(Span<float> dstLeft, Span<float> dstRight)
{
	return renderDirect(dstLeft, dstRight);
}

WaveTableSampler WaveTableInstrument::operator()(float freq, float volume, unsigned sampleRate) const
{
	auto& table = Tables->Get(freq, sampleRate);
	const float ratio = freq / float(sampleRate);
	const size_t level = table.NearestLevelForRatio(ratio);
	const auto samples = table.LevelSamples(level);
	//A register-dependent vibrato profile overrides the fixed instrument values.
	Vibrato vib;
	if(VibratoProfile) vib = VibratoProfile(freq);
	else
	{
		vib.Frequency = VibratoFrequency;
		vib.Value = VibratoValue;
		vib.Tremolo = VibratoTremolo;
		vib.Delay = VibratoDelay;
		vib.Ramp = VibratoRamp;
		vib.Jitter = VibratoJitter;
		vib.JitterFreq = VibratoJitterFrequency;
	}
	EnvelopeFactory envelopeFactory = Envelope;
	if(EnvelopeProfile) envelopeFactory = EnvelopeProfile(freq);
	// Note-dependent detune (see FreqScaleRefHz): cents*(refHz/freq), clamped in
	// the two lowest octaves. Multiplication by exactly 1 keeps the common path
	// bit-identical to the fixed FreqScale it replaces.
	const float detuneScale = FreqScaleRefHz > 0?
		Math::Pow(2.0f, FreqScaleCents*(FreqScaleRefHz/Math::Max(freq, FreqScaleRefHz*0.25f))/1200.0f): 1.0f;
	// Additive overtones of the preset (0 = none, the table has them all). The
	// table hands over the scale it used for its own lines (WaveTable::CoreScale),
	// so the cores match the table's level exactly instead of a fitted constant.
	VoiceCoreSet cores;
	if(Cores) BuildVoiceCores(Cores - 1, freq, sampleRate, table.CoreScale, cores);
	// Таблица онсета: строится/кешируется здесь же (OnsetTables живёт в
	// инструменте так же, как Tables), фазы линий заданы явно в профиле, а
	// нормировка и базис — те же, что у BuildWaveTableCore, поэтому обе
	// таблицы играют на одной громкости при равных амплитудах.
	const float* onsetSamples = nullptr;
	size_t onsetLength = 0;
	unsigned onsetHold = 0, onsetFade = 0;
	float onsetWeight = 0;
	// Вторая ступень (Update 236): «вспышка» обертонов после тёмного старта.
	const float* onsetSamples2 = nullptr;
	size_t onsetLength2 = 0;
	unsigned onsetDelay2 = 0, onsetRise2 = 0, onsetFade2 = 0;
	float onsetWeight2 = 0;
	if(OnsetProfile)
	{
		const OnsetTableDesc desc = OnsetProfile(freq);
		if(!desc.Harmonics.Empty() && desc.Duration > 0 && desc.Weight != 0)
		{
			WaveTable& onsetTable = OnsetTables.Get(freq, sampleRate, desc.Harmonics, 0);
			const size_t onsetLevel = onsetTable.NearestLevelForRatio(ratio);
			auto onsetFrag = onsetTable.LevelSamples(onsetLevel);
			onsetSamples = onsetFrag.Data();
			onsetLength = onsetFrag.Length();
			onsetHold = unsigned(desc.Duration*float(sampleRate));
			onsetFade = unsigned(desc.Crossfade*float(sampleRate));
			onsetWeight = desc.Weight;
		}
		if(!desc.Harmonics2.Empty() && desc.Rise2 > 0 && desc.Weight2 != 0)
		{
			// Вторая ступень — тот же кеш, другой слот (см. WaveTableCache::Get).
			WaveTable& onsetTable2 = OnsetTables.Get(freq, sampleRate, desc.Harmonics2, 1);
			const size_t onsetLevel2 = onsetTable2.NearestLevelForRatio(ratio);
			auto onsetFrag2 = onsetTable2.LevelSamples(onsetLevel2);
			onsetSamples2 = onsetFrag2.Data();
			onsetLength2 = onsetFrag2.Length();
			onsetDelay2 = unsigned(desc.Delay2*float(sampleRate));
			onsetRise2 = unsigned(desc.Rise2*float(sampleRate));
			onsetFade2 = unsigned(desc.Fade2*float(sampleRate));
			onsetWeight2 = desc.Weight2;
		}
	}
	return WaveTableSampler(samples, WaveTableSamplerParams::Make(
		ratio/table.LevelRatio(level)*FreqScale*detuneScale, Exp(-ExpCoeff/float(sampleRate)),
		volume*VolumeScale, (sampleRate >> 7) % samples.Length(), envelopeFactory(sampleRate),
		tableVibratoParams(vib, sampleRate, VibratoBlock), cores,
		onsetSamples, onsetLength, onsetHold, onsetFade, onsetWeight,
		onsetSamples2, onsetLength2, onsetDelay2, onsetRise2, onsetFade2, onsetWeight2));
}


WaveTable& WaveTableCache::Get(float freq, unsigned sampleRate) const
{
	float freqSampleRateRatio = freq/float(sampleRate);
	for(size_t i = 0; i < Tables.Length(); i++)
	{
		float rate = freqSampleRateRatio / Tables[i].BaseLevelRatio;
		if(!AllowMipmaps)
		{
			// Таблица квантует основной тон на сетку ДПФ (см. BuildWaveTable):
			// её BaseLevelRatio = bin/N, поэтому «своя» частота воспроизводится с
			// rate = 1 ± 0.5/bin. Жёсткий допуск 0.9999…1.0001 не попадал НИКОГДА
			// (расстройка до 0.5 %, т.е. до ±8.9 центов на C4) — каждый note-on
			// строил новую таблицу, кеш только рос. Допуск по bin это учитывает;
			// переиспользование соседнего bin безвредно: таблица гармонична, а
			// высоту выправляет та же скорость воспроизведения.
			const float bins = Math::Max(Tables[i].BaseLevelRatio*float(Tables[i].BaseLevelLength), 1.0f);
			const float tolerance = 0.9f/bins;
			if(rate > 1.0f - tolerance && rate < 1.0f + tolerance) return Tables[i];
			continue;
		}
		float r = rate;
		while(r >= 0.9999f)
		{
			if(0.9999f < r && r < 1.0001f) return Tables[i];
			r *= 0.5f;
		}
	}
	return Tables.AddLast(Generator(freq, sampleRate));
}

// Таблица онсета с ЯВНЫМИ фазами линий: амплитуды и фазы заданы профилем
// (обычный BuildWaveTableCore раскидывает фазы случайно, что убило бы
// кроссфейд — на этом умер блум, Update 223). Нормировка, сетка бинов и
// знак фазы повторяют BuildWaveTableCore построчно, отличаясь только фазой
// каждой линии, — поэтому форма профиля и уровень связаны с сустейном ровно
// так, как задано в OnsetTableDesc.Weight.
// Отличие от ConvertAmplitudesToSamplesUnnormalized только в источнике фаз;
// зато та же схема (полная длина, зеркало + IFFT) даёт тот же период
// (базовый уровень = BaseLevelLength) и ту же громкость на равных амплитудах.
// ВАЖНО: в прошлой попытке здесь стоял IFFT на N/2 без зеркала — половина
// фрагмента таблицы оставалась мусором от мнимой части, а знак фазы был
// зеркальным, и атака выходила втрое громче при потере тембра.
WaveTable& WaveTableCache::Get(float freq, unsigned sampleRate,
	Span<const float> harmonics, unsigned slot) const
{
	const float freqSampleRateRatio = freq/float(sampleRate);
	for(size_t i = 0; i < Tables.Length(); i++)
	{
		// Таблица того же профиля: у одной частоты их может быть несколько.
		if(Tables[i].ProfileSlot != slot) continue;
		const float rate = freqSampleRateRatio / Tables[i].BaseLevelRatio;
		const float bins = Math::Max(Tables[i].BaseLevelRatio*float(Tables[i].BaseLevelLength), 1.0f);
		const float tolerance = 0.9f/bins;
		if(rate > 1.0f - tolerance && rate < 1.0f + tolerance) return Tables[i];
	}
	const size_t tableSize = 16384;
	WaveTable tbl;
	tbl.BaseLevelLength = tableSize;
	tbl.Data.Reserve(tableSize*2);
	// Layout как в ConvertAmplitudesToSamplesUnnormalized(WaveTable&):
	// Data.Length() = N/2 (бины) → N*2 (бины + временный буфер) → N (семплы).
	tbl.Data.SetCount(tableSize/2);
	size_t baseBin = size_t(Math::Round(float(double(freq)*double(tableSize)/double(sampleRate))));
	if(baseBin < 1) baseBin = 1;
	tbl.BaseLevelRatio = float(baseBin)/float(tableSize);
	tbl.Data.SetCount(tableSize*2);
	FillZeros(tbl.Data);
	ConvertPhasedHarmonicsToSamples(tbl.Data.Take(tableSize), tbl.Data.Drop(tableSize),
		tbl.BaseLevelRatio, harmonics);
	// Мипмапов у онсета нет: он живёт только в атаке.
	tbl.Data.SetCount(tbl.BaseLevelLength);
	tbl.LevelCount = 1;
	tbl.ProfileSlot = slot;
	return Tables.AddLast(Move(tbl));
}
