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
#include "MusicalInstrument.h"
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

	// Ручной выбор инструмента из веб-UI (событие 0xC0 от веб-UI) — ФОРС
	// канала: пока он стоит, Program Change ИЗ ФАЙЛА на этом канале не
	// применяется вообще (владелец: «ручной выбор инструмента должен
	// форсить его и игнорить любые переключения инструментов на канале»).
	// Без флага файл перезаписывал выбор UI своим событием программы — после
	// выбора «Flute» вместо Pan Flute в дорожке всё равно звучала Pan Flute
	// (и в реальном времени, и в полной генерации). Снимается выбором
	// «Исходный из файла» (0xFF).
	bool mChannelProgramForced[16];

	// Громкость/пан/инструмент каналов — единое MIDI-состояние (CC7/CC10/
	// Program Change). Оно одно для событий из любого источника: файлового
	// потока и живого ввода (SendMidiEvent). Громкость из веб-UI посылается
	// обычным MIDI CC7 — раскатка на ноты происходит в одном месте (OnNoteOn),
	// никакого отдельного gain-пути.
	byte mLiveVolume[16];
	byte mLivePan[16];
	double mLastLiveEventTime = -1;

	// Мьют дорожек веб-UI: битовая маска каналов (1 = замьючен). Мьют — слой
	// микшера ПОВЕРХ MIDI-громкости: CC7 канала остаётся его настоящим
	// состоянием (его меняет и сам файл, и ползунок UI), а мьют задаёт
	// Sampler::ChannelGain = 0 звучащим нотам. Ноты при этом НЕ гасятся, поэтому
	// снятие мьюта возвращает их в том же месте огибающей. Мьют через CC7=0 для
	// этого не годится: файл сам присылает CC7 (например, tous les garçons — CC7
	// по 7 каналам в начале) и отменял бы мьют.
	uint16 mChannelMuteMask = 0;

	// Кольцо обратной связи для веб-UI: канальные события (NoteOn/CC7/CC10/
	// Program Change), замеченные синтезатором ИЗ ЛЮБОГО источника — файловый
	// поток (OnChannelControlChange/OnProgramChange/OnNoteOn) или живой ввод.
	// UI осушает кольцо SourceDrainMidiFeedback. Точка фиксации — синтезатор:
	// парсер файла о фидбеке не знает. 3 байта на событие, переполнение
	// затирает старейшее. Ёмкость 64 (192 байта статики): UI осушает кольцо
	// каждый кадр (~16 мс), плотность событий каналов много меньше. Стоимость:
	// несколько записей на событие, нулевая — на семпл.
	static constexpr size_t FEEDBACK_CAP = 64;
	byte mFeedback[FEEDBACK_CAP][3];
	size_t mFeedbackHead = 0, mFeedbackCount = 0;
	void PushFeedback(byte status, byte d0, byte d1)
	{
		const size_t idx = (mFeedbackHead + mFeedbackCount) % FEEDBACK_CAP;
		if(mFeedbackCount == FEEDBACK_CAP) mFeedbackHead = (mFeedbackHead + 1) % FEEDBACK_CAP;
		else mFeedbackCount++;
		mFeedback[idx][0] = status; mFeedback[idx][1] = d0; mFeedback[idx][2] = d1;
	}

	// Перемотка вперёд читает события файла (чтобы состояние каналов было
	// актуальным), но ноты пропущенного участка не создаёт: иначе все они
	// зазвучали бы разом — «оглушающий взрыв» при каждом seek. Так же (решение
	// владельца, Update 84) гасятся и голоса, звучавшие до перемотки: после seek
	// играют только ноты, начинающиеся ПОСЛЕ цели, — обычное поведение
	// MIDI-секвенсера. Это дешевле дозвучивания удерживаемых нот на 1.4 КБ WASM
	// и не даёт транзиента переатакованных педальных нот.
	bool mSkippingEvents = false;

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

	RenderParams mRenderParams;
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
	void OnChannelControlChange(byte channel, byte control, byte value) final;
	void OnProgramChange(byte channel, byte program) final;

	/// Applies the source-level parameters to the master effects and to any
	/// currently sounding note-local processors. Reverb is a master-bus effect;
	/// it is never configured on an individual sampler.
	void SetRenderParams(const RenderParams& params);

	/// Мьют каналов (дорожек) из веб-UI: битовая маска, 1 = канал замьючен.
	/// Уже звучащие ноты новомьюченных каналов гасятся сразу, новые ноты таких
	/// каналов не создаются. Громкость канала (CC7) не затрагивается.
	void SetChannelMuteMask(ushort mask)
	{
		const ushort changed = ushort(mask ^ mChannelMuteMask);
		mChannelMuteMask = mask;
		for(byte ch = 0; changed != 0 && ch < 16; ch++)
			if((changed >> ch) & 1) UpdateChannelGain(ch);
	}

	/// Меняет инструмент канала на лету: последующие ноты канала будут
	/// синтезироваться программой program (GM-номер). 0xFF снимает переопределение.
	/// Волновые таблицы выбранного инструмента строятся сразу (на JS-потоке,
	/// вне аудио-колбэка), чтобы первая нота после смены инструмента не
	/// генерировала таблицы внутри рендера и не роняла звук.
	void SetChannelProgram(byte channel, byte program)
	{
		// Ставит и ФОРС канала: пришедшее от UI переопределение — ручное
		// решение, которое файл больше не имеет права перебить.
		if(channel < 16) {mChannelProgramOverride[channel] = program; mChannelProgramForced[channel] = program != 0xFF;}
		if(program != 0xFF && program < 128)
		{
			auto* instr = mInstruments.Instruments[program];
			if(instr) instr->PreloadTables(mSampleRate);
		}
	}

	/// Осушает кольцо обратной связи канальных событий (CC7/CC10/Program Change,
	/// пришедших ИЗ ФАЙЛА или от живого ввода) в буфер вызывающего. Формат записи:
	/// 3 байта — статус+канал, data0, data1. Возвращает число записанных событий.
	size_t DrainMidiFeedback(byte dst[][3], size_t maxEvents)
	{
		const size_t n = Min(mFeedbackCount, Min(maxEvents, FEEDBACK_CAP));
		for(size_t i = 0; i < n; i++)
		{
			const size_t idx = (mFeedbackHead + i) % FEEDBACK_CAP;
			dst[i][0] = mFeedback[idx][0];
			dst[i][1] = mFeedback[idx][1];
			dst[i][2] = mFeedback[idx][2];
		}
		mFeedbackHead = (mFeedbackHead + n) % FEEDBACK_CAP;
		mFeedbackCount -= n;
		return n;
	}

