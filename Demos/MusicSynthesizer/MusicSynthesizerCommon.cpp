#include "MusicSynthesizerCommon.h"

#include "Extra/IO/ConsoleInput.h"
#include "Extra/IO/Std.h"
#include "Extra/IO/FileSystem.h"
#include "Extra/IO/FileReader.h"

#include "Extra/Container/Sequential/String.h"

#include "Extra/Unstable/Audio/Midi/MidiFileParser.h"
#include "Extra/Unstable/Audio/AudioBuffer.h"
#include "Extra/Unstable/Audio/Sound.h"
#include "Extra/Unstable/Audio/AudioSource.h"
#include "Extra/Unstable/Audio/Sources/MidiSynth.h"

#include "Extra/System/Stopwatch.h"


#include "MidiInstrumentMapping.h"

INTRA_DISABLE_REDUNDANT_WARNINGS

using namespace Intra;
using namespace IO;
using namespace Audio;

String GetMidiPath(StringView fileName)
{
	String ResDir = "Resources/";
	for(size_t i=0; i<5; i++)
	{
		if(OS.FileExists(ResDir)) break;
		ResDir = "../"+ResDir;
	}
	String filePath = fileName;
	if(!OS.FileExists(filePath)) filePath = ResDir+"Music/Midi/"+fileName;
	return filePath;
}


Midi::MidiFileInfo PrintMidiInfo(InputStream stream, ErrorStatus& status)
{
	Midi::MidiFileInfo info(Move(stream), status);
	Std.PrintLine("Длительность музыки: ", StringOf(info.Duration, 2), " с.");
	Std.PrintLine("Число нот: ", info.NoteCount);
	Std.PrintLine("Число дорожек: ", info.TrackCount);
	Std.PrintLine("Число каналов: ", info.ChannelsUsed);
	return info;
}

bool PrintMidiFileInfo(StringView filePath)
{
	Std.PrintLine("Загрузка midi файла ", filePath, "...");

	FatalErrorStatus status;
	auto file = OS.FileOpen(filePath, status);
	if(file) PrintMidiInfo(Move(file), status);

	if(status.Handle())
	{
		Std.PrintLine(status.GetLog());
		ConsoleIn.GetChar();
		return false;
	}

	return true;
}

Unique<IAudioSource> CreateMidiAudioSource(InputStream midiFileStream,
	double duration, float startingVolume, ErrorStatus& status, unsigned sampleRate)
{
	return new Sources::MidiSynth(
		Midi::MidiFileParser::CreateSingleOrderedMessageStream(Move(midiFileStream), status),
		duration, GetMapping(), startingVolume, null, sampleRate == 0? Sound::DefaultSampleRate(): sampleRate, true);
}

Sound CreateSoundFromMidi(ForwardStream midiFilestream, double duration, float startingVolume, bool printMessages)
{
	if(printMessages) Std.PrintLine("Синтез...");
	FatalErrorStatus status;
	Stopwatch sw;
	Sound sound = Sound(CreateMidiAudioSource(Move(midiFilestream), duration, startingVolume, status), status);
	if(printMessages) Std.PrintLine("Время синтеза: ", StringOf(sw.ElapsedSeconds()*1000, 2), " мс.");
	if(status.Handle())
	{
		Std.PrintLine(status.GetLog());
		ConsoleIn.GetChar();
		return null;
	}
	return sound;
}

StreamedSound CreateStreamedSoundFromMidi(ForwardStream midiFileStream, float startingVolume, bool printMessages)
{
	Midi::MidiFileInfo info(midiFileStream, Error::Skip());
	GetMapping().Preload(info, Sound::DefaultSampleRate());
	if(printMessages) Std.PrintLine("Инициализация синтезатора...");
	FatalErrorStatus status;
	StreamedSound sound = StreamedSound(CreateMidiAudioSource(Move(midiFileStream), Infinity, startingVolume, status), 16384);
	if(status.Handle())
	{
		Std.PrintLine(status.GetLog());
		ConsoleIn.GetChar();
		return null;
	}
	return sound;
}
