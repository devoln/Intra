

#include "MusicSynthesizerCommon.h"
#include "MidiInstrumentMapping.h"

#include "Audio/Midi/MidiFileParser.h"
#include "MidiSynth.h"

using namespace Audio;

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

extern "C"
{
	IAudioSource* EMSCRIPTEN_KEEPALIVE SourceCreateFromMidiFileData(char* midiDataPtr, unsigned midiDataLength, unsigned sampleRate, unsigned numChannels)
	{
		ErrorStatus status;
		auto stream = SpanOfRaw(midiDataPtr, midiDataLength);
		Midi::MidiFileInfo info(stream, status);
		if(status.Handle()) return nullptr;

		auto mapping = GetMapping();
		mapping.Preload(info, sampleRate);
		// Реверберация отключена по умолчанию; обработка включается через SourceSetParams.
		// При нулевом wet master-bus эффект не выполняет работу.
		return new MidiSynth(
			Midi::MidiFileParser::CreateSingleOrderedMessageStream(stream, status),
			info.Duration, mapping, 0.05f, nullptr, sampleRate, numChannels >= 2, true, false, false);
	}

	void EMSCRIPTEN_KEEPALIVE SourceFree(IAudioSource* sourcePtr)
	{
		delete sourcePtr;
	}

	// Живой источник для "чистой игры": без MIDI-файла, бесконечный поток
	// тишины, который звучит только от SourceSendMidiEvent (Web MIDI-клавиатура,
	// экранное пианино). Программы инструментов грузятся лениво при первой ноте.
	IAudioSource* EMSCRIPTEN_KEEPALIVE SourceCreateLive(unsigned sampleRate, unsigned numChannels)
	{
		return new MidiSynth(
			Midi::TrackCombiner(1), Infinity, GetMapping(), 0.05f, nullptr,
			sampleRate, numChannels >= 2, true, true, false);
	}

	// Единая точка входа для живых MIDI-событий (вместо набора кастомных API):
	// статус-байт + до двух байт данных, как в Web MIDI. Диспетчеризуется в
	// текущий поток (MidiSynth::SendMidiEvent) с временем "сейчас": Note On/Off,
	// Program Change (заменяет прежний SourceSetProgram), Pitch Bend, Control
	// Change (7/10/123). Позже сюда же можно направлять события MIDI-клавиатуры.
	void EMSCRIPTEN_KEEPALIVE SourceSendMidiEvent(IAudioSource* sourcePtr, unsigned status, unsigned data0, unsigned data1)
	{
		static_cast<MidiSynth*>(sourcePtr)->SendMidiEvent(byte(status), byte(data0), byte(data1));
	}

	// Устанавливает все runtime-параметры конкретного источника одной структурой.
	// ABI: один float — ReverbWet.
	void EMSCRIPTEN_KEEPALIVE SourceSetParams(IAudioSource* sourcePtr, const RenderParams* params)
	{
		if(params) static_cast<MidiSynth*>(sourcePtr)->SetRenderParams(*params);
	}

	// Кольцо обратной связи: канальные события (CC7/CC10/Program Change),
	// замеченные синтезом ИЗ ФАЙЛА или от живого ввода. UI осушает кольцо
	// и отражает изменения, сделанные самим MIDI-файлом, в своих контролах.
	unsigned EMSCRIPTEN_KEEPALIVE SourceDrainMidiFeedback(IAudioSource* sourcePtr, byte* dst, unsigned capEvents)
	{
		return unsigned(static_cast<MidiSynth*>(sourcePtr)->DrainMidiFeedback(
			reinterpret_cast<byte(*)[3]>(dst), size_t(capEvents)));
	}

	// Уровни нот каналов для индикаторов дорожек в веб-UI: на канал 2 байта —
	// номер ноты (0xFF = канал молчит) и уровень её огибающей (0..127). Тело
	// собирается только с -DINTRA_UI_METERS (канонический WASM); без него —
	// пустая заглушка, чтобы минимальная сборка не тянула ни строчки.
	void EMSCRIPTEN_KEEPALIVE SourceGetNoteLevels(IAudioSource* sourcePtr, byte* dst)
	{
#ifdef INTRA_UI_METERS
		static_cast<MidiSynth*>(sourcePtr)->GetChannelNoteLevels(dst);
#else
		(void)sourcePtr;
		(void)dst;
#endif
	}

	// Мьют дорожек веб-UI: битовая маска каналов (1 = замьючен). Мьют — слой
	// микшера, а не громкость: CC7 канала (его меняет и сам файл, и ползунок UI)
	// остаётся как есть, замьюченный канал просто не звучит. Уже звучащие ноты
	// новомьюченного канала гасятся синтезатором.
	void EMSCRIPTEN_KEEPALIVE SourceSetChannelMute(IAudioSource* sourcePtr, unsigned mask)
	{
		static_cast<MidiSynth*>(sourcePtr)->SetChannelMuteMask(ushort(mask));
	}

	// Мгновенная перемотка вперёд: звучащее гасится, события файла до targetSample
	// пропускаются без рендера, поэтому после seek играют только ноты,
	// начинающиеся после цели (пропущенный участок разом не звучит).
	// Назад — пересозданием источника в JS.
	void EMSCRIPTEN_KEEPALIVE SourceFastForward(IAudioSource* sourcePtr, unsigned targetSample)
	{
		static_cast<MidiSynth*>(sourcePtr)->FastForward(size_t(targetSample));
	}

	unsigned EMSCRIPTEN_KEEPALIVE SourceSamplesLeft(IAudioSource* source)
	{
		return unsigned(source->SamplesLeft());
	}

	unsigned EMSCRIPTEN_KEEPALIVE SourceGetUninterleavedSamples(IAudioSource* source, float* dst, unsigned count, unsigned bufferSizeInSamples)
	{
		Span<float> channels[2];
		for(auto& channel: channels)
		{
			channel = SpanOfPtr(dst, count);
			dst += bufferSizeInSamples;
		}
		return unsigned(source->GetUninterleavedSamples(Take(channels, source->ChannelCount())));
	}

	char* EMSCRIPTEN_KEEPALIVE GetMidiInfoString(char* midiDataPtr, unsigned midiDataLength)
	{
		auto stream = SpanOfRaw(midiDataPtr, midiDataLength);
		ErrorStatus status;
		Midi::MidiFileInfo info(stream, status);
		if(status.Handle()) return nullptr;
		const String resultStr = String::Concat(
			"Длительность музыки: ", StringOf(info.Duration, 2), " с.\n",
			"Число нот: ", info.NoteCount, "\n",
			"Число дорожек: ", info.TrackCount, "\n",
			"Число каналов: ", info.ChannelsUsed,
		'\0');
		char* result = static_cast<char*>(malloc(resultStr.Length()));
		resultStr.AsConstRange().CopyTo(SpanOfPtr(result, resultStr.Length()));
		return result;
	}
}