#ifdef INTRA_UI_METERS
	/// Уровни нот каналов для индикаторов дорожек в веб-UI: 16 байт, на канал —
	/// уровень огибающей самой громкой звучащей ноты (0 = канал молчит). Номер
	/// ноты UI берёт из кольца фидбека (NoteOn), здесь нужна только громкость.
	/// Дёргается из JS раз в ~200 мс — на семпл расходов нет.
	void GetChannelNoteLevels(byte* dst);
#endif

	/// Мгновенная перемотка: гасит всё звучащее и пропускает события файла до
	/// времени targetSample без рендера (линейно дешевле реального времени).
	/// Ноты пропущенного участка НЕ создаются (иначе они зазвучали бы разом на
	/// целевой позиции — «оглушающий взрыв»), а голоса, звучавшие до перемотки,
	/// отпускаются: после seek играют только ноты, начинающиеся после цели.
	/// Назад — через пересоздание источника в веб-UI (состояние потока не
	/// обращаемо).
	void FastForward(size_t targetSample)
	{
		const double targetTime = double(targetSample) / mSampleRate;
		// Голоса не убиваем, а отпускаем (NoteRelease): их релиз доигрывается
		// ~0.5 с и гаснет сам, а мгновенное обнуление дало бы щелчок.
		for(byte ch = 0; ch < 16; ch++) OnAllNotesOff(ch);
		mSkippingEvents = true;
		while(!mMusic.Empty() && mMusic.NextEventTime() <= targetTime)
			mMusic.ProcessEvent(*this);
		mSkippingEvents = false;
		mTime = targetTime;
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
	/// Громкость канала как живой множитель для ноты: 0 — дорожка замьючена,
	/// иначе CC7 канала относительно CC7 в момент рождения ноты (BornCC7).
	float ChannelGainFor(byte channel, byte bornCC7) const
	{
		if((mChannelMuteMask >> channel) & 1) return 0.0f;
		return float(mLiveVolume[channel])/float(bornCC7);
	}

	/// CC7, с которым нота родится живой: 0 запрещён (нота, рождённая при
	/// нулевой громкости дорожки, ждёт подъёма CC7 и звучит тогда).
	static byte BornCC7For(byte volume) {return volume == 0 ? byte(1) : volume;}

	/// Пересчитывает живой множитель громкости у всех звучащих нот канала
	/// (изменение CC7 файла/ползунка или мьюта). Само тело ноты не трогается —
	/// меняется только слой поверх него, поэтому доигрывающие ноты слышат
	/// изменение сразу, а их тембр остаётся тем, с каким они родились.
	void UpdateChannelGain(byte channel);

	double liveEventTime();
	bool synthNote(Sampler& sampler, Span<float> ioDstLeft, Span<float> ioDstRight);
	float pitchBendToFreqMultiplier(short relativePitchBend) const;
};

INTRA_WARNING_POP
