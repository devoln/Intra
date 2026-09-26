#pragma once

// JS-faithful building blocks ported from devoln/web-midisynth (instruments.js,
// wavegen.js, utils.js). The wavetable generator, envelope mapping, noise
// sampler and the two time-varying one-pole filters reproduce the web
// algorithm so the C++ version sounds identical.

#include <Cpp/Warnings.h>

#include <Math/Math.h>
#include <Random/FastUniform.h>

#include <Container/Sequential/Array.h>

#include <Range/Mutation/Copy.h>
#include <Range/Mutation/Transform.h>

#include "WaveTable.h"
#include "WaveTableGeneration.h"

#ifdef INTRA_PROBE_NAN
#include <stdio.h>
#endif
#include "WaveTableSampler.h"
#include "Envelope.h"
#include "Types.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

// ---------------------------------------------------------------------------
// Wavetable generator (WaveTableGeneratorFromHarmonics + AddSineHarmonicGauss)
// ---------------------------------------------------------------------------

struct HarmonicDesc { float Amplitude; float FreqMultiplier; float Bandwidth; };
struct ResonanceDesc { float Frequency; float Width; float Amplitude; };
struct HarmonicSet
{
	Array<HarmonicDesc> Harmonics;
	Array<ResonanceDesc> Resonances;
	bool IsResonanceMultiplicative = false;

	// Gain applied after the harmonic profile is normalized. This keeps one
	// compact profile able to carry a measured per-zone loudness correction;
	// multiplying Harmonics before BuildWaveTable would be cancelled by the
	// normalization below.
	float VolumeScale = 1;
};

// JS AddSineHarmonic: index = round(ratio*N); dstAmpls[index] += amplitude*N.
inline void AddSineHarmonic(Span<float> dstAmpls, float ratio, float amplitude)
{
	const size_t N = dstAmpls.Length()*2;
	const size_t index = size_t(Math::Round(ratio*float(N)));
	if(index >= dstAmpls.Length()) return;
	dstAmpls[index] += amplitude*float(N);
}

// JS AddSineHarmonicGauss. Unlike the old AddSineHarmonicGaussianProfile this
// does not scale the gaussian by the harmonic index and integrates the exact
// JS amplitude chain (amplitude*N*sqrt(pi)/2 with erf integration).
noinline void AddSineHarmonicGauss(Span<float> dstAmpls, float ratio, float baseRatio, float amplitude, float bandwidthCents)
{
	const size_t N = dstAmpls.Length()*2;
	double bwi = (Math::Pow2(bandwidthCents/1200.0f - 1.0f) - 0.5)*baseRatio;
	if(bwi < 1e-10)
	{
		AddSineHarmonic(dstAmpls, ratio, amplitude);
		return;
	}

	double rw = -ratio/bwi;
	const double rdw = 1.0/(double(N)*bwi);

	double range = 2;
	if(rdw > 1) range = 3*rdw;
	if(-range > rw)
	{
		const double elementsToSkip = Math::Floor((-range - rw)/rdw);
		dstAmpls.PopFirstN(size_t(elementsToSkip));
		rw += elementsToSkip*rdw;
	}
	if(rw < range) dstAmpls = dstAmpls.Take(size_t(Math::Ceil((range - rw)/rdw)));

	const double A = amplitude*double(N)*(Math::SqrtPI/2);
	double erf = Math::Erf(rw);
	while(!dstAmpls.Empty())
	{
		rw += rdw;
		const double erfNext = Math::Erf(rw);
		dstAmpls.Next() += float(A*(erfNext - erf));
		erf = erfNext;
	}
}// Builds one wave table for a note of the given frequency from a single
// harmonic set. Exposed so instruments can pick the harmonic profile per
// register inside a custom WaveTableCache::Generator (e.g. Flute).
// Table build kernel from a ready harmonic row (defined below, before CreateWaveTables). Called from here and from the profile decoder in InstrumentLibrary.cpp, so the build body exists once.
noinline WaveTable BuildWaveTableCore(Span<const HarmonicDesc> harmonics, float volumeScale,
	size_t tableSize, float freq, unsigned sampleRate);

