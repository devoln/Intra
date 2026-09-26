#include "MidiSynth.h"

#include <Cpp/Warnings.h>
#include <Math/Math.h>

#include <Range/Mutation/Fill.h>
#include <Range/Mutation/Transform.h>
#include <Range/Reduction.h>

#include <Audio/Midi/MidiFileParser.h>

#ifndef __EMSCRIPTEN__
#include <IO/FileSystem.h>
#include <IO/FileReader.h>
#endif

#include "MusicalInstrument.h"

#if defined(INTRA_PROBE_NAN) || defined(INTRA_PROBE_ACTIVE_VOICES) || defined(INTRA_FRAME_TRACE)
#include <stdio.h>
#endif

#ifdef INTRA_FRAME_TRACE
// Trace everything up to INTRA_FRAME_TRACE_LIMIT seconds of song time
// (set via env var in probe builds; everything by default).
static const double gTraceLimitSeconds = [] {
	const char* s = ::getenv("INTRA_FRAME_TRACE_LIMIT");
	return s != nullptr ? ::atof(s) : 1e30;
}();
#define FRAME_TRACE(...) do { if(mTime < gTraceLimitSeconds) fprintf(stderr, __VA_ARGS__); } while(0)
#else
#define FRAME_TRACE(...) ((void)0)
#endif

using namespace Audio;
using namespace Midi;

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

// Standard Kit (bank 128/program 0) loudness calibration against Titanic, dry
// reference renderer gain 0.6.  The static table is measured at v=100 from isolated
// strikes and stored in quarter-dB.  Sparse velocity-layer residuals mirror
// only real Titanic reference bank zone boundaries; the common amplitude law stays v^2.
static forceinline float TitanicStandardKitCorrection(byte note, byte velocity)
{
	static const signed char qdb35To81[] =
	{
		 10,   0,  14,   0,  26, -10,  46,   5,  46,  -4,  58,  37,
		 63,  58,  18,  30, -29,  27,   0,  -5,  21,  42,  11,  16,
		-30,  18,  19,   2,  18,  52,  20,  27,  36,  14,  -4, -40,
		 16,  44, -31, -35,  24,  -4,  -7,  29,  38,  -7,  29
	};
	if(note < 35 || note > 81) return 1.0f;
	int qdb = qdb35To81[note - 35];
	switch(note)
	{
	case 37: if(velocity <= 85) qdb -= 17; break;
	case 38:
		if(velocity <= 71) qdb -= 13;
		else if(velocity >= 106) qdb += 5;
		break;
	case 40:
		if(velocity <= 71) qdb -= 23;
		else if(velocity >= 106) qdb += 7;
		break;
	case 42:
		if(velocity <= 84) qdb += 1;
		else if(velocity >= 110) qdb -= 7;
		break;
	case 44: if(velocity <= 84) qdb += 5; break;
	case 51: if(velocity <= 89) qdb -= 10; break;
	case 57: if(velocity <= 89) qdb -= 3; break;
	case 59: if(velocity <= 89) qdb -= 10; break;
	default: break;
	}
	return Math::Exp(float(qdb)*0.02878231366f); // 10^(qdb/80), without generic powf
}

MidiSynth::MidiSynth(Midi::TrackCombiner music, double duration, const MidiInstrumentSet& instruments, float maxVolume,
	BasicAudioSource::OnCloseResourceCallback onClose, unsigned sampleRate, bool stereo, bool reverb, bool live,
	bool compress):
	SeparateFloatAudioSource(Move(onClose), sampleRate, uint16(stereo ? 2 : 1)),
	mInstruments(instruments),
	mMusic(Move(music)),
	mTime(0),
	// Нельзя полагаться на duration == Infinity: при -ffast-math (-ffinite-math-only)
	// сравнение с бесконечностью считается ложным, и поток стал бы конечным нулевой
	// длины. Живой режим задаёт бесконечную длину напрямую флагом.
	mSampleCount(live ? ~size_t() : size_t((duration + 2)*sampleRate)),
	mMaxSample(maxVolume),
	// reverbVolume 2.5: the tap energy in the feedback loop grows super-linearly
	// with volume (vol 1 measured ~2% of dry RMS on a dense file — inaudible; vol 3
	// overshot into grain). 2.5 lands clearly audible without the sandy texture.
	// k 0.5 keeps the decay short enough that the tail does not smear the piano.
	mReverberator(size_t(reverb ? 16384 : 0), size_t(reverb ? 32 : 0), 2.5f, 0.5f),
	mCompressor(compress ? sampleRate : 0u),
	mCompress(compress),
	mLiveMode(live)
