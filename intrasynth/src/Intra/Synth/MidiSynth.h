#pragma once

#include "Utils/FixedArray.h"
#include "System/Error.h"
#include "Container/Associative/HashMap.h"
#include "Container/Sequential/Array.h"

#include "Audio/AudioSource.h"
#include "Types.h"
#include "Audio/Midi/Messages.h"
#include "Audio/Midi/MidiFileParser.h"
#include "InstrumentSet.h"
#include "PostEffects.hh"
#include "Sampler.h"

#ifndef __EMSCRIPTEN__
#include "ParallelSynthExecutor.h"
#endif

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

struct MidiState
{
	short ChannelPitchBend[16]{};
	uint16 PitchBendRangeInSemitones = 2;

	MidiState() {}
};

class MidiSynth: public Audio::SeparateFloatAudioSource, public Audio::Midi::IDevice
{
	MidiInstrumentSet mInstruments;
	Audio::Midi::TrackCombiner mMusic;
	double mTime = 0, mPrevTime = 0;
	size_t mSampleCount;
	float mMaxSample;

	MidiState mMidiState;

	// Переопределение программы на канал (0xFF = нет) — смена инструментов на
	// лету через MIDI Program Change (SourceSendMidiEvent). Применяется к
	// последующим нотам: и к нотам файла, и к живым нотам.
	byte mChannelProgramOverride[16];

	// Живой ввод (SourceSendMidiEvent): состояние каналов для событий, приходящих
	// из браузера. В файле его ведёт DeviceState внутри TrackCombiner, у живых
	// событий доступа к нему нет, поэтому дублируем только то, что нужно нотам.
	byte mLiveVolume[16];
	byte mLivePan[16];
	double mLastLiveEventTime = -1;

	// Sustain-педаль (CC64) на канал. Пока педаль нажата, NoteOff не демпфирует
	// голос — он продолжает звучать и отпускается только при снятии педали.
	bool mSustain[16] = {};

	// Живой режим (SourceCreateLive): без MIDI-файла, бесконечный поток тишины,
	// звучащий только от SendMidiEvent. Рендер не завершается, когда нет ни нот,
	// ни музыки (иначе поток бы закончился в первом же кадре тишины).
	bool mLiveMode;

	SamplerContainer mNoteSamplers;

	struct NoteInfo
	{
		float Time;
		byte Channel;
		byte NoteOctaveOrDrumId;
		// Голос получил NoteOff при нажатой педали и ждёт её снятия.
		bool SustainHold = false;
		// NoteRelease уже вызывался: защита от двойного демпфирования (повторный
		// NoteOn той же клавиши, AllNotesOff, снятие педали после повторного удара).
		bool Released = false;

		INTRA_FORCEINLINE uint16 Key() const {return uint16((Channel << 8) | NoteOctaveOrDrumId);}
	};

	typedef Container::HashMap<uint16, uint16> NoteSamplerMap;
	NoteSamplerMap mPlayingNoteMap;

	PostEffects::HallReverb mReverberator;
	PostEffects::DynamicsCompressor mCompressor;
	bool mCompress;
	Array<float> mReverbChannelBuffer;

#ifndef __EMSCRIPTEN__
	ParallelSynthExecutor mExecutor;
#endif

public:
	MidiSynth(Audio::Midi::TrackCombiner music, double duration, const MidiInstrumentSet& instruments, float maxVolume=1,
		BasicAudioSource::OnCloseResourceCallback onClose=nullptr, unsigned sampleRate=48000, bool stereo=true, bool reverb=true,
		bool live=false, bool compress=true);
	~MidiSynth() {}

	MidiSynth(const MidiSynth&) = delete;
	MidiSynth& operator=(const MidiSynth&) = delete;

#ifndef __EMSCRIPTEN__
	static Unique<MidiSynth> FromFile(StringView path, double duration, const MidiInstrumentSet& instruments,
		float maxVolume=1, unsigned sampleRate=48000, bool stereo=true, ErrorStatus& status=Error::Skip());
#endif

	size_t SampleCount() const final {return mSampleCount;}
	size_t SamplePosition() const final {return size_t(mTime*mSampleRate);}

	size_t GetUninterleavedSamplesAdd(Span<const Span<float>> outFloats);
	size_t GetUninterleavedSamples(Span<const Span<float>> outFloats) final;

	void OnNoteOn(const Audio::Midi::NoteOn& noteOn) final;
	void OnNoteOff(const Audio::Midi::NoteOff& noteOff) final;
	void OnPitchBend(const Audio::Midi::PitchBend& pitchBend) final;
	void OnAllNotesOff(byte channel) final;
	void OnSustain(byte channel, bool down) final;

	/// Меняет инструмент канала на лету: последующие ноты канала будут
	/// синтезироваться программой program (GM-номер). 0xFF снимает переопределение.
	void SetChannelProgram(byte channel, byte program)
	{
		if(channel < 16) mChannelProgramOverride[channel] = program;
	}

	/// Отправляет одно сырое MIDI-сообщение (статус-байт + до двух байт данных)
	/// в текущий поток. Диспетчеризуется как в MIDI-файле: Note On/Off, Program
	/// Change, Pitch Bend, Control Change (7 — громкость, 10 — панорама,
	/// 123 — All Notes Off). Время события — текущая позиция потока, поэтому
	/// сообщение звучит немедленно. Единая точка входа для живого ввода
	/// (Web MIDI-клавиатура и т.п.) вместо набора кастомных API. Control Change:
	/// 64 — sustain-педаль, 7 — громкость, 10 — панорама, 123 — All Notes Off.
	void SendMidiEvent(byte status, byte data0, byte data1);

private:
	double liveEventTime();
	bool synthNote(Sampler& sampler, Span<float> ioDstLeft, Span<float> ioDstRight, Span<float> ioDstReverb);
	float pitchBendToFreqMultiplier(short relativePitchBend) const;
};

INTRA_WARNING_POP