noinline WaveTable BuildWaveTable(const HarmonicSet& set, size_t tableSize, float freq, unsigned sampleRate)
{
	WaveTable tbl;
	tbl.BaseLevelLength = tableSize;
	tbl.Data.Reserve(tableSize*2);
	tbl.Data.SetCount(tableSize/2);
	// Основной тон квантуется на сетку ДПФ таблицы (fs/tableSize): таблица
	// периодична ровно на tableSize семплов, поэтому любая её линия обязана
	// стоять на этой сетке. Раньше baseRatio = freq/sampleRate, и округление
	// индекса round(baseRatio*n*N) давало систематическую расстройку (до
	// ±fs/2N = ±8.9 цента на C4 при N=16384) И ингармоничность — партиалы
	// уезжали от точных кратных (на клавише 72 h2 стоял на +4.5 цента от
	// 2·h1), что слушатель слышит как «расстройка большая». Теперь базис
	// таблицы — целое число bins, все партиалы стоят точно на сетке, а
	// точную высоту даёт скорость воспроизведения (rate = freq/(bin·fs/N)).
	size_t baseBin = size_t(Math::Round(float(double(freq)*double(tableSize)/double(sampleRate))));
	if(baseBin < 1) baseBin = 1;
	tbl.BaseLevelRatio = float(baseBin)/float(tableSize);
	const float baseRatio = tbl.BaseLevelRatio;

	// One shared kernel does the build. Without resonances the row goes in as is: the kernel computes amplSum exactly the same way (the sum of harmonic amplitudes below Nyquist).
	if(set.Resonances.Empty())
		return BuildWaveTableCore(set.Harmonics, set.VolumeScale, tableSize, freq, sampleRate);

	// With resonances the row is edited first, and the same kernel builds the table.
	Array<HarmonicDesc> finalHarmonics;
	for(const auto& harm: set.Harmonics)
	{
		const float ifreq = harm.FreqMultiplier*freq;
		if(ifreq > float(sampleRate)/2.0f) continue; // harmonics sorted by frequency

		float amplitude = harm.Amplitude;
		float res = 0;
		for(const auto& r: set.Resonances)
		{
			// A zero resonance width divides by zero (x = ifreq/0 -> inf, then 0*exp(-inf)/0 -> NaN). Such resonances are meaningless and are skipped, which also guards against NaN from corrupted data.
			if(r.Width <= 0.0f) continue;
			const float x = (ifreq - r.Frequency)/r.Width;
			res += r.Amplitude*Math::Exp(-0.5f*x*x)/(2.507f*r.Width);
		}
		if(set.IsResonanceMultiplicative) amplitude *= res;
		else amplitude += res;
		finalHarmonics.AddLast(HarmonicDesc{amplitude, harm.FreqMultiplier, harm.Bandwidth});
	}
	return BuildWaveTableCore(Span<const HarmonicDesc>(finalHarmonics.Data(), finalHarmonics.Length()),
		set.VolumeScale, tableSize, freq, sampleRate);
}

// Table build from a ready harmonic row: no resonances and no ownership. Needed where the profile is decoded on the stack (choir/voice region profiles in InstrumentLibrary.cpp): HarmsProfile used to hand out a HarmonicSet with two Arrays per anchor, pulling in destructors, allocations and inlined code. Basis quantisation and normalisation are exactly as in BuildWaveTable below, so the spectral shape and the level match it.
noinline WaveTable BuildWaveTableCore(Span<const HarmonicDesc> harmonics, float volumeScale,
	size_t tableSize, float freq, unsigned sampleRate)
{
	WaveTable tbl;
	tbl.BaseLevelLength = tableSize;
	tbl.Data.Reserve(tableSize*2);
	tbl.Data.SetCount(tableSize/2);
	size_t baseBin = size_t(Math::Round(float(double(freq)*double(tableSize)/double(sampleRate))));
	if(baseBin < 1) baseBin = 1;
	tbl.BaseLevelRatio = float(baseBin)/float(tableSize);
	const float baseRatio = tbl.BaseLevelRatio;

	float amplSum = 0;
	for(const auto& harm: harmonics)
	{
		const float ifreq = harm.FreqMultiplier*freq;
		if(ifreq > float(sampleRate)/2.0f) continue; // harmonics sorted by frequency
		amplSum += harm.Amplitude;
	}
	amplSum *= float(tableSize);
	for(const auto& h: harmonics)
	{
		if(h.FreqMultiplier*freq > float(sampleRate)/2.0f) continue;
		AddSineHarmonicGauss(tbl.Data.AsRange(), baseRatio*h.FreqMultiplier, baseRatio,
			h.Amplitude*volumeScale/amplSum, h.Bandwidth);
	}

	ConvertAmplitudesToSamplesUnnormalized(tbl, false);
	return tbl;
}

// Port of WaveTableGeneratorFromHarmonics. The resulting table is NOT peak
// normalized (JS uses InplaceInverseFFTNonNormalized); relative loudness is
// controlled by each instrument's Volume.
noinline WaveTableCache CreateWaveTables(const Array<HarmonicSet>& sets, size_t tableSize)
{
	WaveTableCache result;
	Array<HarmonicSet> setsCopy = sets;
	result.Generator = [setsCopy, tableSize](float freq, unsigned sampleRate)
	{
		WaveTable tbl;
		tbl.BaseLevelLength = tableSize;
		tbl.Data.Reserve(tableSize*2);
		tbl.Data.SetCount(tableSize/2);
		tbl.BaseLevelRatio = freq/float(sampleRate);
		const float baseRatio = tbl.BaseLevelRatio;

		// Несколько наборов гармоник суммируются в одну таблицу с общей
		// нормировкой (как в JS WaveTableGeneratorFromHarmonics).
		Array<HarmonicDesc> finalHarmonics;
		float amplSum = 0;
		for(const auto& set: setsCopy)
		{
			for(const auto& harm: set.Harmonics)
			{
				const float ifreq = harm.FreqMultiplier*freq;
				if(ifreq > float(sampleRate)/2.0f) continue; // harmonics sorted by frequency

				float amplitude = harm.Amplitude;
				if(!set.Resonances.Empty())
				{
					float res = 0;
					for(const auto& r: set.Resonances)
					{
						if(r.Width <= 0.0f) continue;
						const float x = (ifreq - r.Frequency)/r.Width;
						res += r.Amplitude*Math::Exp(-0.5f*x*x)/(2.507f*r.Width);
					}
					if(set.IsResonanceMultiplicative) amplitude *= res;
					else amplitude += res;
				}
				finalHarmonics.AddLast(HarmonicDesc{amplitude, harm.FreqMultiplier, harm.Bandwidth});
				amplSum += amplitude;
			}
		}
		amplSum *= float(tableSize);
		for(const auto& h: finalHarmonics)
			AddSineHarmonicGauss(tbl.Data.AsRange(), baseRatio*h.FreqMultiplier, baseRatio, h.Amplitude/amplSum, h.Bandwidth);

		ConvertAmplitudesToSamplesUnnormalized(tbl, false);
		return tbl;
	};
	result.AllowMipmaps = false;
	return result;
}