#ifndef __EMSCRIPTEN__
	, mExecutor()
#endif
{
	for(auto& p: mChannelProgramOverride) p = 0xFF;
	for(auto& f: mChannelProgramForced) f = false;
	for(auto& v: mLiveVolume) v = 100;
	for(auto& p: mLivePan) p = 64;
#ifdef INTRA_UI_METERS
	for(auto& n: mUiMeterNote) n = 0xFF;
#endif
}

void MidiSynth::SetRenderParams(const RenderParams& params)
{
	RenderParams next = params;
	next.ReverbWet = next.ReverbWet < 0.0f ? 0.0f :
		(next.ReverbWet > 2.0f ? 2.0f : next.ReverbWet);

	// Do not retain old delay-line energy across an explicit off/on cycle.
	// This runs only when the UI changes the mode, never in the audio loop.
	if((mRenderParams.ReverbWet > 0.0f) != (next.ReverbWet > 0.0f))
		mReverberator.Reset();

	mRenderParams = next;
	for(auto noteSamplers = mNoteSamplers.AsRange(); !noteSamplers.Empty();)
		noteSamplers.Next().SetRenderParams(mRenderParams);
}

size_t MidiSynth::GetUninterleavedSamplesAdd(Span<const Span<float>> outFloatChannels)
{
	if(outFloatChannels.Empty()) return 0;

	Span<float> dstLeft = outFloatChannels.First();
	Span<float> dstRight = outFloatChannels.Length() >= 2 ?
		outFloatChannels.Get(1).Take(dstLeft.Length()) : nullptr;

	size_t totalSamplesProcessed = 0;

#ifdef INTRA_PROBE_ACTIVE_VOICES
	static double lastProbeTime = -1e9;
#endif

	while(!dstLeft.Empty())
	{
		const bool musicEmpty = mMusic.Empty();
		const double nextTime = musicEmpty ? Infinity : mMusic.NextEventTime();
		mPrevTime = nextTime;
		// События считаем наступившими с небольшим допуском: время события
		// вычисляется через float TickDuration, а mTime — накоплением семплов,
		// поэтому точное == почти никогда не срабатывает для ненулевых времён.
		if(!musicEmpty && nextTime - mTime <= 0.0001)
		{
			FRAME_TRACE("[EV-FIRE] t=%.9f next=%.9f\n", mTime, nextTime);
			mMusic.ProcessEvent(*this);
			continue;
		}

		// Нельзя полагаться на сравнение nextTime == Infinity: при -ffast-math
		// компилятор считает плавающие значения конечными, поэтому такое
		// сравнение может дать false и обнулить длину кадра (бесконечный цикл).
		size_t samplesBeforeNextEvent = musicEmpty ? ~size_t() :
			size_t(Max((nextTime - mTime)*mSampleRate + 0.5, 1.0));
		const size_t samplesLeft = SamplesLeft();
		if(samplesLeft == 0) break;
		samplesBeforeNextEvent = Min(samplesBeforeNextEvent, samplesLeft);
		samplesBeforeNextEvent = Min(samplesBeforeNextEvent, dstLeft.Length());

		// 1. Голоса генерируют таски в общую очередь, запоминая диапазон задач
		// каждого семплера и суммарную цену — это нужно для равномерного
		// распределения нагрузки между потоками на нативных сборках.
		// (Update 77: отдельный tap-проход для измерения дорожек убран —
		// индикаторы нот в UI строятся из кольца фидбека NoteOn-событий,
		// почти бесплатно; см. PushFeedback в OnNoteOn.)
		SamplerTaskContainer tasks;
		Array<SamplerJob> jobs;
		for(auto noteSamplers = mNoteSamplers.AsRange(); !noteSamplers.Empty();)
		{
			const size_t samplerIndex = noteSamplers.Index;
			auto& sampler = noteSamplers.Next();
			const auto& info = sampler.GetInfo<NoteInfo>();
			const uint16 key = info.Key();
			const uint16 taskBegin = uint16(tasks.Length());
			const bool alive = sampler.Generate(tasks, 0, samplesBeforeNextEvent);
			const uint16 taskEnd = uint16(tasks.Length());
			if(taskEnd != taskBegin)
			{
				uint32 cost = 0;
				for(uint16 t = taskBegin; t < taskEnd; t++)
				{
					SamplerTask* task = tasks[t];
					if(task) cost += task->Cost;
				}
				jobs.AddLast(SamplerJob{taskBegin, taskEnd, uint16(Min<uint32>(cost, 0xFFFFu))});
			}
			if(!alive)
			{
				FRAME_TRACE("[VOICE-END] t=%.9f key=%u age=%.3f\n", mTime, key, double(mTime - sampler.GetInfo<NoteInfo>().Time));
				mNoteSamplers.Delete(samplerIndex);
				// ВАЖНО: удалять запись из mPlayingNoteMap можно только если она всё ещё
				// указывает на умирающий семплер. Если та же нота (канал+клавиша) была
				// перезапущена, пока старый семплер ещё затухал, карта уже указывает на
				// новый семплер — безусловное удаление по ключу потеряло бы note-off
				// новой ноты, и она звучала бы бесконечно (накапливающиеся "зацикленные"
				// фоновые ноты при перекрывающихся дорожках).
				auto found = mPlayingNoteMap.Find(key);
				if(!found.Empty() && found.First().Value == samplerIndex)
					mPlayingNoteMap.Remove(key);
			}
		}

#ifdef INTRA_PROBE_ACTIVE_VOICES
		if(mTime - lastProbeTime >= 5.0)
		{
			lastProbeTime = mTime;
			size_t live = 0, oldCount = 0, oldestAge = 0;
			for(auto probeIter = mNoteSamplers.AsRange(); !probeIter.Empty();)
			{
				auto& s = probeIter.Next();
				live++;
				const float age = float(mTime) - s.GetInfo<NoteInfo>().Time;
				if(age < 0) continue;
				size_t ageSec = size_t(age);
				if(ageSec > oldestAge) oldestAge = ageSec;
				if(ageSec > 10) oldCount++;
			}
			fprintf(stderr, "[VOICES] t=%.1f slots=%zu live=%zu oldest=%zus >10s=%zu\n",
				double(mTime), mNoteSamplers.Length(), live, oldestAge, oldCount);
		}
#endif

		// 2. Выполняем таски в локальные буферы каналов (нативно — параллельно).
		SamplerTaskContext frame(samplesBeforeNextEvent);
		FRAME_TRACE("[FRAME] t=%.9f n=%zu\n", mTime, samplesBeforeNextEvent);
#ifdef __EMSCRIPTEN__
		frame.RunTasks(tasks);
#else
		mExecutor.Run(frame, tasks, jobs.AsConstRange(), samplesBeforeNextEvent);
#endif

		Span<float> dstLeftPart = dstLeft.Take(samplesBeforeNextEvent);
		Span<float> dstRightPart = dstRight.Take(samplesBeforeNextEvent);
		Add(dstLeftPart, frame.Channels[0].Take(samplesBeforeNextEvent));
		if(!dstRightPart.Empty()) Add(dstRightPart, frame.Channels[1].Take(samplesBeforeNextEvent));

	// 3. Master effects. Reverb receives the already mixed dry signal,
	// not a per-voice send. At wet=0 this whole block is skipped, so the
	// reverb comb bank does not consume render CPU in the default mode.
	if(mRenderParams.ReverbWet > 0.0f && mReverberator)
		{
			// Audibility budget (verified in-browser on a dense piano MIDI file):
			// ReverbWet is the direct public control: 1.0 means UI 100%, 2.0 means
			// UI 200%. Recalibrated so 1.0 has the strength of the old 0.4 setting
			// and 2.0 the old 0.8 setting. There is no hidden UI-side remapping.
			const float wet = mRenderParams.ReverbWet;
			const float dryGain = 1.0f - 0.04f*wet;
			if(dryGain != 1.0f)
			{
				for(size_t i = 0; i < samplesBeforeNextEvent; i++)
				{
					dstLeftPart[i] *= dryGain;
					if(!dstRightPart.Empty()) dstRightPart[i] *= dryGain;
				}
			}
			mReverbChannelBuffer.SetCount(samplesBeforeNextEvent);
			const float sendGain = 1.2f * wet;
			for(size_t i = 0; i < samplesBeforeNextEvent; i++)
			{
				const float dryL = frame.Channels[0][i];
				const float dryR = frame.Channels[1][i];
				mReverbChannelBuffer[i] = 0.5f*(dryL + dryR)*sendGain;
			}
			if(dstRightPart.Empty())
			{
				mReverbChannelBuffer.SetCount(samplesBeforeNextEvent);
				for(size_t i = 0; i < samplesBeforeNextEvent; i++)
					mReverbChannelBuffer[i] = frame.Channels[0][i]*sendGain;
			}
			Span<float> reverbRight = dstRightPart.Empty() ? dstLeftPart : dstRightPart;
			mReverberator(dstLeftPart, reverbRight,
				Span<const float>(mReverbChannelBuffer.Data(), samplesBeforeNextEvent));
		}

#ifdef INTRA_PROBE_NAN
		{
			static int probeLines = 0;
			if(probeLines < 40)
			{
				bool bad = false;
				for(size_t i = 0; i < samplesBeforeNextEvent; i++)
				{
					const float l = frame.Channels[0][i], r = frame.Channels[1][i];
					if(!(l == l && r == r && l <= 1e30f && l >= -1e30f && r <= 1e30f && r >= -1e30f)) {bad = true; break;}
				}
				if(bad)
				{
					fprintf(stderr, "[PROBE] non-finite frame at t=%.4f (samplesBeforeNextEvent=%zu) active voices:\n", mTime, samplesBeforeNextEvent);
					for(auto probeIter = mNoteSamplers.AsRange(); !probeIter.Empty();)
					{
						auto& sampler = probeIter.Next();
						const auto& info = sampler.GetInfo<NoteInfo>();
						fprintf(stderr, "[PROBE]   key=%u ch=%u note=%d time=%.4f\n", info.Key(), info.Channel, info.NoteOctaveOrDrumId, info.Time);
					}
					probeLines++;
				}
			}
		}
#endif

		// 4. Мастер-компрессор, повторяющий DynamicsCompressorNode из web-midisynth
		// (threshold -24 дБ, knee 30 дБ, ratio 12:1, attack 3 мс, release 250 мс).
		// Он выравнивает громкость sustained-пэдов и коротких пиано-нот так же, как
		// браузерный мастер-компрессор web-версии. Старая нормализация по бегущему
		// максимуму давала другой баланс громкостей. НО: он же вносит AM-искажения
		// (побочные полосы f2-f1, "плато" затухания) — в Emscripten-порте отключён
		// (compress=false): эталон звука там — reference bank-семплы через reference renderer, без
		// мастер-компрессора.
		if(mCompress) mCompressor(dstLeftPart, dstRightPart);

		dstLeft.PopFirstExactly(samplesBeforeNextEvent);
		if(!dstRight.Empty()) dstRight.PopFirstExactly(samplesBeforeNextEvent);
		totalSamplesProcessed += samplesBeforeNextEvent;
		mTime += double(samplesBeforeNextEvent) / mSampleRate;

		// В живом режиме (без файла) поток бесконечный: не заканчиваем кадр,
		// когда нет ни нот, ни музыки, — продолжаем отдавать тишину.
		if(!mLiveMode && mNoteSamplers.Empty() && mMusic.Empty()) break;
	}

	return totalSamplesProcessed;
}

