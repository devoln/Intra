#pragma once

#include <Math/Math.h>
#include "Intra/Simd/Simd.h"

#include "Intra/Range/Span.h"
#include "Utils/FixedArray.h"
#include "Types.h"
#include "PianoRegions.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS


/// Аддитивный семплер: сумма N независимых SineRange-осцилляторов (рекурсия
/// s2 = 2·cos(dphi)·s1 − s0, по 1 FMA на партиал на семпл). Партиалы берутся
/// из таблицы PianoRegions.h — реального пианино (Clavinova Grand), измерены
/// из семплов: амплитуда/фаза (окно атаки), частота (измеренный ratio —
/// растяжка/негармоничность), затухание (2-скоростное: быстрый начальный
/// спад λ1 + хвост λ2, по факту семпла).
///
/// Поверх таблицы — три физических механизма настоящего рояля:
///   - Атака per-partial: фундаментал нарастает медленно (τ≈AttackT), верхние
///     обертоны быстрее (τ≈AttackT/k) — чёткий ударный транзиент;
///   - Унисон: 1-3 «струны» на ноту с расстройкой DetuneCents (биения);
///   - Velocity→яркость: громче играешь — ярче тембр (слои SF2).
///
/// Горячий цикл (шаблон ниже) инстанцируется в AdditiveSampler.cpp, который
/// в size-сборках компилируется -O3 без LTO. По 4 партиалы за раз через
/// SIMD128 (wasm) / SSE2 (x86), скалярный фолбэк для сборок без SIMD.
/// Партиалы обрабатываются блоками: состояние осцилляторов живёт в регистрах
/// на весь блок, аккумуляторы — 4 лейна на сэмпл, горизонтальная свёртка —
/// один раз в конце блока.
class AdditiveSampler: public IGenericSampler
{
	// Состояние рекурсии SineRange: mS1[p], mS2[p] — два последних сэмпла
	// (амплитуда уже внутри: s1_0 = a·sin(phi)), mK[p] = 2·cos(dphi_p).
	FixedArray<float> mS1, mS2, mK;
	// mAmp[p] — множитель огибающей: 4-скоростное затухание по факту семпла
	// (быстрый начальный спад λ1 → средний λ2 → поздний средний λ3 → медленный
	// хвост λ4; затухание реальных семплов гнётся непрерывно — 3-скоростная
	// модель переключалась на хвост уже в 0.9с, а семпл держит быстрый спад
	// до ~1.8с, и хвост «пересиживался» на 10-25 дБ). Затухание начинается
	// после атаки с DecayOnset. mDecay[p] — активный шаг (1.0 до onset),
	// mDecay1/2/3/4 — шаги сегментов; переключение выполняется за O(count)
	// на точной границе сэмпла.
	FixedArray<float> mAmp, mDecay, mDecay1, mDecay2, mDecay3, mDecay4;
// mDecayRelease[p] — шаг затухания при отпускании клавиши (демпфер):
// быстрее обычного, с зависимостью от k (высокие гармоники гаснут
// быстрее, как в реальном пианино). Применяется через dec[p] в
// hot loop — никаких доп. вычислений на сэмпл.
	FixedArray<float> mDecayRelease;
// mAtk[p] — шаг per-partial разгона амплитуды струны после контакта:
// возбуждённые ударом партиалы стартуют с уровня gSeam (последний сэмпл
// буфера атаки) и дорастают до табличной амплитуды (gSeam→1), не
// возбуждённые (G≈0) — с нуля (0→1), всё за τ≈AttackT/k. В любом случае
// mAtk обнуляется на DecayOnset, чтобы не тянуть амплитуду вверх при
// затухании.
	FixedArray<float> mAtk;

// Биения унисона при voices==2: две расстроенные струны коллапсированы в
// ОДИН лейн на партиалу (вдвое меньше синусоид в горячем цикле — было
// 43.8× realtime на Chopin, стало ~70×). Амплитуда лейна уже включает
// g0+g1, а биение воспроизводится медленной огибающей
//   E(t) = sqrt(cos²(Δ·t) + r²·sin²(Δ·t)),  r = (g0−g1)/(g0+g1),
// Δ — половинная разность частот струн (растёт с номером партиалы k:
// Δ ≈ π·k·f0·(det1−det0)/sr — биения у k-й гармоники в k раз быстрее).
// Огибающая пересчитывается раз в блок (псевдоконстанта: mBeatE0/mBeatE1
// на границах блока), внутри блока — линейная интерполяция, поэтому на
// стыках блоков нет ступенек/щелчков. Точность: амплитуда суммы двух
// струн воспроизводится точно; отброшен только фазовый воббл
// |θ| ≤ atan(r) ≈ 10° (периодический, на слух незаметен). Для голосов 1/3
// (и любых не-2-голосных инструментов) mBeatOn=false — лейны как раньше.
	FixedArray<float> mBeatStep, mBeatPh;
	FixedArray<float> mBeatE0, mBeatE1;
	// Accepted true-stereo fingerprint. One oscillator recurrence stays shared
	// between channels; right-channel phase is reconstructed as a linear
	// combination of the two adjacent sine recurrence states.
	FixedArray<float> mStereoPartL, mStereoPartRA, mStereoPartRB;
	// Experimental early harmonic trajectory. Coefficients describe the raw
	// SF2 amplitude before DecayOnset as three common exponential basis
	// functions; env0/env1 are block endpoints for cheap linear interpolation.
	FixedArray<float> mAttackCoeff0, mAttackCoeff1, mAttackCoeff2;
	FixedArray<float> mAttackEnv0, mAttackEnv1;
	// Current/end exponential basis values for the three shared attack time
	// constants. Advancing these recursively avoids libm exp calls per block.
	float mAttackBasis0 = 1.0f, mAttackBasis1 = 1.0f, mAttackBasis2 = 1.0f;
	float mAttackBasisEnd0 = 0.0f, mAttackBasisEnd1 = 0.0f, mAttackBasisEnd2 = 0.0f;
	float mAttackBasisStep0 = 1.0f, mAttackBasisStep1 = 1.0f, mAttackBasisStep2 = 1.0f;
	// Attack interpolation is anchored to fixed note-relative 128-sample
	// segments, not caller/render blocks. This makes the onset bit-stable when
	// MIDI events, cache handoff or host block sizes split a segment midway.
	float mAttackSegEndBasis0 = 1.0f, mAttackSegEndBasis1 = 1.0f, mAttackSegEndBasis2 = 1.0f;
	size_t mAttackInterpPos = 0, mAttackInterpLen = 0;
	// Per-partial квадрат глубины r²: огибающая E = sqrt(1 − (1−r²)·sin²).
	// r зависит от партиалы через вес w(k) (см. конструктор): низкие партиалы
	// бьются, высокие (h4+) — нет, в басе мелко бьётся и h1. 1.0 = нет биений.
	FixedArray<float> mBeatR2;
	bool mBeatOn;
	// Compact SF2-sample-level amplitude modulation fitted per physical region.
	// This is deliberately COMMON to the summed note, not a per-partial table.
	// It replaces the old k*fBeat unison approximation only where the fitter has
	// an identifiable region-level solution. Runtime cost: two slow evaluations
	// per canonical 128-sample segment, linearly interpolated inside it.
	bool mCommonAmOn = false;
	float mCommonAmFreqHz = 0.0f;
	float mCommonAmGain0 = 0.0f;
	float mCommonAmLambda = 0.0f;
	float mCommonAmPhase = 0.0f;
	float mCommonAmRefInv = 1.0f;
	float mCommonAmPlaybackRate = 1.0f;