// ---------------------------------------------------------------------------
// Envelope (Envelope.Segments/Volume/Exp/ExponentialInterpolation)
// ---------------------------------------------------------------------------

struct EnvelopeDesc
{
	float Attack = 0;       // Segments[0]
	float Decay = 0;        // Segments[1] (0 for 1-2 segment envelopes)
	float Sustain = 1;      // Volume[2] plateau
	float Release = 0;      // Segments[last]
	float Exp = 0;          // exponential decay coefficient (separate gain in JS)
	bool Exponential = false;
	bool StartsAtFull = false; // Volume[0] == 1 -> no attack ramp
	// Attack delay (s), for notes that start with a consonant rather than a tone: in the bank's VoiceOohs the vowel only appears about 24 ms after note-on, and a broadband burst sounds before it (a separate attack layer, see kVoiceAttackDb). At zero it changes nothing for other instruments: the ADSR factory does not read the field, and MakeEnvelope without it builds the previous ADSR.
	float Delay = 0;
};

noinline EnvelopeFactory MakeEnvelope(const EnvelopeDesc& e)
{
	EnvelopeFactory f = EnvelopeFactory::ADSR(
		e.StartsAtFull ? 0.0f : e.Attack,
		e.Decay, e.Sustain, e.Release, e.Exponential);
	if(e.StartsAtFull) f.StartVolume = 1.0f;
	// Attack delay: the leading segment takes loudness from zero to 1/255 (-48 dB, where the exponential ramp starts) and the attack begins only after it. A linear ramp from zero would already be loud in the first milliseconds (at 53 it reached -6 dB by 20 ms while the bank was still -32 dB down), and the factory cannot build an exponential ramp from zero loudness (0*Pow(Inf, n) is NaN). The delay means a swell, so the attack is always exponential there, while the Exponential flag keeps controlling decay and release.
	if(e.Delay > 0)
	{
		f.Segments[EnvelopeFactory::N - 5] = {false, 1.0f/255.0f, e.Delay};
		f.Segments[EnvelopeFactory::N - 4].Exponential = true;
	}
	return f;
}


// ---------------------------------------------------------------------------
// Noise sampler (NoiseWaveform: GenNoisePeriod + LowpassFilter at note freq)
// ---------------------------------------------------------------------------
class NoiseSampler: public IGenericSampler
{
	Array<float> mTable;
	// Позиция чтения — ДРОБНАЯ (Update 59). С вибрато скорость чтения
	// модулируется тем же LFO, что у тела ноты, поэтому гребёнка «юбок» едет
	// по частоте вместе с гармониками. Без вибрато скорость ровно 1, дробная
	// часть равна нулю — выход побитово прежний.
	float mPos = 0;
	float mVolume;
	Envelope mEnv;
	bool mHasEnv = false;
	VibratoLfo mVibrato;

	// Один сэмпл таблицы с учётом вибрато (линейная интерполяция только при
	// дробной позиции).
	INTRA_FORCEINLINE float ReadNext()
	{
		const float len = float(mTable.Length());
		const float rate = mVibrato.Active ? 1.0f + mVibrato.Next() : 1.0f;
		mPos += rate;
		if(mPos >= len) mPos -= len;
		const size_t i0 = size_t(mPos);
		const size_t i1 = i0 + 1 < mTable.Length() ? i0 + 1 : 0;
		const float frac = mPos - float(i0);
		const float s = mTable[i0] + (mTable[i1] - mTable[i0])*frac;
		// Update 64: АМ-ветвь того же LFO (Tremolo). Без Tremolo множитель
		// ровно 1 (LastGated = 0 у неактивного LFO), выход прежний побитово.
		return s*(1.0f + mVibrato.Tremolo*mVibrato.LastGated);
	}