size_t MidiSynth::GetUninterleavedSamples(Span<const Span<float>> outFloatChannels)
{
	for(auto channel: outFloatChannels) FillZeros(channel);
	return GetUninterleavedSamplesAdd(outFloatChannels);
}

#ifdef INTRA_UI_METERS
void MidiSynth::GetChannelNoteLevels(byte* dst)
{
	// The web UI displays the latest NoteOn per channel. Meter exactly that note,
	// not the loudest/oldest pedal tail on the same channel. If the displayed
	// voice has already ended, return 0 and let the UI fallback finish/fade it.
	float newestTime[16];
	for(size_t ch = 0; ch < 16; ch++) { dst[ch] = 0; newestTime[ch] = -1.0e30f; }
	for(auto samplers = mNoteSamplers.AsRange(); !samplers.Empty();)
	{
		auto& sampler = samplers.Next();
		const auto& info = sampler.GetInfo<NoteInfo>();
		if(info.Channel >= 16 || info.NoteOctaveOrDrumId != mUiMeterNote[info.Channel] ||
			info.Time < newestTime[info.Channel]) continue;
		newestTime[info.Channel] = info.Time;
		dst[info.Channel] = byte(Min(1.0f, Max(0.0f, sampler.GetLevel()))*127.0f + 0.5f);
	}
}
#endif

