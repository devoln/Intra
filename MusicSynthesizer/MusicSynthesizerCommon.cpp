#include "MusicSynthesizerCommon.h"

#include "IO/Stream.h"
#include "IO/File.h"
#include "Sound/Midi.h"
#include "Sound/Music.h"
#include "Sound/SoundBuilder.h"
#include "Sound/Sound.h"
#include "Sound/SoundSource.h"
#include "Core/Time.h"

using namespace Intra;
using namespace Intra::IO;

String GetMidiPath(StringView fileName)
{
	String ResDir = "Resources/";
	for(size_t i=0; i<5; i++)
	{
		if(DiskFile::Exists(ResDir)) break;
		ResDir = "../"+ResDir;
	}
	String filePath = fileName;
	if(!DiskFile::Exists(filePath)) filePath = ResDir+"Music/Midi/"+fileName;
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
	auto file = DiskFile::Reader(filePath);
	if(file==null)
	{
		Console.PrintLine("Файл не открыт!");
		Console.GetChar();
		return false;
	}
	auto mapping = file.Map<Intra::byte>();
	auto music = ReadMidiFile(mapping);
	file.Unmap();

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