	// Envelope-aware mixing: advances mEnv and returns the number of processed
	// samples (less than requested once the release segment has finished).
	size_t generateWithEnvelope(Span<float> ioDst, Span<float> ioDstRight)
	{
		const size_t n = ioDstRight.Empty() ? ioDst.Length() : Math::Min(ioDst.Length(), ioDstRight.Length());
		size_t processed = 0;
		while(processed < n && mEnv.CurrentSegment.SamplesLeft > 0)
		{
			const size_t chunk = Min(size_t(mEnv.CurrentSegment.SamplesLeft), n - processed);
			float gain = mEnv.CurrentSegment.Volume;
			const float gainStep = mEnv.CurrentSegment.DU;
			const bool exponential = mEnv.CurrentSegment.Exponential;
			for(size_t i = 0; i < chunk; i++)
			{
				const float s = ReadNext()*mVolume*gain;
				ioDst[processed + i] += s;
				if(!ioDstRight.Empty()) ioDstRight[processed + i] += s;
				gain = exponential ? gain*gainStep : gain + gainStep;
			}
			mEnv.CurrentSegment.Advance(chunk);
			if(mEnv.CurrentSegment.SamplesLeft == 0) mEnv.StartNextSegment();
			processed += chunk;
		}
		return processed;
	}

public:
	NoiseSampler(float freq, float volume, unsigned sampleRate, size_t tableSize, float scale, uint seed,
		float cutoffMultiplier, bool hasEnvelope, const EnvelopeDesc& env, float hpMultiplier = 0.0f,
		unsigned lpPasses = 1, float combGain = 0.0f, unsigned combPasses = 1,
		const Vibrato& vibrato = {})
	{
		Random::FastUniform<float> noise(seed);
		mTable.SetCount(tableSize);
		for(size_t i = 0; i < tableSize; i++) mTable[i] = noise.SignedNext();

		// JS LowpassFilter(samples, freq/sampleRate): alpha = 2*pi*cutoffRatio.
		// cutoffMultiplier scales the cutoff above the note frequency so the
		// breath noise keeps an airy band instead of collapsing to a dull thump.
		const float alpha = Math::Min(2.0f*float(Math::PI)*freq*Math::Max(cutoffMultiplier, 0.001f)/float(sampleRate), 0.999f);
		float prev = 0;
		for(size_t i = 0; i < tableSize; i++)
		{
			const float s = mTable[i];
			prev = s*alpha + prev*(1.0f - alpha);
			mTable[i] = prev;
		}
		// Каскад однополюсных ФНЧ: lpPasses полюсов (Update 47). Банк режет
		// воздух круче двух полюсов (у пан-флейты выше 3.5 кГц — 20+ дБ/окт),
		// поэтому число полюсов стало параметром; 1 = как раньше без каскада.
		for(unsigned pass = 1; pass < Math::Max(lpPasses, 1u); pass++)
		{
			prev = 0;
			for(size_t i = 0; i < tableSize; i++)
			{
				const float s = mTable[i];
				prev = s*alpha + prev*(1.0f - alpha);
				mTable[i] = prev;
			}
		}
		// Second-order resonant high-pass: real flute breath is a JET-NOISE BAND
		// (energy concentrated roughly 500 Hz..3 kHz, peak ~1.5-2 kHz), not a
		// white-noise bed — the low air below ~0.5-0.75·f0 must be cut. Measured
		// 2026-09-09 (bank C5/C6 inter-harmonic noise is 15-25 dB quieter below
		// 500 Hz than at the 1.5-2 kHz peak; our LP-only bed was flat down to
		// 120 Hz — the "шипит белым шумом" complaint). hpMultiplier = high-pass
		// corner as a multiple of the note frequency (0 = disabled, legacy flat
		// bed). RBJ biquad HPF, Q = 0.75.
		if(hpMultiplier > 0.0f)
		{
			const float fc = Math::Clamp(freq*hpMultiplier, 80.0f, 1200.0f);
			const float w0 = 2.0f*float(Math::PI)*fc/float(sampleRate);
			const float cosw = Math::Cos(w0), sinw = Math::Sin(w0);
			const float q = 0.75f;
			const float alphaHp = sinw/(2.0f*q);
			const float a0 = 1.0f + alphaHp;
			const float b0 = (1.0f + cosw)*0.5f/a0;
			const float b1 = -(1.0f + cosw)/a0;
			const float b2 = (1.0f + cosw)*0.5f/a0;
			const float a1 = -2.0f*cosw/a0;
			const float a2 = (1.0f - alphaHp)/a0;
			float x1 = 0, x2 = 0, y1 = 0, y2 = 0;
			for(size_t i = 0; i < tableSize; i++)
			{
				const float x = mTable[i];
				const float y = b0*x + b1*x1 + b2*x2 - a1*y1 - a2*y2;
				x2 = x1; x1 = x;
				y2 = y1; y1 = y;
				mTable[i] = y;
			}
		}
		// Tone-correlated breath (2026-09-10): feedback comb at the note period
		// (delay = one period) turns the uncorrelated jet band into partial "skirts" —
		// energy concentrated AT the harmonics, exactly how the bank's breath reads
		// ("дыхание, привязанное к основной гармонике"). Uncoupled white noise at
		// -40..-50 dB reads as hiss ("шипит"), correlated air does not. combGain 0 =
		// legacy flat noise. Applied in place: t[i-D] is already the comb output.
		// Update 56: combPasses — гребёнка применяется НЕСКОЛЬКО раз (каскад).
		// Один проход g=0.85 даёт контраст «пик/пол» всего 1/(1−g)·(1+g) = 18.9 дБ,
		// а у банка между гармониками местами −80…−100 дБ при юбках −40…−50,
		// то есть 30-40 дБ: наш ровный пол читался как шипение (владелец:
		// «чувствуется что-то типа шума»). Каскад из двух проходов даёт +18.9 дБ
		// контраста при том же уровне юбок.
		if(combGain > 0.0f)
		{
			const float g = Math::Min(combGain, 0.95f);
			// Update 56: ДРОБНАЯ задержка. sr/f0 почти никогда не целое (на C5
			// 84.28 сэмпла), а округление до 84 сдвигает гребёнку на 5.6 цента —
			// её пики уезжают с гармоник тем сильнее, чем выше номер гармоники
			// (h4: 2100 вместо 2093 Гц, то есть 7 Гц при полосе пика 3 Гц).
			// Замер .scratch/skirt-time.mjs читал из-за этого юбки h2..h4 на
			// 10-16 дБ тише, чем они есть. Линейная интерполяция линии задержки
			// возвращает гребёнку точно на гармоники (как у семпла банка, где
			// юбки привязаны к своей ноте).
			const float delayF = float(sampleRate)/Math::Max(freq, 20.0f);
			const int D = Math::Max(1, int(delayF));
			const float fr = Math::Clamp(delayF - float(D), 0.0f, 1.0f);
			for(unsigned pass = 0; pass < Math::Max(combPasses, 1u); pass++)
				for(size_t i = 0; i < tableSize; i++)
				{
					const float x = mTable[i];
					const float d0 = i >= size_t(D) ? mTable[i - D] : 0.0f;
					const float d1 = i >= size_t(D + 1) ? mTable[i - D - 1] : 0.0f;
					mTable[i] = x + g*(d0 + (d1 - d0)*fr);
				}
		}
		// Вибрато дыхания — то же, что у тела ноты (см. VibratoLfo): юбки едут
		// вместе с гармониками, а не бьются с ними («насос»).
		mVibrato.Init(vibrato, sampleRate);
		mVolume = volume*scale;
		mHasEnv = hasEnvelope;
		if(hasEnvelope)
		{
			// Starts at 0 and swells to 1 over Attack (the chiff burst), then
			// decays to the Sustain air bed and holds; NoteRelease starts the
			// release segment. Only meaningful with a non-zero Attack.
			mEnv = EnvelopeFactory::ADSR(env.Attack, env.Decay, env.Sustain, env.Release, env.Exponential)(sampleRate);
		}
	}