	INTRA_FORCEINLINE float CommonAmAtSample(size_t targetSample) const
	{
		if(!mCommonAmOn) return 1.0f;
		const float sourceAge = float(targetSample)*mCommonAmPlaybackRate/float(mSampleRate);
		const float tt = sourceAge - 0.125f;
		const float g = mCommonAmGain0*Math::Exp(-mCommonAmLambda*tt);
		const float ph = 2.0f*float(Math::PI)*mCommonAmFreqHz*tt + mCommonAmPhase;
		const float z = Math::Sqrt(Math::Max(0.0f, 1.0f + g*g + 2.0f*g*Math::Cos(ph)));
		const float fitted = z*mCommonAmRefInv;
		const float mix = Math::Clamp((sourceAge - 0.080f)*(1.0f/0.080f), 0.0f, 1.0f);
		return 1.0f + (fitted - 1.0f)*mix;
	}
	// «Окно свободной атаки»: NoteOff раньше 35 мс откладывает
	// демпфер до конца окна (см. NoteRelease/RenderInto).
	bool mReleasePending;
	size_t mReleaseAt;
	unsigned mSampleRate;
	// Скрэтч-аккумуляторы: 4 лейна на сэмпл блока, отдельно L/R.
	FixedArray<float> mScratch;
	FixedArray<float> mScratchR;
	size_t mCount;   // число осцилляторов (партиалы × струны, кратно 4)
	float mVolume;
	// Переключение затухания: старт на DecayOnset (1.0 → λ1), λ1 → λ2 на
	// DecayOnset+SegT, λ2 → λ3 на DecayOnset+SegT+SegT2, λ3 → λ4 на
	// DecayOnset+SegT+SegT2+SegT3 (границы — окна измерений, из таблицы).
	size_t mDecayOnsetSamples;
	// Raw-SF2 contact prelude: coherent modal string is revealed only after the
	// contact window. The string state itself is analytically fast-forwarded to
	// this age, so hidden oscillators do not burn CPU sample-by-sample.
	size_t mStringRevealSamples = 0;
	// The recorded sample transition is continuous, unlike a binary gate. Blend
	// the already-developed string in over a short source-time window so reveal
	// does not inject a broadband step/click. This changes output only, never the
	// analytically sought modal state.
	size_t mStringRevealBlendSamples = 0;
	size_t mSegSamples;
	size_t mSegSamples2;
	size_t mSegSamples3;
	size_t mRendered;
	bool mDecayStarted;
	bool mSegSwitched;
	bool mSegSwitched2;
	bool mSegSwitched3;
	// Конец ноты: SF2 без лупа — fluidsynth играет семпл один раз, нота
	// заканчивается на SampleLen региона (масштабировано транспозицией: семпл,
	// сыгранный в 2 раза быстрее, в 2 раза короче). Перед концом — короткий
	// фейд (иначе щелчок); после — тишина.
	size_t mEndSamples;
	size_t mFadeSamples;
	// Стерео: панорамирование голосов унисона. При StereoPan > 0 голос 0
	// (первая струна) панорамируется влево, голос 1 — вправо. Pan = 0
	// означает моно (L=R). Измерено по разнице уровней L/R семплов SF2:
	// tanh(dB/6) даёт мягкую S-кривую в диапазоне [-1; +1].
	float mStereoGainL;
	float mStereoGainR;
	// MIDI CC10 is a channel pan layer on top of the measured per-partial
	// stereo image. Gains are normalized so pan=0 is exactly the accepted
	// stereo render; hard pan matches the existing linear pan law used by
	// WaveTableSampler (the surviving side is 2x relative to centered 0.5).
	float mMidiPanGainL = 1.0f;
	float mMidiPanGainR = 1.0f;

	// Clavinova program 0 reuses the same raw stereo samples for all six
	// velocity zones. Keep the raw source independent of velocity, then apply
	// the SF2 velocity low-pass as an outer per-note layer. Coefficients use
	// the same 2-pole Butterworth form as FilterCoeffs::Calculate.
	bool mVelocityFilterModel = false;
	bool mVelocityFilterBypass = true;
	bool mVelocityFilterInPartials = false;
	size_t mVelocityFilterHandoffSamples = 0;
	float mVelA1 = 0, mVelA2 = 0, mVelB1 = 0, mVelB2 = 0, mVelC = 1;
	float mVelPrevSrcL = 0, mVelPrevSrc2L = 0, mVelPrevOutL = 0, mVelPrevOut2L = 0;
	float mVelPrevSrcR = 0, mVelPrevSrc2R = 0, mVelPrevOutR = 0, mVelPrevOut2R = 0;
	// P1/P2 in the original SF2 also route the very slow modulation envelope
	// to filter cutoff. It is negligible inside the 200 ms PCM onset cache,
	// then updated in the partial domain at a low control rate.
	float mVelocityFilterBaseCutoff = 19912.0f;
	float mVelocityFilterCurrentCutoff = 19912.0f;
	float mVelocityModEnvCents = 0.0f;
	float mVelocityModReleaseLevel = 0.0f;
	size_t mVelocityModNextUpdate = 0;
	size_t mVelocityModReleaseSample = 0;
	bool mVelocityModReleased = false;

