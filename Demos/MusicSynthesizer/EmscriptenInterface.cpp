

#include "MusicSynthesizerCommon.h"
#include "MidiInstrumentMapping.h"

#include "Audio/Midi/MidiFileParser.h"
#include "MidiSynth.h"

using namespace Audio;

#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Emscripten)
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

extern "C"
{
	IAudioSource* EMSCRIPTEN_KEEPALIVE SourceCreateFromMidiFileData(char* midiDataPtr, uint midiDataLength, uint sampleRate, uint numChannels)
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

	uint EMSCRIPTEN_KEEPALIVE SourceSamplesLeft(IAudioSource* source)
	{
		return uint(source->SamplesLeft());
	}

	uint EMSCRIPTEN_KEEPALIVE SourceGetUninterleavedSamples(IAudioSource* source, float* dst, uint count, uint bufferSizeInSamples)
	{
		Span<float> channels[2];
		for(auto& channel: channels)
		{
			channel = Range::SpanOfPtr(dst, count);
			dst += bufferSizeInSamples;
		}
		return uint(source->GetUninterleavedSamples(Range::Take(channels, source->ChannelCount())));
	}

	char* EMSCRIPTEN_KEEPALIVE GetMidiInfoString(char* midiDataPtr, uint midiDataLength)
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