	void MultiplyVolume(float volumeMultiplier) override {mVolume *= volumeMultiplier;}

	size_t GenerateMono(Span<float> ioDst) override
	{
		if(mHasEnv) return generateWithEnvelope(ioDst, nullptr);
		for(size_t i = 0; i < ioDst.Length(); i++)
			ioDst[i] += ReadNext()*mVolume;
		return ioDst.Length();
	}

	size_t GenerateStereo(Span<float> ioDst, Span<float> ioDstRight) override
	{
		// Честный стерео-рендер (Update 18): один и тот же шум в оба канала,
		// по 0.5 на канал — суммарная мощность равна старому моно-уровню.
		// Раньше здесь была копия сигнала в оба канала (та же амплитуда, что
		// в моно), из-за чего слой был на +6 дБ громче на канал, чем у нот без
		// модификаторов (их путь суммировал по 0.5).
		if(mHasEnv) return generateWithEnvelope(ioDst, ioDstRight);
		const size_t n = Math::Min(ioDst.Length(), ioDstRight.Length());
		for(size_t i = 0; i < n; i++)
		{
			const float s = ReadNext()*mVolume*0.5f;
			ioDst[i] += s;
			ioDstRight[i] += s;
		}
		return n;
	}

	void NoteRelease() override
	{
		if(mHasEnv) mEnv.StartLastSegment();
	}
};

// ---------------------------------------------------------------------------
// BloomSampler — эволюция спектра на атаке (записанный онсет банка Titanic).

// Плоская wavetable-таблица не может менять тембр во времени, а в банке
// атака живёт: яркая вспышка h4/h5 (+10…+18 дБ, спадает к ~0.5-0.6 с) и
// «всплытие» h2/h3 из тёмного начала в сустейн (замеры .scratch/
// bloom-measure.mjs, окна 0-0.1/0.1-0.4/0.4-0.8/0.8-1.8 с от note-on).
// Механизм — как оверлеи в AdditiveSampler: фаза-выровненная сумма
// гармоник × огибающая. Гармоники вспышки (положительные амплитуды) и
// гармоники «всплытия» (отрицательные — вычитают из тела, пока тембр ещё
// тёмный) живут в одном голосе, но с разными огибающими:
//   gate(t)     — линейный 0→1 за toneRise: блум едет на раздуве тона, поэтому
//                 в первые миллисекунды (тон ещё тихий) ничего не добавляет и
//                 отрицательные гармоники не могут «перевернуть» фазу тела;
//   flash(t)    — 0→1 за flashRise, затем экспоненциальный спад τ=flashTau:
//                 яркая вспышка атаки;
//   swell(t)    — 1→0 экспоненциально τ=swellTau: тёмное начало h2/h3
//                 рассасывается к сустейну.
// Итог: тело(t) = sustain_таблица + Σ flash_i·gate·flash + Σ swell_j·gate·swell,
// т.е. спектр плавно переходит атака(яркая/чистая) → сустейн(темнее).
class BloomSampler: public IGenericSampler
{
	// Состояние SineRange-рекурсии: s_{n+1} = k·s_n − s_{n−1}, амплитуда уже
	// внутри s1/s2 (как в AdditiveSampler). Первые mFlashCount партиал —
	// вспышка, остальные — «всплытие».
	Array<float> mS1, mS2, mK, mKSlope, mAmp, mHarm;
	size_t mCount = 0;
	size_t mFlashCount = 0;
	// Вибрато — то же, что у тела ноты (см. VibratoLfo): блум не должен
	// стоять на месте, пока тон вибрирует.
	VibratoLfo mVibrato;
	float mBaseStep;   // 2π·f0/sr — шаг фазы первой гармоники
	float mVolume;     // volume × scale (0.5/0.5 на канал добавляет стерео-путь)
	// Огибающие (см. комментарий класса).
	float mGate = 0;
	float mGateStep = 0;
	float mFlash = 0;
	float mFlashRiseStep = 0;
	float mFlashDecay = 0;
	float mSwell = 1;
	float mSwellDecay = 0;
	bool mFlashPeaked = false;
	// NoteRelease: быстрый фейд обеих огибающих (τ≈8 мс), чтобы стаккато не
	// щёлкало по ещё живой вспышке.
	float mReleaseStep = 0;
	bool mReleased = false;