	// Compact upper-register SF2 harmonic attack residual.  The packed modal
	// state is measured at DecayOnset; roots 96+ need a hotter common transient
	// before that point.  Region-level envelopes plus one shared velocity law
	// reconstruct it without per-partial or velocity-zone tables.  Applied
	// outside the raw onset cache so cache ON/OFF use the same envelope.
	float mAttackBoostExp = 1.0f;
	float mAttackBoostStep = 1.0f;
	float mAttackBoostEndExp = 1.0f;
	float mAttackBoostDepth = 0.0f;
	float mAttackBoostPolyDepth = 0.0f;
	float mAttackBoostVelocityScale = 1.0f;
	size_t mAttackBoostSamples = 0;

	INTRA_FORCEINLINE float StepAttackBoost(size_t t)
	{
		if(mAttackBoostSamples == 0 || t >= mAttackBoostSamples) return 1.0f;
		if(mAttackBoostPolyDepth != 0.0f)
		{
			const float u = float(t)/float(mAttackBoostSamples);
			const float u2 = u*u;
			return 1.0f + mAttackBoostPolyDepth*mAttackBoostVelocityScale*(1.0f - u2*u2);
		}
		const float g = 1.0f + mAttackBoostDepth*mAttackBoostVelocityScale*(mAttackBoostExp - mAttackBoostEndExp);
		mAttackBoostExp *= mAttackBoostStep;
		if(mAttackBoostExp < mAttackBoostEndExp) mAttackBoostExp = mAttackBoostEndExp;
		return g;
	}

	// Таблица партиал/регионов (PianoRegions.h): по умолчанию общая
	// (acoustic), per-instrument SF2-таблицы — через PianoTableId.
	const PianoTable* mTable;
	uint8 mTableId = 0; // compact preset/table selector for velocity calibration
	uint8 mVelocityProfile = 0;
	uint8 mRegionIndex = 0;
	// mDone = true когда нота полностью закончилась (mRendered >= mEndSamples):
	// GenerateMono/GenerateStereo возвращают 0, и NoteSampler удаляет голос.
	bool mDone;
// mReleased = true после NoteRelease().  Для SF2-match acoustic P1
// естественная траектория партиал продолжает идти как при удержании, а поверх
// неё действует общий линейный volume-envelope release FluidSynth.
	bool mReleased;
	bool mSf2UniformRelease = false;
	float mSf2ReleaseGain = 1.0f;
	float mSf2ReleaseStep = 1.0f;
	size_t mSf2ReleaseSamples = 0;
	size_t mSf2ReleaseSamplesLeft = 0;

	INTRA_FORCEINLINE float ApplyVelocityFilter(float x, bool right)
	{
		if(!mVelocityFilterModel || mVelocityFilterBypass) return x;
		float &ps = right ? mVelPrevSrcR : mVelPrevSrcL;
		float &ps2 = right ? mVelPrevSrc2R : mVelPrevSrc2L;
		float &po = right ? mVelPrevOutR : mVelPrevOutL;
		float &po2 = right ? mVelPrevOut2R : mVelPrevOut2L;
		const float y = x*mVelC + ps*mVelA1 + ps2*mVelA2 + po*mVelB1 + po2*mVelB2;
		ps2 = ps; ps = x; po2 = po; po = y;
		return y;
	}

	void SetHeldStringStateAt(size_t target);
	void PromoteVelocityFilterToPartials();
	void UpdateVelocityModEnvelope();
	void SetVelocityFilterCutoff(float newCutoff);
	void ApplyVelocityFilterCutoffRatio(float newCutoff);
public:
	static const size_t mBlockSize = 512;

	///   MaxPartials — верхняя граница числа партиал (режется также по Найквисту
	///     с запасом 8%);
	///   Brightness — 0..1, при > 0.25 слегка усиливает верхние партиалы
	///     (a_k ·= k^(0.8·(Brightness−0.25))); 0.25 — нейтрально;
	///   Scale — пик суммы партиал (нормируется синтезом одного периода);
	///   DecayScale — множитель на измеренное затухание (1 = как в семпле,
	///     <1 — длиннее, >1 — короче);
	///   DecayStiffness — «жёсткость»: λ_k ·= (1 + c·k²) (0 = по семплу);
	///   DetuneCents — расстройка унисона (полный разброс, 0 = одна струна);
	///   UnisonVoices — число «струн» на ноту (1..3);
	///   VelBrightness — чувствительность яркости к velocity (0..1);
	///   TrebleTilt — подавление обертонов с ростом высоты (0 = по семплу).
	///
	/// Атака — контактная сила: короткий sin²-импульс возбуждает те же
	/// моды, что звучат в сустейне (см. конструктор); отдельного слоя
	/// молоточка нет.
	AdditiveSampler(float freq, float volume, unsigned sampleRate,
		size_t maxPartials, float brightness, float scale, float decayScale,
		float decayStiffness, float detuneCents,
		int unisonVoices, float velBrightness, float trebleTilt,
		float volumeScale = 1.0f, float beatScale = 1.0f, int tableId = 0,
		float beatCents = 0, uint8 velocityProfile = 0);
	~AdditiveSampler() override;