void MidiSynth::OnNoteOn(const Midi::NoteOn& noteOn)
{
	FRAME_TRACE("[ON] t=%.9f ch=%u note=%u inst=%u vel=%u vol=%u\n", noteOn.Time, noteOn.Channel, noteOn.NoteOctaveOrDrumId, noteOn.Instrument, noteOn.Velocity, noteOn.Volume);
	// Ноты пропущенного перемоткой участка не создаются: все они стартовали бы
	// не в своих файловых временах, а разом на целевой позиции. Состояние
	// каналов при этом читается (см. FastForward), а фидбек не пишется, чтобы UI
	// не подсвечивал пропущенное.
	if(mSkippingEvents) return;
	// Фидбек NoteOn для индикаторов дорожек (веб-UI).
	PushFeedback(byte(0x90 | noteOn.Channel), noteOn.NoteOctaveOrDrumId, noteOn.Velocity);
#ifdef INTRA_UI_METERS
	if(noteOn.Channel < 16) mUiMeterNote[noteOn.Channel] = noteOn.NoteOctaveOrDrumId;
#endif
	// Note-on noteParams are resolved by the selected instrument. MIDI velocity
	// does not cross into the sampler API: the voice receives a ready linear
	// note gain plus a dimensionless strike coordinate for timbral physics.
	// Channel volume is a separate live outer gain and is therefore not baked
	// into the note body. This is algebraically equivalent to the old born-CC7
	// ratio path, including notes created while CC7=0.
	const uint16 key = noteOn.Id();

	auto found = mPlayingNoteMap.Find(key);
	if(!found.Empty())
	{
		const auto samplerIndex = found.First().Value;
		auto& sampler = mNoteSamplers.Get(samplerIndex);
		auto& samplerInfo = sampler.GetInfo<NoteInfo>();
		if(samplerInfo.Time == float(noteOn.Time)) return;
		if(!samplerInfo.Released) sampler.NoteRelease();
		samplerInfo.Released = true;
		samplerInfo.SustainHold = false;
	}

	if(noteOn.Channel == 9)
	{
		auto instr = mInstruments.DrumInstruments[noteOn.NoteOctaveOrDrumId];
		if(instr == nullptr) return;
		const NoteOnParams noteParams = ResolveDefaultNoteOnParams(noteOn.Velocity);
		const float velocity = float(noteOn.Velocity)*0.01f; // legacy key-36 extra factor only
		NoteSampler note;
		note.GenericSamplers.AddLast((*instr)(noteParams.Gain, mSampleRate));
		// Titanic key 36 explicitly overrides the default 960 cB velocity->
		// attenuation modulator with 1440 cB: one extra velocity factor on top
		// of the common direct v^2 construction law.
		const float kick36Velocity = noteOn.NoteOctaveOrDrumId == 36 ? velocity : 1.0f;
		// Static Titanic per-key correction is valid only for the seven percussion
		// models we actually implement. Unimplemented keys deliberately use the
		// neutral bass-drum fallback at its own calibrated level instead of applying
		// an unrelated key's correction (some exceed +15 dB).
		const byte drumKey = noteOn.NoteOctaveOrDrumId;
		const bool calibratedDrumKey = drumKey == 35 || drumKey == 36 || drumKey == 38 ||
			drumKey == 40 || drumKey == 42 || drumKey == 44 || drumKey == 46;
		// Keep the fallback deliberately quieter than the real kick. Its job is to
		// make unsupported GM percussion unobtrusive, not to impersonate each key.
		// 0.18 puts its early energy near the median of the unsupported Titanic keys
		// while leaving plenty of reverb headroom in dense drum patterns.
		const float drumStaticGain = calibratedDrumKey
			? TitanicStandardKitCorrection(drumKey, noteOn.Velocity) : 0.18f;
		note.MultiplyVolume(kick36Velocity*drumStaticGain);
		auto& stored = mNoteSamplers.Add<NoteSampler>(Move(note));
		const uint16 idx = uint16(mNoteSamplers.Length() - 1);
		stored.GetInfo<NoteInfo>() = NoteInfo{float(noteOn.Time), noteOn.Channel, noteOn.NoteOctaveOrDrumId, false, false};
		stored.OutputGain = ChannelOutputGain(noteOn.Channel);
		stored.SetPruneGain(ChannelPruneGain(noteOn.Channel));
		stored.SetRenderParams(mRenderParams);
		mPlayingNoteMap[key] = idx;
		return;
	}

	const byte overrideProg = mChannelProgramOverride[noteOn.Channel];
	const byte instrument = overrideProg != 0xFF ? overrideProg : noteOn.Instrument;
	auto instr = mInstruments.Instruments[instrument];
	if(instr == nullptr) return;

	const NoteOnParams noteParams = instr->ResolveNoteOnParams(noteOn.Velocity);
	uint16 idx = 0;
	Sampler& newSampler = instr->CreateSampler(noteOn.Frequency(), noteParams.Gain, mSampleRate, noteParams, mNoteSamplers, &idx);
	newSampler.GetInfo<NoteInfo>() = NoteInfo{float(noteOn.Time), noteOn.Channel, noteOn.NoteOctaveOrDrumId, false, false};
	newSampler.OutputGain = ChannelOutputGain(noteOn.Channel);
	newSampler.SetPruneGain(ChannelPruneGain(noteOn.Channel));
	newSampler.SetPan(float(noteOn.Pan) / 64.0f);
	newSampler.SetRenderParams(mRenderParams);
	const float freqMult = pitchBendToFreqMultiplier(mMidiState.ChannelPitchBend[noteOn.Channel]);
	if(freqMult != 1) newSampler.MultiplyPitch(freqMult);
	mPlayingNoteMap[key] = idx;
}