	void AddPartial(int harmonic, float amp)
	{
		mHarm.AddLast(float(harmonic));
		mAmp.AddLast(amp);
		const float step = mBaseStep*float(harmonic);
		mK.AddLast(2.0f*Math::Cos(step));
		// k = 2cos(step): при вибрато step → step(1+δ), поэтому
		// k(δ) ≈ 2cos(step) − 2sin(step)·step·δ (δ ≤ 0.005, приближение
		// первого порядка держит расстройку на уровне сотых цента).
		mKSlope.AddLast(2.0f*Math::Sin(step)*step);
		mS2.AddLast(0.0f);
		mS1.AddLast(amp*Math::Sin(step));
		mCount++;
	}

public:
	BloomSampler(float freq, float volume, unsigned sampleRate,
		Span<const float> flashAmps, size_t flashMaxPartial,
		Span<const float> swellAmps, size_t swellMaxPartial,
		float flashRiseSeconds, float flashTau, float swellTau,
		float toneRiseSeconds, float scale, const Vibrato& vibrato = {})
	{
		mBaseStep = 2.0f*float(Math::PI)*freq/float(sampleRate);
		// Блум — часть той же ноты, поэтому вибрато у него то же, что у тела:
		// иначе статичный овершут h1 сидит на месте и вычитает вибрато тона
		// (владелец: «не слышу вибрато»).
		mVibrato.Init(vibrato, sampleRate);
		mVolume = volume*scale;
		for(size_t k = 1; k <= flashMaxPartial && k <= flashAmps.Length(); k++)
		{
			const float a = flashAmps[k - 1];
			if(Abs(a) > 1e-5f) AddPartial(int(k), a);
		}
		mFlashCount = mCount;
		for(size_t k = 1; k <= swellMaxPartial && k <= swellAmps.Length(); k++)
		{
			const float a = swellAmps[k - 1];
			if(Abs(a) > 1e-5f) AddPartial(int(k), a);
		}
		mFlashRiseStep = flashRiseSeconds > 0 ? 1.0f/(flashRiseSeconds*float(sampleRate)) : 0.0f;
		mFlashDecay = Math::Exp(-1.0f/(Math::Max(flashTau, 0.001f)*float(sampleRate)));
		mSwellDecay = Math::Exp(-1.0f/(Math::Max(swellTau, 0.001f)*float(sampleRate)));
		mGateStep = toneRiseSeconds > 0 ? 1.0f/(toneRiseSeconds*float(sampleRate)) : 1.0f;
		mReleaseStep = Math::Exp(-1.0f/(0.008f*float(sampleRate)));
	}

	void MultiplyVolume(float volumeMultiplier) override {mVolume *= volumeMultiplier;}

	size_t GenerateMono(Span<float> ioDst) override
	{
		const size_t n = ioDst.Length();
		const size_t count = mCount;
		float* s1 = mS1.Data();
		float* s2 = mS2.Data();
		const float* k = mK.Data();
		const float* kSlope = mKSlope.Data();
		const float* amp = mAmp.Data();
		const size_t flashCount = mFlashCount;
		const float vol = mVolume;
		float gate = mGate, flash = mFlash, swell = mSwell;
		for(size_t i = 0; i < n; i++)
		{
			if(gate < 1.0f)
			{
				gate += mGateStep;
				if(gate > 1.0f) gate = 1.0f;
			}			if(mReleased) { flash *= mReleaseStep; swell *= mReleaseStep; }
			// Одноразовый подъём вспышки 0→1: после пика она ТОЛЬКО спадает
			// (× mFlashDecay). Раньше после спада ниже 1.0 условие flash<1 снова
			// поднимало её линейно, и подъём (2.27e-4/семпл) выигрывал у спада
			// (1.74e-4/семпл у flash≈1) — вспышка висела на ~1.0 всю ноту,
			// давая устойчивый пересвет h4 на +10…13 дБ вместо короткого блума.
			if(!mFlashPeaked)
			{
				flash += mFlashRiseStep;
				if(flash >= 1.0f) { flash = 1.0f; mFlashPeaked = true; }
			}
			else flash *= mFlashDecay;
			swell *= mSwellDecay;
			const float vib = mVibrato.Active ? mVibrato.Next() : 0.0f;
			float s = 0;
			for(size_t p = 0; p < flashCount; p++) s += amp[p]*s1[p]*(gate*flash);
			for(size_t p = flashCount; p < count; p++) s += amp[p]*s1[p]*(gate*swell);
			if(vib != 0.0f)
			{
				for(size_t p = 0; p < count; p++)
				{
					const float nxt = (k[p] - kSlope[p]*vib)*s1[p] - s2[p];
					s2[p] = s1[p];
					s1[p] = nxt;
				}
			}
			else for(size_t p = 0; p < count; p++)
			{
				const float nxt = k[p]*s1[p] - s2[p];
				s2[p] = s1[p];
				s1[p] = nxt;
			}
			ioDst[i] += s*vol;
		}
		mGate = gate; mFlash = flash; mSwell = swell;
		return mFlashPeaked && flash < 1e-4f && swell < 1e-4f ? 0 : n;
	}