	/// Рендерит numSamples отсчётов. Лямбда-sink — как у KS/SpectralString:
	/// на wasm поинтер-инкремент в лямбде даёт лучший код, чем индексная
	/// запись. Сумма партиал копится в 4 лейна на сэмпл блока; после блока
	/// лейны сворачиваются и уходят в sink.
	template<typename TSink> void RenderInto(size_t numSamples, TSink&& sink)
	{
		const size_t count = mCount;
		float* s1 = mS1.Data();
		float* s2 = mS2.Data();
		float* k = mK.Data();
		float* amp = mAmp.Data();
		float* dec = mDecay.Data();
		float* atk = mAtk.Data();
		const float* stereoL = mStereoPartL.Data();
		const float* stereoRA = mStereoPartRA.Data();
		const float* stereoRB = mStereoPartRB.Data();
	const float volL = mVolume*mMidiPanGainL;
	const float volR = mVolume*mMidiPanGainR;
	while(numSamples)
	{
		if(mReleasePending && mRendered >= mReleaseAt)
		{
			mReleasePending = false;
			ApplyRelease();
		}
		// Before the coherent string reveal there is no audible modal string.
		// Do not spend the SIMD hot loop advancing inaudible oscillators sample by
		// sample: advance note time cheaply, then analytically seek the complete
		// string state once at the reveal boundary. This prefix is intentionally
		// kept as a separate hook for the hammer/contact model below.
		if(mStringRevealSamples != 0 && mRendered < mStringRevealSamples)
		{
			size_t pre = Math::Min(numSamples, mStringRevealSamples - mRendered);
			const size_t start = mRendered;
			mRendered += pre;
			for(size_t i = 0; i < pre; i++)
			{
				const size_t t = start + i;
				const float contact = 0.0f;
				const float attackBoost = StepAttackBoost(t);
				float releaseGain = 1.0f;
				if(mReleased && mSf2UniformRelease)
				{
					releaseGain = mSf2ReleaseGain;
					if(mSf2ReleaseSamplesLeft != 0)
					{ mSf2ReleaseGain *= mSf2ReleaseStep; --mSf2ReleaseSamplesLeft; }
				}
				sink(ApplyVelocityFilter(contact, false)*volL*releaseGain*attackBoost,
					ApplyVelocityFilter(contact, true)*volR*releaseGain*attackBoost);
			}
			numSamples -= pre;
			if(mRendered == mStringRevealSamples) SetHeldStringStateAt(mRendered);
			continue;
		}
		size_t n = Math::Min(mBlockSize, numSamples);
			// The fastest basis is 10 ms; keep early blocks short enough that
			// endpoint interpolation is accurate. Sustain keeps the normal 512.
			if(mRendered < mDecayOnsetSamples) n = Math::Min(n, size_t(128));
			float commonAm0 = 1.0f, commonAmStep = 0.0f;
			if(mCommonAmOn)
			{
				// Anchor interpolation to fixed note-relative 128-sample segments so
				// host block boundaries and cache handoff cannot change the waveform.
				const size_t pos = mRendered & size_t(127);
				const size_t base = mRendered - pos;
				n = Math::Min(n, size_t(128) - pos);
				const float a = CommonAmAtSample(base);
				const float b = CommonAmAtSample(base + 128);
				commonAmStep = (b - a)*(1.0f/128.0f);
				commonAm0 = a + commonAmStep*float(pos);
			}
			if(mVelocityFilterModel && !mVelocityFilterInPartials && mVelocityFilterHandoffSamples > mRendered)
				n = Math::Min(n, mVelocityFilterHandoffSamples - mRendered);
			// Переключение на следующий сегмент затухания: старт на DecayOnset,
			// затем λ1 → λ2 на SegT, λ2 → λ3 на SegT2 (границы из таблицы).
			// При release (демпфере) эти переключения пропускаются — dec[p]
			// уже установлен на mDecayRelease в NoteRelease().
			const bool naturalActive = !mReleased || mSf2UniformRelease;
			if(naturalActive && !mDecayStarted && mRendered >= mDecayOnsetSamples)
			{
				float* dec1 = mDecay1.Data();
				for(size_t p = 0; p < count; p++) { dec[p] = dec1[p]; atk[p] = 0.0f; }
				mDecayStarted = true;
			}
			if(naturalActive && !mSegSwitched && mRendered >= mSegSamples)
			{
				float* dec2 = mDecay2.Data();
				for(size_t p = 0; p < count; p++) dec[p] = dec2[p];
				mSegSwitched = true;
			}
			if(naturalActive && !mSegSwitched2 && mRendered >= mSegSamples2)
			{
				float* dec3 = mDecay3.Data();
				for(size_t p = 0; p < count; p++) dec[p] = dec3[p];
				mSegSwitched2 = true;
			}
			if(naturalActive && !mSegSwitched3 && mRendered >= mSegSamples3)
			{
				float* dec4 = mDecay4.Data();
				for(size_t p = 0; p < count; p++) dec[p] = dec4[p];
				mSegSwitched3 = true;
			}
			// Не пересекать ближайшую границу внутри блока: следующий проход
			// применит новый шаг с точного сэмпла границы. При release — без
			// границ (демпфер не переключается).
			if(naturalActive)
			{
				if(!mDecayStarted) n = Math::Min(n, mDecayOnsetSamples - mRendered);
				else if(!mSegSwitched) n = Math::Min(n, mSegSamples - mRendered);
				else if(!mSegSwitched2) n = Math::Min(n, mSegSamples2 - mRendered);
				else if(!mSegSwitched3) n = Math::Min(n, mSegSamples3 - mRendered);
			}
			if(n == 0) continue;
			// Start/continue a fixed note-relative attack interpolation segment.
			// The segment endpoints never depend on how the host split RenderInto,
			// so cache/no-cache and different audio block sizes render identically.
			if(mRendered < mDecayOnsetSamples)
			{
				if(mAttackInterpLen == 0)
				{
					const size_t segLen = Math::Min(size_t(128), mDecayOnsetSamples - mRendered);
					auto advance = [segLen](float step)
					{
						float r = 1.0f, b = step;
						size_t q = segLen;
						while(q) { if(q & 1) r *= b; b *= b; q >>= 1; }
						return r;
					};
					mAttackSegEndBasis0 = mAttackBasis0*advance(mAttackBasisStep0);
					mAttackSegEndBasis1 = mAttackBasis1*advance(mAttackBasisStep1);
					mAttackSegEndBasis2 = mAttackBasis2*advance(mAttackBasisStep2);
					const float b00 = mAttackBasis0 - mAttackBasisEnd0;
					const float b01 = mAttackBasis1 - mAttackBasisEnd1;
					const float b02 = mAttackBasis2 - mAttackBasisEnd2;
					const float b10 = mAttackSegEndBasis0 - mAttackBasisEnd0;
					const float b11 = mAttackSegEndBasis1 - mAttackBasisEnd1;
					const float b12 = mAttackSegEndBasis2 - mAttackBasisEnd2;
					const float* c0 = mAttackCoeff0.Data();
					const float* c1 = mAttackCoeff1.Data();
					const float* c2 = mAttackCoeff2.Data();
					float* e0 = mAttackEnv0.Data();
					float* e1 = mAttackEnv1.Data();
					for(size_t p = 0; p < count; p++)
					{
						e0[p] = Math::Max(0.05f, 1.0f + c0[p]*b00 + c1[p]*b01 + c2[p]*b02);
						e1[p] = Math::Max(0.05f, 1.0f + c0[p]*b10 + c1[p]*b11 + c2[p]*b12);
					}
					mAttackInterpPos = 0;
					mAttackInterpLen = segLen;
				}
				n = Math::Min(n, mAttackInterpLen - mAttackInterpPos);
			}
			const size_t blockStart = mRendered;
			mRendered += n;
			float* acc = mScratch.Data();
			float* accR = mScratchR.Data();
			for(size_t i = 0; i < 4*n; i++) { acc[i] = 0; accR[i] = 0; }
			// Огибающая биений (коллапс 2-струнного унисона): пересчёт раз в
			// блок на границах [ph, ph + step·n], внутри блока — линейная
			// интерполяция (gv = E0 + (E1−E0)·i/n), поэтому на стыках блоков
			// огибающая непрерывна (нет ступенек/щелчков).
			if(mBeatOn)
			{
				const float* r2a = mBeatR2.Data();
				const float* bs = mBeatStep.Data();
				float* bp = mBeatPh.Data();
				float* e0 = mBeatE0.Data();
				float* e1 = mBeatE1.Data();
				for(size_t p = 0; p < count; p++)
				{
					const float ph = bp[p];
					const float s0 = Math::Sin(ph);
					const float r2 = r2a[p];
					e0[p] = Math::Sqrt(1.0f - (1.0f - r2)*(s0*s0));
					const float ph1 = ph + bs[p]*float(n);
					const float s1 = Math::Sin(ph1);
					e1[p] = Math::Sqrt(1.0f - (1.0f - r2)*(s1*s1));
					bp[p] = ph1;
				}
			}
if(blockStart < mDecayOnsetSamples)
			{
	#if INTRA_SIMD_SUPPORT >= INTRA_SIMD_SSE2
				if(mBeatOn)
				{
					const float* e0a = mBeatE0.Data();
					const float* e1a = mBeatE1.Data();
					const float invN = 1.0f/float(n);
					const float attackInvLen = 1.0f/float(mAttackInterpLen);
					const float attackBase = float(mAttackInterpPos);
					for(size_t p = 0; p < count; p += 4)
					{
						__m128 s1v = _mm_loadu_ps(s1+p);
						__m128 s2v = _mm_loadu_ps(s2+p);
						__m128 kv  = _mm_loadu_ps(k+p);
						__m128 av  = _mm_loadu_ps(amp+p);
						const __m128 dv = _mm_loadu_ps(dec+p);
						const __m128 ak = _mm_loadu_ps(atk+p);
						const __m128 mv = _mm_sub_ps(dv, ak);
						const __m128 glv = _mm_loadu_ps(stereoL+p);
						const __m128 rav = _mm_loadu_ps(stereoRA+p);
						const __m128 rbv = _mm_loadu_ps(stereoRB+p);
						const __m128 b0 = _mm_loadu_ps(e0a + p);
						const __m128 bd = _mm_sub_ps(_mm_loadu_ps(e1a + p), b0);
						const __m128 a0 = _mm_loadu_ps(mAttackEnv0.Data() + p);
						const __m128 ad = _mm_sub_ps(_mm_loadu_ps(mAttackEnv1.Data() + p), a0);
						for(size_t i = 0; i < n; i++)
						{
							const __m128 fi = _mm_set1_ps(float(i)*invN);
							const __m128 afi = _mm_set1_ps((attackBase + float(i))*attackInvLen);
							const __m128 gv = _mm_add_ps(b0, _mm_mul_ps(bd, fi));
							const __m128 ae = _mm_add_ps(a0, _mm_mul_ps(ad, afi));
							const __m128 env = _mm_mul_ps(_mm_mul_ps(av, gv), ae);
							const __m128 outL = _mm_mul_ps(_mm_mul_ps(s1v, env), glv);
							const __m128 carrierR = _mm_add_ps(_mm_mul_ps(rav, s1v), _mm_mul_ps(rbv, s2v));
							const __m128 outR = _mm_mul_ps(carrierR, env);
							const __m128 newS = _mm_sub_ps(_mm_mul_ps(kv, s2v), s1v);
							s1v = s2v;
							s2v = newS;
							av = _mm_add_ps(_mm_mul_ps(av, mv), ak);
							_mm_storeu_ps(acc + 4*i, _mm_add_ps(_mm_loadu_ps(acc + 4*i), outL));
							_mm_storeu_ps(accR + 4*i, _mm_add_ps(_mm_loadu_ps(accR + 4*i), outR));
						}
						_mm_storeu_ps(s1+p, s1v);
						_mm_storeu_ps(s2+p, s2v);
						_mm_storeu_ps(amp+p, av);
					}
				}
				else
				{
					for(size_t p = 0; p < count; p += 4)
					{
						__m128 s1v = _mm_loadu_ps(s1+p);
						__m128 s2v = _mm_loadu_ps(s2+p);
						__m128 kv  = _mm_loadu_ps(k+p);
						__m128 av  = _mm_loadu_ps(amp+p);
						const __m128 dv = _mm_loadu_ps(dec+p);
						const __m128 ak = _mm_loadu_ps(atk+p);
						const __m128 mv = _mm_sub_ps(dv, ak);
						const __m128 glv = _mm_loadu_ps(stereoL+p);
						const __m128 rav = _mm_loadu_ps(stereoRA+p);
						const __m128 rbv = _mm_loadu_ps(stereoRB+p);
						const __m128 a0 = _mm_loadu_ps(mAttackEnv0.Data() + p);
						const __m128 ad = _mm_sub_ps(_mm_loadu_ps(mAttackEnv1.Data() + p), a0);
						const float attackInvLen = 1.0f/float(mAttackInterpLen);
						const float attackBase = float(mAttackInterpPos);
						for(size_t i = 0; i < n; i++)
						{
							const __m128 ae = _mm_add_ps(a0, _mm_mul_ps(ad, _mm_set1_ps((attackBase + float(i))*attackInvLen)));
							const __m128 ave = _mm_mul_ps(av, ae);
							const __m128 outL = _mm_mul_ps(_mm_mul_ps(s1v, ave), glv);
							const __m128 carrierR = _mm_add_ps(_mm_mul_ps(rav, s1v), _mm_mul_ps(rbv, s2v));
							const __m128 outR = _mm_mul_ps(carrierR, ave);
							const __m128 newS = _mm_sub_ps(_mm_mul_ps(kv, s2v), s1v);
							s1v = s2v;
							s2v = newS;
							av = _mm_add_ps(_mm_mul_ps(av, mv), ak);
							_mm_storeu_ps(acc + 4*i, _mm_add_ps(_mm_loadu_ps(acc + 4*i), outL));
							_mm_storeu_ps(accR + 4*i, _mm_add_ps(_mm_loadu_ps(accR + 4*i), outR));
						}
						_mm_storeu_ps(s1+p, s1v);
						_mm_storeu_ps(s2+p, s2v);
						_mm_storeu_ps(amp+p, av);
					}
				}
	#else
				if(mBeatOn)
				{
					const float* e0a = mBeatE0.Data();
					const float* e1a = mBeatE1.Data();
					const float invN = 1.0f/float(n);
					const float attackInvLen = 1.0f/float(mAttackInterpLen);
					const float attackBase = float(mAttackInterpPos);
					for(size_t p = 0; p < count; p++)
					{
						float s1v = s1[p], s2v = s2[p], kv = k[p];
						float av = amp[p];
						const float mv = dec[p] - atk[p];
						const float ak = atk[p];
						const float gl = stereoL[p], ra = stereoRA[p], rb = stereoRB[p];
						const float b0 = e0a[p], bd = e1a[p] - b0;
						const float a0 = mAttackEnv0[p], ad = mAttackEnv1[p] - a0;
						for(size_t i = 0; i < n; i++)
						{
							const float env = av*(b0 + bd*(float(i)*invN))*(a0 + ad*((attackBase + float(i))*attackInvLen));
							acc[4*i] += s1v*env*gl;
							accR[4*i] += (ra*s1v + rb*s2v)*env;
							const float newS = kv*s2v - s1v;
							s1v = s2v;
							s2v = newS;
							av = av*mv + ak;
						}
						s1[p] = s1v;
						s2[p] = s2v;
						amp[p] = av;
					}
				}
				else
				{
					for(size_t p = 0; p < count; p++)
					{
						float s1v = s1[p], s2v = s2[p], kv = k[p];
						float av = amp[p];
						const float mv = dec[p] - atk[p];
						const float ak = atk[p];
						const float gl = stereoL[p], ra = stereoRA[p], rb = stereoRB[p];
						const float a0 = mAttackEnv0[p], ad = mAttackEnv1[p] - a0;
						const float attackInvLen = 1.0f/float(mAttackInterpLen);
						const float attackBase = float(mAttackInterpPos);
						for(size_t i = 0; i < n; i++)
						{
							const float ae = a0 + ad*((attackBase + float(i))*attackInvLen);
							acc[4*i] += s1v*av*ae*gl;
							accR[4*i] += (ra*s1v + rb*s2v)*av*ae;
							const float newS = kv*s2v - s1v;
							s1v = s2v;
							s2v = newS;
							av = av*mv + ak;
						}
						s1[p] = s1v;
						s2[p] = s2v;
						amp[p] = av;
					}
				}
	#endif
			}
			else
			{
	#if INTRA_SIMD_SUPPORT >= INTRA_SIMD_SSE2
				if(mBeatOn)
				{
					const float* e0a = mBeatE0.Data();
					const float* e1a = mBeatE1.Data();
					const float invN = 1.0f/float(n);
					for(size_t p = 0; p < count; p += 4)
					{
						__m128 s1v = _mm_loadu_ps(s1+p);
						__m128 s2v = _mm_loadu_ps(s2+p);
						__m128 kv  = _mm_loadu_ps(k+p);
						__m128 av  = _mm_loadu_ps(amp+p);
						const __m128 dv = _mm_loadu_ps(dec+p);
						const __m128 ak = _mm_loadu_ps(atk+p);
						const __m128 mv = _mm_sub_ps(dv, ak);
						const __m128 glv = _mm_loadu_ps(stereoL+p);
						const __m128 rav = _mm_loadu_ps(stereoRA+p);
						const __m128 rbv = _mm_loadu_ps(stereoRB+p);
						const __m128 b0 = _mm_loadu_ps(e0a + p);
						const __m128 bd = _mm_sub_ps(_mm_loadu_ps(e1a + p), b0);
						for(size_t i = 0; i < n; i++)
						{
							const __m128 gv = _mm_add_ps(b0, _mm_mul_ps(bd, _mm_set1_ps(float(i)*invN)));
							const __m128 env = _mm_mul_ps(av, gv);
							const __m128 outL = _mm_mul_ps(_mm_mul_ps(s1v, env), glv);
							const __m128 carrierR = _mm_add_ps(_mm_mul_ps(rav, s1v), _mm_mul_ps(rbv, s2v));
							const __m128 outR = _mm_mul_ps(carrierR, env);
							const __m128 newS = _mm_sub_ps(_mm_mul_ps(kv, s2v), s1v);
							s1v = s2v;
							s2v = newS;
							av = _mm_add_ps(_mm_mul_ps(av, mv), ak);
							_mm_storeu_ps(acc + 4*i, _mm_add_ps(_mm_loadu_ps(acc + 4*i), outL));
							_mm_storeu_ps(accR + 4*i, _mm_add_ps(_mm_loadu_ps(accR + 4*i), outR));
						}
						_mm_storeu_ps(s1+p, s1v);
						_mm_storeu_ps(s2+p, s2v);
						_mm_storeu_ps(amp+p, av);
					}
				}
				else
				{
					for(size_t p = 0; p < count; p += 4)
					{
						__m128 s1v = _mm_loadu_ps(s1+p);
						__m128 s2v = _mm_loadu_ps(s2+p);
						__m128 kv  = _mm_loadu_ps(k+p);
						__m128 av  = _mm_loadu_ps(amp+p);
						const __m128 dv = _mm_loadu_ps(dec+p);
						const __m128 ak = _mm_loadu_ps(atk+p);
						const __m128 mv = _mm_sub_ps(dv, ak);
						const __m128 glv = _mm_loadu_ps(stereoL+p);
						const __m128 rav = _mm_loadu_ps(stereoRA+p);
						const __m128 rbv = _mm_loadu_ps(stereoRB+p);
						for(size_t i = 0; i < n; i++)
						{
							const __m128 outL = _mm_mul_ps(_mm_mul_ps(s1v, av), glv);
							const __m128 carrierR = _mm_add_ps(_mm_mul_ps(rav, s1v), _mm_mul_ps(rbv, s2v));
							const __m128 outR = _mm_mul_ps(carrierR, av);
							const __m128 newS = _mm_sub_ps(_mm_mul_ps(kv, s2v), s1v);
							s1v = s2v;
							s2v = newS;
							av = _mm_add_ps(_mm_mul_ps(av, mv), ak);
							_mm_storeu_ps(acc + 4*i, _mm_add_ps(_mm_loadu_ps(acc + 4*i), outL));
							_mm_storeu_ps(accR + 4*i, _mm_add_ps(_mm_loadu_ps(accR + 4*i), outR));
						}
						_mm_storeu_ps(s1+p, s1v);
						_mm_storeu_ps(s2+p, s2v);
						_mm_storeu_ps(amp+p, av);
					}
				}
	#else
				if(mBeatOn)
				{
					const float* e0a = mBeatE0.Data();
					const float* e1a = mBeatE1.Data();
					const float invN = 1.0f/float(n);
					for(size_t p = 0; p < count; p++)
					{
						float s1v = s1[p], s2v = s2[p], kv = k[p];
						float av = amp[p];
						const float mv = dec[p] - atk[p];
						const float ak = atk[p];
						const float gl = stereoL[p], ra = stereoRA[p], rb = stereoRB[p];
						const float b0 = e0a[p], bd = e1a[p] - b0;
						for(size_t i = 0; i < n; i++)
						{
							const float env = av*(b0 + bd*(float(i)*invN));
							acc[4*i] += s1v*env*gl;
							accR[4*i] += (ra*s1v + rb*s2v)*env;
							const float newS = kv*s2v - s1v;
							s1v = s2v;
							s2v = newS;
							av = av*mv + ak;
						}
						s1[p] = s1v;
						s2[p] = s2v;
						amp[p] = av;
					}
				}
				else
				{
					for(size_t p = 0; p < count; p++)
					{
						float s1v = s1[p], s2v = s2[p], kv = k[p];
						float av = amp[p];
						const float mv = dec[p] - atk[p];
						const float ak = atk[p];
						const float gl = stereoL[p], ra = stereoRA[p], rb = stereoRB[p];
						for(size_t i = 0; i < n; i++)
						{
							acc[4*i] += s1v*av*gl;
							accR[4*i] += (ra*s1v + rb*s2v)*av;
							const float newS = kv*s2v - s1v;
							s1v = s2v;
							s2v = newS;
							av = av*mv + ak;
						}
						s1[p] = s1v;
						s2[p] = s2v;
						amp[p] = av;
					}
				}
	#endif
			}
			if(blockStart < mDecayOnsetSamples && mAttackInterpLen != 0)
			{
				mAttackInterpPos += n;
				if(mAttackInterpPos == mAttackInterpLen)
				{
					mAttackBasis0 = mAttackSegEndBasis0;
					mAttackBasis1 = mAttackSegEndBasis1;
					mAttackBasis2 = mAttackSegEndBasis2;
					mAttackInterpPos = 0;
					mAttackInterpLen = 0;
				}
			}
			// Свёртка 4 лейнов.
			for(size_t i = 0; i < n; i++)
			{
				float s = (acc[4*i] + acc[4*i+1]) + (acc[4*i+2] + acc[4*i+3]);
				float sr = (accR[4*i] + accR[4*i+1]) + (accR[4*i+2] + accR[4*i+3]);
				const size_t t = mRendered - n + i;
				if(t < mStringRevealSamples) { s = 0.0f; sr = 0.0f; }
				else if(mStringRevealBlendSamples != 0 && t < mStringRevealSamples + mStringRevealBlendSamples)
				{
					const float blend = float(t - mStringRevealSamples)/float(mStringRevealBlendSamples);
					s *= blend; sr *= blend;
				}
				if(mCommonAmOn)
				{
					const float am = commonAm0 + commonAmStep*float(i);
					s *= am; sr *= am;
				}
				// Конец региона: фейд на последних mFadeSamples, дальше тишина
				// (как fluidsynth без лупа — нота заканчивается вместе с семплом).
				if(mEndSamples)
				{
					if(t >= mEndSamples) { s = 0.0f; sr = 0.0f; }
					else if(t + mFadeSamples >= mEndSamples)
					{
						const float f = float(mEndSamples - t)/float(mFadeSamples);
						s *= f; sr *= f;
					}
				}
				const float vl = ApplyVelocityFilter(s, false);
				const float vr = ApplyVelocityFilter(sr, true);
				const float attackBoost = StepAttackBoost(t);
				float releaseGain = 1.0f;
				if(mReleased && mSf2UniformRelease)
				{
					releaseGain = mSf2ReleaseGain;
					if(mSf2ReleaseSamplesLeft != 0)
					{
						mSf2ReleaseGain *= mSf2ReleaseStep;
						--mSf2ReleaseSamplesLeft;
					}
				}
				sink(vl*volL*releaseGain*attackBoost, vr*volR*releaseGain*attackBoost);
			}
			// After the attack every modal amplitude is monotonic. Once the
			// highest SIMD group falls below the inaudible trim threshold it can never
			// become audible again. Drop trailing groups permanently so long
			// piano tails do not keep evaluating dead upper harmonics. Keep the
			// canonical cache builder untrimmed so every note starts from the same
			// full physical source recipe.
			if(
				mRendered >= mDecayOnsetSamples && mCount > 4)
			{
				size_t trimmed = mCount;
				while(trimmed > 4)
				{
					bool silent = true;
					for(size_t q = trimmed - 4; q < trimmed; q++)
					{
						float a = amp[q] < 0.0f ? -amp[q] : amp[q];
						if(mReleased && mSf2UniformRelease) a *= mSf2ReleaseGain;
						const float trimFloor = (mReleased && mSf2UniformRelease) ? 1e-3f : 1e-4f;
						if(a >= trimFloor) { silent = false; break; }
					}
					if(!silent) break;
					trimmed -= 4;
				}
				mCount = trimmed;
			}
			// A raw cache builder must stay raw for the entire cached prefix. The old
			// 160 ms cache ended before the 200 ms velocity-filter handoff; with a
			// 500 ms cache, promoting here would bake the builder velocity into PCM.
			if(
				mVelocityFilterModel && !mVelocityFilterInPartials &&
				mRendered >= mVelocityFilterHandoffSamples)
				PromoteVelocityFilterToPartials();
			if(mVelocityModEnvCents != 0.0f && mRendered >= mVelocityModNextUpdate)
				UpdateVelocityModEnvelope();
			numSamples -= n;
		}
		if(mEndSamples && mRendered >= mEndSamples) mDone = true;
		else if(mReleased)
		{
			if(mSf2UniformRelease)
			{
				// FluidSynth also retires voices once sample*envelope falls below its
				// noise floor. Keep the old proven -60 dB modal gate instead of
				// evaluating an inaudible full 1.2-second release tail.
				float maxAmp = 0.0f;
				for(size_t p = 0; p < mCount; p++)
				{
					const float a = amp[p] < 0.0f ? -amp[p] : amp[p];
					if(a > maxAmp) maxAmp = a;
				}
				if(mSf2ReleaseSamplesLeft == 0 || maxAmp*mSf2ReleaseGain < 1e-3f) mDone = true;
			}
			else
			{
				float maxAmp = 0.0f;
				for(size_t p = 0; p < count; p++)
				{
					const float a = amp[p] < 0.0f ? -amp[p] : amp[p];
					if(a > maxAmp) maxAmp = a;
				}
				if(maxAmp < 1e-3f) mDone = true;
			}
		}
	}

