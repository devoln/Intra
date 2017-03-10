#include "MusicSynthesizerCommon.h"

#include "IO/Stream.h"
#include "IO/FileSystem.h"
#include "Audio/Midi.h"
#include "Audio/Music.h"
#include "Audio/AudioBuffer.h"
#include "Audio/Sound.h"
#include "Audio/AudioSource.h"
#include "Platform/Time.h"
#include "Platform/CppWarnings.h"

INTRA_DISABLE_REDUNDANT_WARNINGS

using namespace Intra;
using namespace Intra::IO;
using namespace Intra::Audio;

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


void PrintMusicInfo(const Music& music)
{
	uint noteCount = 0;
	for(auto&& track: music.Tracks)
		for(auto&& note: track.Notes)
			noteCount += uint(!note.Note.IsPause());

	Console.PrintLine("Длительность музыки: ", ToString(music.Duration(), 2), " с.");
	Console.PrintLine("Число нот: ", noteCount);
	Console.PrintLine("Число дорожек: ", music.Tracks.Count());
}

bool PrintMidiFileInfo(StringView filePath)
{
	Console.PrintLine("Загрузка midi файла ", filePath, "...");
	auto fileMapping = OS.MapFile(filePath);
	if(fileMapping==null)
	{
		Console.PrintLine("Файл не открыт!");
		Console.GetChar();
		return false;
	}
	auto music = ReadMidiFile(fileMapping.AsRange());

	if(music.Tracks==null)
	{
		Console.PrintLine("Ошибка!");
		Console.GetChar();
		return false;
	}

	PrintMusicInfo(music);

	return true;
}

Sound SynthSoundFromMidi(StringView filePath, bool printMessages)
{
	if(printMessages) Console.PrintLine("Синтез...");
	Timer tim;
	auto sound = Sound::FromFile(filePath);
	if(printMessages)
	{
		auto time = tim.GetTime();
		Console.PrintLine("Время синтеза: ", ToString(time*1000, 2), " мс.");
	}
	return sound;
}