	size_t GenerateStereo(Span<float> ioDstLeft, Span<float> ioDstRight) override
	{
		const size_t n = Math::Min(ioDstLeft.Length(), ioDstRight.Length());
		const size_t count = mCount;
		float* s1 = mS1.Data();
		float* s2 = mS2.Data();
		const float* k = mK.Data();
		const float* kSlope = mKSlope.Data();
		const float* amp = mAmp.Data();
		const size_t flashCount = mFlashCount;
		const float vol = mVolume*0.5f;
		float gate = mGate, flash = mFlash, swell = mSwell;
		for(size_t i = 0; i < n; i++)
		{
			if(gate < 1.0f)
			{
				gate += mGateStep;
				if(gate > 1.0f) gate = 1.0f;
			}
			if(mReleased) { flash *= mReleaseStep; swell *= mReleaseStep; }
			// Односторонняя вспышка: подъём 0→1 один раз, дальше только спад
			// (см. комментарий в GenerateMono — без этого вспышка не гаснет).
			if(!mFlashPeaked)
			{
				flash += mFlashRiseStep;
				if(flash >= 1.0f) { flash = 1.0f; mFlashPeaked = true; }
			}
			else flash *= mFlashDecay;
			swell *= mSwellDecay;
			const float vib = mVibrato.Active ? mVibrato.Next() : 0.0f;
			float s = 0;
			for(size_t p = 0; p < flashCount; p++) s += amp[p]*s1[p]*(gate*flash);
			for(size_t p = flashCount; p < count; p++) s += amp[p]*s1[p]*(gate*swell);
			if(vib != 0.0f)
			{
				for(size_t p = 0; p < count; p++)
				{
					const float nxt = (k[p] - kSlope[p]*vib)*s1[p] - s2[p];
					s2[p] = s1[p];
					s1[p] = nxt;
				}
			}
			else for(size_t p = 0; p < count; p++)
			{
				const float nxt = k[p]*s1[p] - s2[p];
				s2[p] = s1[p];
				s1[p] = nxt;
			}
			const float out = s*vol;
			ioDstLeft[i] += out;
			ioDstRight[i] += out;
		}
		mGate = gate; mFlash = flash; mSwell = swell;
		return mFlashPeaked && flash < 1e-4f && swell < 1e-4f ? 0 : n;
	}

	void MultiplyPitch(float freqMultiplier) override
	{
		mBaseStep *= freqMultiplier;
		for(size_t p = 0; p < mCount; p++)
		{
			const float step = mBaseStep*mHarm[p];
			mK[p] = 2.0f*Math::Cos(step);
			mKSlope[p] = 2.0f*Math::Sin(step)*step;
		}
	}

	void NoteRelease() override { mReleased = true; }
};

struct NoiseInstrument
{
	size_t TableSize = 32768;
	float VolumeScale = 1;
	uint Seed = 157898685;
	float CutoffMultiplier = 1;
	EnvelopeDesc Envelope;
	bool HasEnvelope = false;

	GenericSamplerRef operator()(float freq, float volume, unsigned sampleRate) const
	{return new NoiseSampler(freq, volume, sampleRate, TableSize, VolumeScale, Seed, CutoffMultiplier, HasEnvelope, Envelope);}
};

// ---------------------------------------------------------------------------
// Time-varying one-pole filters (ExpExp Filter and CutoffFreq approximation)
// out = in*mu + prev*(1-mu), with mu(t) piecewise linear.
// ---------------------------------------------------------------------------

class TimeVaryingLowpass
{
	float mPrev = 0;
	float mMu;
	float mDMu = 0;
	size_t mIndex = 0;
	size_t mStepLeft = 0;
	size_t mStep;
	CSpan<float> mDeltas;

public:
	TimeVaryingLowpass(CSpan<float> deltas, float startMu, size_t step):
		mMu(startMu), mStep(step), mDeltas(deltas) {}