void MidiSynth::OnNoteOff(const Midi::NoteOff& noteOff)
{
	FRAME_TRACE("[OFF] t=%.9f ch=%u note=%u\n", noteOff.Time, noteOff.Channel, noteOff.NoteOctaveOrDrumId);
	// Перемотка: голоса пропущенного участка не создавались, а звучавшие до неё
	// уже отпущены в её начале — NoteOff делать нечего. Состояние педали при
	// этом продолжает отслеживаться (её события идут через OnSustain).
	if(mSkippingEvents) return;
	// Отпускание тоже уходит в кольцо фидбека: веб-UI обновляет яркость
	// индикатора сразу по нему, не дожидаясь следующего 200-мс опроса уровней.
	PushFeedback(byte(0x80 | noteOff.Channel), noteOff.NoteOctaveOrDrumId, 0);
	auto found = mPlayingNoteMap.Find(noteOff.Id());
	if(found.Empty()) return;
	const auto samplerIndex = found.First().Value;
	auto& sampler = mNoteSamplers.Get(samplerIndex);
	auto& info = sampler.GetInfo<NoteInfo>();
	if(info.Released) return;
	if(mSustain[noteOff.Channel])
	{
		// Педаль нажата: клавиша отпущена, но струна продолжает звучать до
		// снятия педали. Голос остаётся в mPlayingNoteMap — повторный удар той
		// же клавиши демпфирует его через OnNoteOn, как в обычном пианино.
		info.SustainHold = true;
		return;
	}
	sampler.NoteRelease();
	info.Released = true;
	mPlayingNoteMap.Remove(noteOff.Id());
}

