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

#if defined(INTRA_PROBE_NAN) || defined(INTRA_PROBE_ACTIVE_VOICES)
#include <stdio.h>
#endif

using namespace Audio;
using namespace Midi;

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

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
	mReverberator(size_t(reverb ? 16384 : 0), size_t(reverb ? 32 : 0), 1),
	mCompressor(compress ? sampleRate : 0u),
	mCompress(compress),
	mLiveMode(live)
#ifndef __EMSCRIPTEN__
	, mExecutor()
#endif
{
	for(auto& p: mChannelProgramOverride) p = 0xFF;
	for(auto& v: mLiveVolume) v = 127;
	for(auto& p: mLivePan) p = 64;
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
		SamplerTaskContainer tasks;
		Array<SamplerJob> jobs;
		for(auto noteSamplers = mNoteSamplers.AsRange(); !noteSamplers.Empty();)
		{
			const size_t samplerIndex = noteSamplers.Index;
			auto& sampler = noteSamplers.Next();
			const uint16 key = sampler.GetInfo<NoteInfo>().Key();
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
#ifdef __EMSCRIPTEN__
		frame.RunTasks(tasks);
#else
		mExecutor.Run(frame, tasks, jobs.AsConstRange(), samplesBeforeNextEvent);
#endif

		Span<float> dstLeftPart = dstLeft.Take(samplesBeforeNextEvent);
		Span<float> dstRightPart = dstRight.Take(samplesBeforeNextEvent);
		Add(dstLeftPart, frame.Channels[0].Take(samplesBeforeNextEvent));
		if(!dstRightPart.Empty()) Add(dstRightPart, frame.Channels[1].Take(samplesBeforeNextEvent));

		// 3. Эффекты (реверберация добавляется к уже сгенерированным каналам).
		if(mReverberator)
			mReverberator(dstLeftPart, dstRightPart, frame.Channels[2].Take(samplesBeforeNextEvent));

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
		// (compress=false): эталон звука там — SF2-семплы через fluidsynth, без
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

void MidiSynth::OnNoteOn(const Midi::NoteOn& noteOn)
{
	// web-midisynth: volume = Math.exp(velocity/127 - 1) * instrument.Volume * (CC7/127).
	// The old linear velocity*CC7/(127*127) crushed soft notes (velocity=1 was ~47x quieter
	// than web), so soft piano melody notes disappeared under the sustained pads in layered
	// files like Celine. Exponential curve reproduces web's per-note loudness exactly.
	const float totalStartVolume = Math::Exp(float(noteOn.Velocity)/127.0f - 1.0f) * (float(noteOn.Volume)/127.0f);
	const uint16 key = noteOn.Id();

	auto found = mPlayingNoteMap.Find(key);
	if(!found.Empty())
	{
		const auto samplerIndex = found.First().Value;
		auto& sampler = mNoteSamplers.Get(samplerIndex);
		auto& samplerInfo = sampler.GetInfo<NoteInfo>();
		if(samplerInfo.Time == float(noteOn.Time)) return;
		sampler.NoteRelease();
	}

	if(noteOn.Channel == 9)
	{
		auto instr = mInstruments.DrumInstruments[noteOn.NoteOctaveOrDrumId];
		if(instr == nullptr) return;
		NoteSampler note;
		note.GenericSamplers.AddLast((*instr)(totalStartVolume, mSampleRate));
		auto& stored = mNoteSamplers.Add<NoteSampler>(Move(note));
		const uint16 idx = uint16(mNoteSamplers.Length() - 1);
		stored.GetInfo<NoteInfo>() = NoteInfo{float(noteOn.Time), noteOn.Channel, noteOn.NoteOctaveOrDrumId};
		mPlayingNoteMap[key] = idx;
		return;
	}

	const byte overrideProg = mChannelProgramOverride[noteOn.Channel];
	const byte instrument = overrideProg != 0xFF ? overrideProg : noteOn.Instrument;
	auto instr = mInstruments.Instruments[instrument];
	if(instr == nullptr) return;

	uint16 idx = 0;
	Sampler& newSampler = instr->CreateSampler(noteOn.Frequency(), totalStartVolume, mSampleRate, mNoteSamplers, &idx);
	newSampler.GetInfo<NoteInfo>() = NoteInfo{float(noteOn.Time), noteOn.Channel, noteOn.NoteOctaveOrDrumId};
	newSampler.SetPan(float(noteOn.Pan) / 64.0f);
	const float freqMult = pitchBendToFreqMultiplier(mMidiState.ChannelPitchBend[noteOn.Channel]);
	if(freqMult != 1) newSampler.MultiplyPitch(freqMult);
	mPlayingNoteMap[key] = idx;
}

void MidiSynth::OnNoteOff(const Midi::NoteOff& noteOff)
{
	auto found = mPlayingNoteMap.Find(noteOff.Id());
	if(!found.Empty())
	{
		const auto samplerIndex = found.First().Value;
		mNoteSamplers.Get(samplerIndex).NoteRelease();
		mPlayingNoteMap.Remove(noteOff.Id());
	}
}

void MidiSynth::OnPitchBend(const Midi::PitchBend& pitchBend)
{
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
		const auto& info = sampler.GetInfo<NoteInfo>();
		if(info.Channel == channel) sampler.NoteRelease();
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
		noteOn.Volume = mLiveVolume[channel];
		noteOn.Pan = sbyte(mLivePan[channel] - 64);
		OnNoteOn(noteOn);
		return;
	}
	case 0xB0: // Control Change
	{
		if(data0 == 0x7B) OnAllNotesOff(channel);            // All Notes Off
		else if(data0 == 0x07) mLiveVolume[channel] = data1; // Channel Volume
		else if(data0 == 0x0A) mLivePan[channel] = data1;    // Pan
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