	void operator()(Span<float> dst)
	{
		for(float& out: dst)
		{
			if(mStepLeft == 0)
			{
				mStepLeft = mStep;
				mIndex++;
				float dmu = 0;
				if(mIndex < mDeltas.Length())
				{
					dmu = mDeltas[mIndex];
					if(dmu < -mMu/float(mStep)) dmu = -mMu/float(mStep);
				}
				mDMu = dmu;
			}
			const float s = mPrev*(1.0f - mMu) + out*mMu;
			mPrev = s;
			out = s;
			mMu += mDMu;
			mStepLeft--;
		}
	}
};

// ExpExp (Envelope.ExpExpK / ExpExpBase): smoothing coefficient table that
// makes the sound mellow over time. Port of GenSmoothCoeffTableExpExp + Filter.
inline Array<float> GenExpExpDeltas(float base, float k, size_t len = 1024, size_t step = 256)
{
	Array<float> res;
	res.AddLast(1.0f);
	float amul = Math::Pow(1.0f - base, float(step));
	float a = k;
	const float emk = Math::Exp(-k);
	float prevmu = 1.0f;
	for(size_t i = 1; i < len; i++)
	{
		a *= amul;
		const float mu = Math::Exp(a)*emk;
		res.AddLast((mu - prevmu)/float(step));
		prevmu = mu;
	}
	return res;
}

struct ExpExpModifierFactory
{
	Array<float> Deltas;
	size_t Step = 256;

	ExpExpModifierFactory(decltype(nullptr)=nullptr) {}
	ExpExpModifierFactory(float base, float k): Deltas(GenExpExpDeltas(base, k)) {}

	GenericModifier operator()(float, float, unsigned) const
	{
		return GenericModifier(TimeVaryingLowpass(Deltas.AsConstRange(), 1.0f, Step));
	}

	INTRA_FORCEINLINE explicit operator bool() const {return !Deltas.Empty();}
};

// CutoffFreq: lowpass whose cutoff ramps with the envelope. JS uses a biquad;
// we use a stable one-pole with alpha = 1 - exp(-2*pi*cutoff/sr). The release
// sweep is not modelled (the modifier has no note-off hook).
class CutoffFilter
{
	float mPrev = 0;
	float mAlpha = 0;
	float mDAlpha = 0;
	size_t mSamplesLeft = 0;
	float mEndAlphas[4];
	float mDurations[4];
	size_t mSegCount = 0;
	size_t mSegIndex = 0;

public:
	CutoffFilter(float startAlpha, float attackSamples, float a1, float decaySamples, float a2)
	{
		mAlpha = startAlpha;
		mEndAlphas[0] = a1; mDurations[0] = attackSamples;
		mEndAlphas[1] = a2; mDurations[1] = decaySamples;
		mEndAlphas[2] = a2; mDurations[2] = 1; // sustain hold
		mSegCount = attackSamples > 0 ? 3 : 2;
	}

	void operator()(Span<float> dst)
	{
		for(float& out: dst)
		{
			if(mSamplesLeft == 0)
			{
				if(mSegIndex >= mSegCount)
				{
					mDAlpha = 0; // hold cutoff after the last segment
				}
				else
				{
					const float dur = mDurations[mSegIndex];
					const float end = mEndAlphas[mSegIndex];
					mSegIndex++;
					mSamplesLeft = size_t(dur);
					mDAlpha = dur > 0 ? (end - mAlpha)/dur : 0;
				}
			}
			const float s = out*mAlpha + mPrev*(1.0f - mAlpha);
#ifdef INTRA_PROBE_NAN
			if(!(s > -1e12f && s < 1e12f))
			{
				static int probeLines = 0;
				if(probeLines < 10)
				{
					fprintf(stderr, "[CUTOFF] s=%.3e in=%.3e prev=%.3e alpha=%.3e dAlpha=%.3e segIdx=%zu segLeft=%zu\n",
						double(s), double(out), double(mPrev), double(mAlpha), double(mDAlpha), mSegIndex, mSamplesLeft);
					probeLines++;
				}
			}
#endif
			mPrev = s;
			out = s;
			mAlpha += mDAlpha;
			if(mSamplesLeft > 0) mSamplesLeft--;
		}
	}
};

struct CutoffFactory
{
	float Cutoffs[4] = {20000, 20000, 20000, 20000};
	EnvelopeDesc Envelope;

	CutoffFactory(decltype(nullptr)=nullptr) {}
	CutoffFactory(float c0, float c1, float c2, float c3, const EnvelopeDesc& env):
		Cutoffs{c0, c1, c2, c3}, Envelope(env) {}

	GenericModifier operator()(float, float, unsigned sampleRate) const
	{
		auto alpha = [sampleRate](float cutoff) -> float
		{
			const float f = Math::Max(cutoff, 1.0f);
			return 1.0f - Math::Exp(-2.0f*float(Math::PI)*f/float(sampleRate));
		};
		const float attackSamples = Math::Max(0.0f, Envelope.Attack*float(sampleRate));
		const float decaySamples = Math::Max(0.0f, Envelope.Decay*float(sampleRate));
		return GenericModifier(CutoffFilter(
			alpha(Cutoffs[0]), attackSamples, alpha(Cutoffs[1]), decaySamples, alpha(Cutoffs[2])));
	}

	INTRA_FORCEINLINE explicit operator bool() const {return false;}
};

INTRA_WARNING_POP