void MidiSynth::OnPitchBend(const Midi::PitchBend& pitchBend)
{
	FRAME_TRACE("[BEND] t=%.9f ch=%u pitch=%d\n", pitchBend.Time, pitchBend.Channel, pitchBend.Pitch);
	const short shift = short(pitchBend.Pitch - mMidiState.ChannelPitchBend[pitchBend.Channel]);
	mMidiState.ChannelPitchBend[pitchBend.Channel] = pitchBend.Pitch;
	const float freqMult = pitchBendToFreqMultiplier(shift);
	for(auto noteSamplers = mNoteSamplers.AsRange(); !noteSamplers.Empty();)
	{
		auto& sampler = noteSamplers.Next();
		const auto& info = sampler.GetInfo<NoteInfo>();
		if(info.Channel == pitchBend.Channel) sampler.MultiplyPitch(freqMult);
	}
}

void MidiSynth::OnAllNotesOff(byte channel)
{
	for(auto noteSamplers = mNoteSamplers.AsRange(); !noteSamplers.Empty();)
	{
		auto& sampler = noteSamplers.Next();
		auto& info = sampler.GetInfo<NoteInfo>();
		if(info.Channel != channel || info.Released) continue;
		sampler.NoteRelease();
		info.Released = true;
		info.SustainHold = false;
	}
}

