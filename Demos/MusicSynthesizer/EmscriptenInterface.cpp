

#include "MusicSynthesizerCommon.h"
#include "MidiInstrumentMapping.h"

#include "Extra/Unstable/Audio/Midi/MidiFileParser.h"
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
		if(status.Handle()) return null;

		auto mapping = GetMapping();
		mapping.Preload(info, sampleRate);
		return new MidiSynth(
			Midi::MidiFileParser::CreateSingleOrderedMessageStream(stream, status),
			info.Duration, mapping, 0.5f, null, sampleRate, numChannels >= 2);
	}

	void EMSCRIPTEN_KEEPALIVE SourceFree(IAudioSource* sourcePtr)
	{
		delete sourcePtr;
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
		if(status.Handle()) return null;
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
