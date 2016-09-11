#include "IO/Stream.h"
#include "IO/File.h"
#include "Sound/Midi.h"
#include "Sound/Music.h"
#include "Sound/SoundBuilder.h"
#include "Sound/Sound.h"
#include "Sound/SoundSource.h"
#include "Core/Time.h"
#include "IO/Networking.h"
#include "Platform/Platform.h"
#include "Threading/Thread.h"

using namespace Intra;
using namespace Intra::IO;

//#define ENABLE_STREAMING

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
	auto args = GetCommandLineArguments();
	if(args.Count()>=2) filePath = args[1];
	return filePath;
}

void PrintMusicInfo(const Music& music)
{
	uint noteCount=0;
	for(auto&& track: music.Tracks)
		for(auto&& note: track.Notes)
			noteCount += int(!note.Note.IsPause());

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
	auto mapping = file.Map<byte>();
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

void MainLoop()
{
#if INTRA_PLATFORM_OS!=INTRA_PLATFORM_OS_Emscripten

	Console.PrintLine("Нажмите любую клавишу, чтобы закрыть...");
#ifdef ENABLE_STREAMING
	Thread thr([]()
	{
		for(;;)
		{
			StreamedSound::UpdateAllExistingInstances();
			Timer::Wait(1);
		}		
	});
#endif
	Console.GetChar();

#else
	emscripten_set_main_loop([]() {}, 30, 1);
#endif
}

void LoadAndPlaySound(StringView filePath)
{
#ifdef ENABLE_STREAMING
	Console.PrintLine("Инициализация...");
	auto sound = StreamedSound::FromFile(filePath);
	sound.Play();
#else
	Sound sound = SynthSoundFromMidi(filePath, true);
	auto inst = sound.CreateInstance();
	inst.Play();
#endif
	Console.PrintLine("Воспроизведение...");
	MainLoop();
}

void PlayMusic(const Music& music, bool printPerf)
{
	Timer tim;
	SoundBuffer buf = music.GetSamples();
	if(printPerf)
	{
		auto time = tim.GetTime();
		Console.PrintLine("Время синтеза: ", ToString(time*1000, 2), " мс.");
	}
	Sound snd(&buf);
	auto inst = snd.CreateInstance();
	inst.Play();
	MainLoop();
}

#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Emscripten)

extern "C" void PlayUrl(const char* url)
{
	ByteBuffer bb = DownloadFile(StringView(url));
	auto music = ReadMidiFile(bb.AsConstByteRange());
	PrintMusicInfo(music);
	PlayMusic(music, true);
}

#endif




using namespace IO;
using namespace Range;

int INTRA_CRTDECL main()
{

#if INTRA_DISABLED
	SoundBuffer buf(2000000, 44100);
	for(uint t=0; t<buf.Samples.Count(); t++)
		buf.Samples[t] = ((byte)( ( (((((t>>3)|(t>>7))*5)|(t>>4))&0xff)/4 + (((((t>>3)|(t>>12))*5)|(t>>7))&0xff)*3/4 ) )-128)/127.0f;
	Sound snd = Sound(&buf);
	snd.CreateInstance().Play();
	Console.GetChar();
#endif

#if INTRA_DISABLED
	const String filePath1 = GetMidiPath("music_vibraphone_only.mid");
	auto music = ReadMidiFile(filePath1);
	for(auto n: music.Tracks[0].Notes)
		IO::Console << int(n.Note.Note+n.Note.Octave*12-69) << ", ";
	IO::Console << IO::endl;
#endif

#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Emscripten)
	PlayUrl("http://gammaker.my.to/site-storage/Midi/lady_gaga-paparazzi.mid");
#else

	//Errors::InitSignals();

	const String filePath = GetMidiPath("lady_gaga-paparazzi.mid");
	
	bool success = PrintMidiFileInfo(filePath);
	if(!success) return false;

	LoadAndPlaySound(filePath);
#endif

	return 0;
}