// Снятие педали после перемотки: голосов пропущенного участка нет, а
// отпущенные в её начале уже доигрывают свой релиз — дополнительно гасить нечего.
void MidiSynth::OnSustain(byte channel, bool down)
{
	FRAME_TRACE("[SUSTAIN] t=%.9f ch=%u down=%d\n", mTime, channel, int(down));
	mSustain[channel] = down;
	if(down) return;
	// Педаль снята: демпфируем все голоса канала, удерживавшиеся ею.
	for(auto noteSamplers = mNoteSamplers.AsRange(); !noteSamplers.Empty();)
	{
		auto& sampler = noteSamplers.Next();
		auto& info = sampler.GetInfo<NoteInfo>();
		if(info.Channel != channel || !info.SustainHold) continue;
		if(!info.Released) sampler.NoteRelease();
		info.Released = true;
		info.SustainHold = false;
	}
}

double MidiSynth::liveEventTime()
{
	// Живые события получают монотонно растущее время: несколько сообщений,
	// присланных между рендер-кадрами, имеют одну и ту же позицию потока (mTime),
	// а OnNoteOn игнорирует повторный NoteOn той же клавиши в тот же момент.
	// Разнос в один семпл не слышен, но триггер ноты обрабатывается корректно.
	const double time = Max(mTime, mLastLiveEventTime);
	mLastLiveEventTime = time + 1.0/mSampleRate;
	return time;
}

void MidiSynth::SendMidiEvent(byte status, byte data0, byte data1)
{
	const byte channel = byte(status & 0x0F);
	switch(status & 0xF0)
	{
	case 0x80: // Note Off
	{
		NoteOff noteOff;
		noteOff.Time = liveEventTime();
		noteOff.Channel = channel;
		noteOff.NoteOctaveOrDrumId = data0;
		noteOff.Velocity = data1;
		OnNoteOff(noteOff);
		return;
	}
	case 0x90: // Note On (velocity 0 = Note Off, как в MIDI)
	{
		if(data1 == 0)
		{
			NoteOff noteOff;
			noteOff.Time = liveEventTime();
			noteOff.Channel = channel;
			noteOff.NoteOctaveOrDrumId = data0;
			noteOff.Velocity = 64;
			OnNoteOff(noteOff);
			return;
		}
		NoteOn noteOn;
		noteOn.Time = liveEventTime();
		noteOn.Channel = channel;
		noteOn.NoteOctaveOrDrumId = data0;
		noteOn.Velocity = data1;
		// Инструмент живого канала: переопределение из Program Change, либо
		// GM-дефолт (фортепиано). Ударные — по маппингу нот, как в файле.
		noteOn.Instrument = channel == 9? byte(data0 + 128): byte(0);
		noteOn.Volume = 127; // mLiveVolume канала применит OnNoteOn — иначе CC7 учтётся дважды
		noteOn.Pan = sbyte(mLivePan[channel] - 64);
		OnNoteOn(noteOn);
		return;
	}
	case 0xB0: // Control Change
	{
		if(data0 == 0x7B) OnAllNotesOff(channel);            // All Notes Off
		else if(data0 == 0x40) OnSustain(channel, data1 >= 64); // Sustain Pedal
		// Живой ввод идёт через ту же точку фиксации, что и файл: состояние
		// канала и фидбек обновляются одинаково для любых источников.
		else if(data0 == 0x07 || data0 == 0x0A) OnChannelControlChange(channel, data0, data1);
		return;
	}
	case 0xC0: // Program Change
	{
		SetChannelProgram(channel, data0);
		return;
	}
	case 0xE0: // Pitch Bend
	{
		PitchBend bend;
		bend.Time = liveEventTime();
		bend.Channel = channel;
		bend.Pitch = short(((data1 << 7) | data0) - 8192);
		OnPitchBend(bend);
		return;
	}
	}
}

