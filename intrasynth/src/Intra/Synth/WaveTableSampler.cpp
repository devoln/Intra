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

static inline auto randGen(Span<const float> periodicWave, float rate, float volume)
{
	return Random::FastUniform<unsigned>(
		1436491347u ^ unsigned(periodicWave.Length()) ^ unsigned(rate*1537) ^ unsigned(volume * 349885300.0f)
	);
}

WaveTableSampler::WaveTableSampler(Span<const float> periodicWave, float rate,
	float attenuationPerSample, float volume, float vibratoDeltaPhase,
	float vibratoValue, const Envelope& envelope, size_t channelDeltaSamples,
	float vibratoDelaySamples, float vibratoRampSamples, float vibratoTremolo,
	float vibratoJitter, float vibratoJitterDeltaPhase,
	float vibratoHarm2, float vibratoHarm3, float vibratoHarm4, float vibratoHarm5):
	mSampleFragmentStart(periodicWave.Data()), mSampleFragmentLength(unsigned(periodicWave.Length())),
	mRate(rate), mLeftMultiplier(0.5f), mRightMultiplier(0.5f),
	mFreqOscillator(1.0f, 0, vibratoDeltaPhase, vibratoJitter, vibratoJitterDeltaPhase,
		vibratoHarm2, vibratoHarm3, vibratoHarm4, vibratoHarm5),
	mVibratoValue(vibratoValue), mVibratoTremolo(vibratoTremolo),
	mHasVibrato((vibratoValue != 0.0f || vibratoTremolo != 0.0f) && vibratoDeltaPhase != 0.0f), mEnvelope(envelope),
	mExpAtten(ExponentAttenuator::FromFactorAndStep(volume, attenuationPerSample)),
	mFragmentOffset(randGen(periodicWave, rate, volume)(mSampleFragmentLength)),
	mRightFragmentOffset((unsigned(mFragmentOffset) + channelDeltaSamples) % mSampleFragmentLength)
{
	if(mHasVibrato && vibratoRampSamples > 0)
	{
		mVibratoDelaySamples = vibratoDelaySamples;
		mVibratoRampSamples = vibratoRampSamples;
	}
	else if(mHasVibrato && vibratoDelaySamples > 0)
	{
		// Мгновенное включение после паузы (без плавного входа).
		mVibratoDelaySamples = vibratoDelaySamples;
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
		const bool constantAmp = (expStep == 1.0f && linStep == 0.0f);
		if(mHasVibrato)
		{
			// Частотное вибрато: скорость чтения модулируется осциллятором
			// (mRate*(1 + vib)); SIMD-ядра с постоянной скоростью неприменимы.
			// VibGate — плавное появление вибрато после атаки (Delay+Ramp).
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
		else if(hasRight)
		{
			auto dstRightChunk = dstRight.Drop(processed).Take(chunk);
			if(constantAmp)
			{
				// Sustain-сегмент: только интерполяция, без пошаговых огибающих.
				const float amp = exp*lin;
#if defined(__AVX2__) && !defined(INTRA_NO_SIMD_KERNELS)
				SynthKernels::AddInterpolatedConstStereo8(dstLeftChunk, dstRightChunk, src,
					leftOffset, rightOffset, mRate, amp*mLeftMultiplier, amp*mRightMultiplier,
					channelDelta);
#elif defined(__wasm_simd128__) && INTRA_SIMD_SUPPORT >= INTRA_SIMD_SSE2 && !defined(INTRA_NO_SIMD_KERNELS)
				SynthKernels::AddInterpolatedConstStereo4(dstLeftChunk, dstRightChunk, src,
					leftOffset, rightOffset, mRate, amp*mLeftMultiplier, amp*mRightMultiplier,
					channelDelta);
#else
				SynthKernels::AddInterpolatedConstStereo(dstLeftChunk, dstRightChunk, src,
					leftOffset, rightOffset, mRate, amp*mLeftMultiplier, amp*mRightMultiplier,
					channelDelta);
#endif
			}
			else
			{
				// Общие огибающие для обоих каналов, одна интерполяция на канал.
#if defined(__AVX2__) && !defined(INTRA_NO_SIMD_KERNELS)
				SynthKernels::MultiplyAddLinearInterpolatedStereo8(dstLeftChunk, dstRightChunk, src,
					leftOffset, rightOffset, mRate, exp, expStep, lin, linStep,
					mLeftMultiplier, mRightMultiplier, channelDelta);
#elif defined(__wasm_simd128__) && INTRA_SIMD_SUPPORT >= INTRA_SIMD_SSE2 && !defined(INTRA_NO_SIMD_KERNELS)
				SynthKernels::MultiplyAddLinearInterpolatedStereo4(dstLeftChunk, dstRightChunk, src,
					leftOffset, rightOffset, mRate, exp, expStep, lin, linStep,
					mLeftMultiplier, mRightMultiplier, channelDelta);
#else
				SynthKernels::MultiplyAddLinearInterpolatedStereo(dstLeftChunk, dstRightChunk, src,
					leftOffset, rightOffset, mRate, exp, expStep, lin, linStep,
					mLeftMultiplier, mRightMultiplier, channelDelta);
#endif
			}
		}
		else if(constantAmp)
		{
			SynthKernels::AddInterpolatedConst(dstLeftChunk, src, leftOffset, mRate,
				exp*lin*mLeftMultiplier);
		}
		else
		{
			float constWrap = 1.0f;
			SynthKernels::MultiplyAddLinearInterpolated(dstLeftChunk, src,
				leftOffset, mRate, exp, expStep, constWrap, 1.0f,
				lin*mLeftMultiplier, linStep*mLeftMultiplier);
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
		if(mHasVibrato)
		{
			SynthKernels::MultiplyAddVibrato(ioDst.Drop(processed).Take(chunk), src,
				offset, mRate, exp, expStep, lin, linStep, mFreqOscillator,
				VibratoGate(), VibratoGateStep(), mVibratoValue, mVibratoTremolo);
		}
		else if(expStep == 1.0f && linStep == 0.0f)
		{
			// Sustain-сегмент: только интерполяция, без пошаговых огибающих.
			SynthKernels::AddInterpolatedConst(ioDst.Drop(processed).Take(chunk), src,
				offset, mRate, exp*lin);
		}
		else
		{
			float constWrap = 1.0f;
			SynthKernels::MultiplyAddLinearInterpolated(ioDst.Drop(processed).Take(chunk), src,
				offset, mRate, exp, expStep, constWrap, 1.0f, lin, linStep);
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
	// Регистровый профиль вибрато (если задан) перекрывает фиксированные
	// VibratoFrequency/VibratoValue инструмента и добавляет задержку появления.
	Vibrato vib;
	if(VibratoProfile) vib = VibratoProfile(freq);
	else
	{
		vib.Frequency = VibratoFrequency;
		vib.Value = VibratoValue;
	}
	EnvelopeFactory envelopeFactory = Envelope;
	if(EnvelopeProfile) envelopeFactory = EnvelopeProfile(freq);
	return WaveTableSampler(samples, ratio/table.LevelRatio(level),
		Exp(-ExpCoeff/float(sampleRate)), volume*VolumeScale,
		2*float(PI)*Math::Max(vib.Frequency, 0.0f)/float(sampleRate), vib.Value,
		envelopeFactory(sampleRate), (sampleRate >> 7) % samples.Length(),
		vib.Delay*float(sampleRate), vib.Ramp*float(sampleRate), vib.Tremolo,
		vib.Jitter, 2*float(PI)*vib.JitterFreq/float(sampleRate),
		vib.Harm2, vib.Harm3, vib.Harm4, vib.Harm5);
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
