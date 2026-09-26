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
	if(vib.Value == 0.0f && vib.Tremolo == 0.0f && vib.Frequency == 0.0f) return null;
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
	vp.BlockSamples = blockSamples;
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
	mEnvelope(params.Envelope)
{
	const VibratoParams* vib = params.Vibrato != null? &params.Vibrato.Value(): nullptr;
	if(vib != nullptr)
	{
		// A block lfo is stepped once per block, so its phase step covers the block.
		mFreqOscillator = WobbleOscillator(1.0f, 0,
			vib->DeltaPhase*vibratoBlockScale(vib->BlockSamples), vib->Jitter,
			vib->JitterDeltaPhase*vibratoBlockScale(vib->BlockSamples),
			vib->Harm2, vib->Harm3, vib->Harm4, vib->Harm5);
		mVibratoValue = vib->Value;
		mVibratoTremolo = vib->Tremolo;
		mHasVibrato = (vib->Value != 0.0f || vib->Tremolo != 0.0f) && vib->DeltaPhase != 0.0f;
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
			const float vibNorm = mFreqOscillator.Next()*
				VibratoGateAt(mElapsedSamples + unsigned(done));
			rate = mRate*(1.0f + mVibratoValue*vibNorm);
			tremolo = 1.0f + mVibratoTremolo*vibNorm;
		}
		auto blockLeft = dstLeft.Drop(done).Take(block);
		if(stereo)
		{
			auto blockRight = dstRight.Drop(done).Take(block);
			SynthKernels::AddConstantRateStereo(blockLeft, blockRight, src, ioOffsetL, ioOffsetR,
				rate, exp, expStep, lin, linStep,
				mLeftMultiplier*tremolo, mRightMultiplier*tremolo, channelDelta);
		}
		else
		{
			// Without vibrato tremolo is exactly 1.0f, so the mono path is unchanged (it has no panorama).
			SynthKernels::AddConstantRateMono(blockLeft, src, ioOffsetL, rate,
				exp, expStep, lin, linStep, tremolo);
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
		if(mHasVibrato && mVibratoBlock == 0)
		{
			// Per-segment frequency vibrato: the read rate is modulated by the oscillator (mRate*(1 + vib)), so constant-rate kernels do not apply. VibGate is the gradual vibrato onset (delay plus ramp).
			const float vibGate = VibratoGate();
			const float vibGateStep = VibratoGateStep();
			if(hasRight)
			{
				auto dstRightChunk = dstRight.Drop(processed).Take(chunk);
				SynthKernels::MultiplyAddVibratoStereo(dstLeftChunk, dstRightChunk, src,
					leftOffset, rightOffset, mRate, exp, expStep, lin, linStep,
					mLeftMultiplier, mRightMultiplier, mFreqOscillator,
					vibGate, vibGateStep, mVibratoValue, mVibratoTremolo);
			}
			else
			{
				SynthKernels::MultiplyAddVibrato(dstLeftChunk, src,
					leftOffset, mRate, exp, expStep,
					lin*mLeftMultiplier, linStep*mLeftMultiplier, mFreqOscillator,
					vibGate, vibGateStep, mVibratoValue, mVibratoTremolo);
			}
		}
		else
		{
			// Shared constant-rate path: the plain layer uses the whole segment as one block, block vibrato uses mVibratoBlock.
			Span<float> rightChunk;
			if(hasRight) rightChunk = dstRight.Drop(processed).Take(chunk);
			renderConstantRate(dstLeftChunk, rightChunk, seg,
				leftOffset, rightOffset, channelDelta);
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
		if(mHasVibrato && mVibratoBlock == 0)
		{
			SynthKernels::MultiplyAddVibrato(ioDst.Drop(processed).Take(chunk), src,
				offset, mRate, exp, expStep, lin, linStep, mFreqOscillator,
				VibratoGate(), VibratoGateStep(), mVibratoValue, mVibratoTremolo);
		}
		else
		{
			// As in renderDirect: the shared constant-rate path.
			float unusedRightOffset = 0;
			renderConstantRate(ioDst.Drop(processed).Take(chunk), Span<float>(), seg,
				offset, unusedRightOffset, 0);
		}

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
		vib.Delay = VibratoDelay;
		vib.Ramp = VibratoRamp;
		vib.Jitter = VibratoJitter;
		vib.JitterFreq = VibratoJitterFrequency;
	}
	EnvelopeFactory envelopeFactory = Envelope;
	if(EnvelopeProfile) envelopeFactory = EnvelopeProfile(freq);
	return WaveTableSampler(samples, WaveTableSamplerParams::Make(
		ratio/table.LevelRatio(level)*FreqScale, Exp(-ExpCoeff/float(sampleRate)),
		volume*VolumeScale, (sampleRate >> 7) % samples.Length(), envelopeFactory(sampleRate),
		tableVibratoParams(vib, sampleRate, VibratoBlock)));
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