float MidiSynth::pitchBendToFreqMultiplier(short relativePitchBend) const
{
	return Pow2(float(relativePitchBend) / 8192.0f * float(mMidiState.PitchBendRangeInSemitones) / 12.0f);
}

// Канальные события ИЗ ЛЮБОГО источника (файл — через IDevice, живой ввод —
// SendMidiEvent): единая точка фиксации CC7 канала. Синтезатор хранит текущий
// CC7/панораму канала и ретранслирует их в кольцо фидбека для веб-UI; OnNoteOn
// применяет громкость канала один раз (см. комментарий там).
void MidiSynth::UpdateChannelGain(byte channel)
{
	const float outputGain = ChannelOutputGain(channel);
	const float pruneGain = ChannelPruneGain(channel);
	for(auto notes = mNoteSamplers.AsRange(); !notes.Empty();)
	{
		auto& sampler = notes.Next();
		if(sampler.GetInfo<NoteInfo>().Channel != channel) continue;
		sampler.OutputGain = outputGain;
		sampler.SetPruneGain(pruneGain);
	}
}

void MidiSynth::UpdateChannelPan(byte channel)
{
	const float pan = (float(mLivePan[channel]) - 64.0f)/64.0f;
	for(auto notes = mNoteSamplers.AsRange(); !notes.Empty();)
	{
		auto& sampler = notes.Next();
		if(sampler.GetInfo<NoteInfo>().Channel != channel) continue;
		sampler.SetPan(pan);
	}
}

void MidiSynth::OnChannelControlChange(byte channel, byte control, byte value)
{
	if(channel >= 16) return;
	if(control == 0x07)
	{
		if(mLiveVolume[channel] != value)
		{
			mLiveVolume[channel] = value;
			// Громкость канала — живой слой: доигрывающие ноты меняют громкость
			// сразу (как в MIDI), а не только последующие.
			UpdateChannelGain(channel);
		}
		PushFeedback(byte(0xB0 | channel), 0x07, value);
	}
	else if(control == 0x0A)
	{
		if(mLivePan[channel] != value)
		{
			mLivePan[channel] = value;
			// CC10 is a live channel layer too: repan notes that are already
			// sounding instead of affecting only future NoteOn events.
			UpdateChannelPan(channel);
		}
		PushFeedback(byte(0xB0 | channel), 0x0A, value);
	}
}

void MidiSynth::OnProgramChange(byte channel, byte program)
{
	if(channel >= 16 || program >= 128) return;
	// Ручной выбор инструмента из веб-UI (ФОРС канала) сильнее файла: событие
	// программы из MIDI-файла не меняет ни инструмент, ни состояние UI.
	if(mChannelProgramForced[channel]) return;
	mChannelProgramOverride[channel] = program;
	auto* instr = mInstruments.Instruments[program];
	if(instr) instr->PreloadTables(mSampleRate);
	PushFeedback(byte(0xC0 | channel), program, 0);
}

#ifndef __EMSCRIPTEN__
Unique<MidiSynth> MidiSynth::FromFile(StringView path, double duration, const MidiInstrumentSet& instruments,
	float maxVolume, unsigned sampleRate, bool stereo, ErrorStatus& status)
{
	auto file = IO::OS.FileOpen(path, status);
	return new MidiSynth(
		Midi::MidiFileParser::CreateSingleOrderedMessageStream(Move(file), status),
		duration, instruments, maxVolume, nullptr, sampleRate, stereo);
}
#endif

INTRA_WARNING_POP