	// Реализации GenerateMono/GenerateStereo/NoteRelease — в AdditiveSampler.cpp
	// (не инлайн: без дублирования кода в TU, меньше WASM).
	size_t GenerateMono(Span<float> ioDst) override;
	size_t GenerateStereo(Span<float> ioDstLeft, Span<float> ioDstRight) override;
	void MultiplyVolume(float volumeMultiplier) override {mVolume *= volumeMultiplier;}

#ifdef INTRA_UI_METERS
	/// Уровень громкости ноты для индикатора веб-UI (см. Sampler::GetLevel).
	/// У пиано общей огибающей нет: затухание «зашито» в партиалы (у каждой
	/// гармоники свой шаг mDecay) — берём максимум по лейнам, то есть самую
	/// громкую гармонику. В атаке и большей части сустейна это фундаментал,
	/// он же определяет воспринимаемую громкость, поэтому яркость индикатора
	/// повторяет затухание ноты в семпле. Уровень относительный (0..1):
	/// velocity и CC7 дорожки домножает UI.
	float GetLevel() const override;
#endif
	bool SupportsEnvelopeRender() const override {return true;}
	size_t GenerateStereoWithEnvelope(Span<float> ioDstLeft, Span<float> ioDstRight,
		const EnvelopeSegment& envelope) override
	{
		if(mDone) return 0;
		const size_t total = Math::Min(ioDstLeft.Length(), ioDstRight.Length());
		float* dstL = ioDstLeft.Data();
		float* dstR = ioDstRight.Data();
		RenderEnvelope gain(envelope);
		float tmpL[mBlockSize], tmpR[mBlockSize];
		size_t done = 0;
		while(done < total)
		{
			const size_t n = Math::Min(mBlockSize, total - done);
			for(size_t i = 0; i < n; i++) tmpL[i] = tmpR[i] = 0.0f;
			GenerateStereo(Span<float>(tmpL, n), Span<float>(tmpR, n));
			for(size_t i = 0; i < n; i++)
			{
				const float g = gain.NextGain();
				dstL[done + i] += tmpL[i]*g;
				dstR[done + i] += tmpR[i]*g;
			}
			done += n;
		}
		return mDone ? 0 : total;
	}
	void NoteRelease() override;
	void SetPan(float newPan) override
	{
		const float pan = Math::Clamp(newPan, -1.0f, 1.0f);
		mMidiPanGainL = 1.0f - pan;
		mMidiPanGainR = 1.0f + pan;
	}
	void SetVelocity(float velocity01) override;
	void ApplyRelease();

};

