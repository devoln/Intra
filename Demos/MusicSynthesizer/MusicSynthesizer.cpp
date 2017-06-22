#include "Cpp/PlatformDetect.h"

#include "System/Stopwatch.h"
#include "System/Environment.h"

#include "IO/ConsoleOutput.h"
#include "IO/ConsoleInput.h"
#include "IO/FileSystem.h"
#include "IO/Networking.h"
#include "IO/Std.h"

#include "Concurrency/Thread.h"

#include "Audio/Midi.h"
#include "Audio/Music.h"
#include "Audio/AudioBuffer.h"
#include "Audio/Sound.h"
#include "Audio/AudioSource.h"
#include "Audio/Sources/MusicSynthSource.h"


#include "MusicSynthesizerCommon.h"

//#define ENABLE_STREAMING

#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
#ifdef _MSC_VER
#pragma warning(disable: 4668)
#endif
#ifndef WIN32_MEAN_AND_LEAN
#define WIN32_MEAN_AND_LEAN
struct IUnknown;
INTRA_DISABLE_REDUNDANT_WARNINGS
#include <Windows.h>
#endif


BOOL WINAPI ConsoleCloseHandler(DWORD CtrlType)
{
	(void)CtrlType;
	Intra::Audio::CleanUpSoundSystem();
	return false;
}

#endif


using namespace Intra;
using namespace IO;
using namespace Audio;


#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Emscripten)
#include <emscripten.h>
#endif

void MainLoop(bool enableStreaming)
{
#if(INTRA_PLATFORM_OS != INTRA_PLATFORM_OS_Emscripten)
	ConsoleOut.PrintLine("Нажмите любую клавишу, чтобы закрыть...");
	Thread thr;
	if(enableStreaming)
	{
		thr = Thread("Sound Streamer", []()
		{
			while(!ThisThread.IsInterrupted())
			{
				StreamedSound::UpdateAllExistingInstances();
				ThisThread.Sleep(1);
			}
		});
	}
	ConsoleIn.GetChar();

#else
	(void)enableStreaming;
	emscripten_set_main_loop([]() {}, 30, 1);
#endif
}

void LoadAndPlaySound(StringView filePath, bool enableStreaming)
{
	StreamedSound streamedSound;
	static Sound sound;
	if(enableStreaming)
	{
		Std.PrintLine("Инициализация...");
		streamedSound = StreamedSound::FromFile(filePath, 16384, Error::Skip());
		streamedSound.Play();
	}
	else
	{
		sound = SynthSoundFromMidi(filePath, true);
		auto inst = sound.CreateInstance();
		inst.Play();
	}
	Std.PrintLine("Воспроизведение...");
	MainLoop(enableStreaming);
}

void PlayMusic(const Music& music, bool printPerf)
{
	Stopwatch sw;
	AudioBuffer buf = music.GetSamples();
	if(printPerf)
	{
		auto time = sw.ElapsedSeconds();
		Std.PrintLine("Время синтеза: ", StringOf(time*1000, 2), " мс.");
	}
	static Sound snd;
	snd = Sound(&buf);
	auto inst = snd.CreateInstance();
	inst.Play();
}

void PlayMusicStream(const Music& music)
{
	const auto sampleRate = StreamedSound::InternalSampleRate();
	Std.PrintLine("Частота дискретизации: ", sampleRate, " Гц");
	Std.PrintLine("Инициализация...");
	static StreamedSound sound;
	sound = StreamedSound(new Sources::MusicSynthSource(music, sampleRate));
	sound.Play();
}

#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Emscripten)

extern "C" EMSCRIPTEN_KEEPALIVE void PlayMidiFileInMemory(const byte* data, size_t size, bool enableStreaming)
{
	StreamedSound::DeleteAllSounds();
	Sound::DeleteAllSounds();

	auto music = ReadMidiFile({data, size});
	PrintMusicInfo(music);
	if(enableStreaming) PlayMusicStream(music);
	else PlayMusic(music, true);
	Std.PrintLine("Воспроизведение...");
}

extern "C" EMSCRIPTEN_KEEPALIVE void PlayUrl(const char* url, bool enableStreaming)
{
	Array<byte> bb = DownloadFile(StringView(url));
	PlayMidiFileInMemory(bb.Data(), bb.Length(), enableStreaming);
}

#endif


#if INTRA_DISABLED
void SoundTest()
{
	AudioBuffer buf(2000000, 44100);
	for(uint t=0; t < buf.Samples.Count(); t++)
		buf.Samples[t] = (byte( ( (((((t>>3)|(t>>7))*5)|(t>>4))&0xff)/4 + (((((t>>3)|(t>>12))*5)|(t>>7))&0xff)*3/4 ) )-128)/127.0f;
	Sound snd = Sound(&buf);
	snd.CreateInstance().Play();
}
#endif


static const StringView DefaultMidiName = "Merry Christmas.mid";

int INTRA_CRTDECL main()
{
#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Emscripten)
#ifdef ENABLE_STREAMING
	PlayUrl(("http://gammaker.github.io/midi/" + DefaultMidiName).CStr(), true);
#else
	PlayUrl(("http://gammaker.github.io/midi/" + DefaultMidiName).CStr(), false);
#endif
	MainLoop(false);
#else
	//System::InitSignals();

#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
	SetConsoleCtrlHandler(ConsoleCloseHandler, true);
#endif

	const String filePath = GetMidiPath(Environment.CommandLine.Get(1, DefaultMidiName));
	
	const bool success = PrintMidiFileInfo(filePath);
	if(!success) return 1;
#ifdef ENABLE_STREAMING
	LoadAndPlaySound(filePath, true);
#else
	LoadAndPlaySound(filePath, false);
#endif

#endif
	CleanUpSoundSystem();
	return 0;
}
