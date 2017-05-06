#include "MusicSynthesizerCommon.h"

#include "IO/ConsoleInput.h"
#include "IO/Std.h"
#include "IO/FileSystem.h"
#include "Audio/Midi.h"
#include "Audio/Music.h"
#include "Audio/AudioBuffer.h"
#include "Audio/Sound.h"
#include "Audio/AudioSource.h"
#include "Platform/Time.h"
#include "Cpp/Warnings.h"

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


void PrintMusicInfo(const Music& music)
{
	uint noteCount = 0;
	for(auto&& track: music.Tracks)
		for(auto&& note: track.Notes)
			noteCount += uint(!note.Note.IsPause());

	Std.PrintLine("Длительность музыки: ", StringOf(music.Duration(), 2), " с.");
	Std.PrintLine("Число нот: ", noteCount);
	Std.PrintLine("Число дорожек: ", music.Tracks.Count());
}

bool PrintMidiFileInfo(StringView filePath)
{
	Std.PrintLine("Загрузка midi файла ", filePath, "...");
	auto fileMapping = OS.MapFile(filePath);
	if(fileMapping==null)
	{
		Std.PrintLine("Файл не открыт!");
		ConsoleIn.GetChar();
		return false;
	}
	auto music = ReadMidiFile(fileMapping.AsRange());

	if(music.Tracks==null)
	{
		Std.PrintLine("Ошибка!");
		ConsoleIn.GetChar();
		return false;
	}

	PrintMusicInfo(music);

	return true;
}

Sound SynthSoundFromMidi(StringView filePath, bool printMessages)
{
	if(printMessages) Std.PrintLine("Синтез...");
	Timer tim;
	auto sound = Sound::FromFile(filePath);
	if(printMessages)
	{
		auto time = tim.GetTime();
		Std.PrintLine("Время синтеза: ", StringOf(time*1000, 2), " мс.");
	}
	return sound;
}