/// Фабрика аддитивного фортепиано.
///   Brightness — 0..1, > 0.25 усиливает верхние партиалы (ярче тембр);
///   MaxPartials — верхняя граница числа партиал;
///   Scale — пик нормировки (общая громкость инструмента);
///   DecayScale — множитель сустейн-затухания (1 = семпл, <1 длиннее);
///   DecayStiffness — «жёсткость»: верха гаснут быстрее (0 = по семплу);
///   DetuneCents — расстройка унисона (биения; honky-tonk — широкая);
///   UnisonVoices — число «струн» на ноту (1..3);
///   VelBrightness — чувствительность яркости к velocity (0..1);
///   TrebleTilt — подавление обертонов на высоких нотах (0 = по семплу).
struct AdditivePianoInstrument
{
	float Brightness;
	size_t MaxPartials;
	float Scale;
	float DecayScale;
	float DecayStiffness;
	float DetuneCents;
	int UnisonVoices;
	float VelBrightness;
	float TrebleTilt;
	/// VolumeScale — готовый линейный per-instrument множитель громкости.
	/// Постоянные dB-калибровки заранее преобразованы в InstrumentLibrary.cpp:
	/// note-on не вычисляет 10^(dB/20). Scale и тембральные параметры не трогает.
	float VolumeScale = 1.0f;
	/// BeatScale — per-instrument множитель регионального профиля биений
	/// (лестница base в AdditiveSampler, измерена по семплам SF2 2026-08-26).
	/// 1 = полный профиль (эталон — AcousticPiano); 0 = региональная расстройка
	/// отключена у инструмента (только его собственная DetuneCents). Wide-
	/// detune пресеты (honky-tonk 9.0c) на регионе 51 давали ~45 центов и
	/// деструктивные биения (AM до 50 дБ) — для них 0 (см. ворклог 2026-08-29).
	float BeatScale = 1.0f;
	/// TableId — таблица коэффициентов (PianoTableId в PianoRegions.h).
	/// 0 = общая acoustic-таблица (по умолчанию). Honky-Tonk =
	/// PianoTableHonkyTonk: его SF2-семплы — те же, что у acoustic (ворклог
	/// 2026-08-30), поэтому под INTRA_PIANO_ALL_TABLES таблица алиасит
	/// общую (0 лишних байт); без define все TableId резолвятся в общую.
	int TableId = 0;
	/// BeatCents — ПЛОСКАЯ расстройка биений унисона в центах (0 = выкл.).
	/// Когда > 0, заменяет региональную лестницу base (которая откалибрована
	/// по acoustic-семплам Clavinova) на постоянный разброс по всей
	/// клавиатуре, а per-partial вес глубины биений берёт EP-профиль
	/// (h1 полный, h2/h3 половинный, h4+ четверть — все партиалы бьются
	/// вместе, как хорус семпла). Это для EP-инструментов, чьи семплы бьются
	/// примерно постоянной скоростью ~2-3 Гц на C4-C5 (DX7/Rhodes тайны), а
	/// не по лестнице струн рояля (2026-09-04, EP2).
	float BeatCents = 0;
	/// Explicit SF2 velocity/profile id. 0=Grand, 1=Bright, 2=ElectricGrand,
	/// 3=Honky, 4=EP1, 5=EP2, 6=Harpsichord, 7=Clavinet.
	uint8 VelocityProfile = 0;

	GenericSamplerRef operator()(float freq, float volume, unsigned sampleRate) const
	{
		return new AdditiveSampler(freq, volume, sampleRate,
			MaxPartials, Brightness, Scale, DecayScale, DecayStiffness,
			DetuneCents, UnisonVoices, VelBrightness, TrebleTilt, VolumeScale, BeatScale, TableId,
			BeatCents, VelocityProfile);
	}
};

// Prebuild the canonical raw 500 ms region prefix outside the audio callback.
// Repeated keys in the same SF2 region are no-ops after the first build.
INTRA_FORCEINLINE void PreloadAcousticPianoKey(float, unsigned) {}

INTRA_WARNING_POP
