﻿#include "IO/Stream.h"
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

#include "MusicSynthesizerCommon.h"

//#define ENABLE_STREAMING

#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
#ifdef _MSC_VER
#pragma warning(disable: 4668)
#endif
#ifndef WIN32_MEAN_AND_LEAN
#define WIN32_MEAN_AND_LEAN
#include <windows.h>
#endif


BOOL WINAPI ConsoleCloseHandler(DWORD CtrlType)
{
	(void)CtrlType;
	Intra::CleanUpSoundSystem();
	exit(0);
}

#endif


using namespace Intra;
using namespace Intra::IO;


#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Emscripten)
#include <emscripten.h>
#endif

void MainLoop(bool enableStreaming)
{
#if(INTRA_PLATFORM_OS!=INTRA_PLATFORM_OS_Emscripten)

	Console.PrintLine("Нажмите любую клавишу, чтобы закрыть...");
	Thread thr;
	if(enableStreaming)
	{
		thr = Thread([]()
		{
			for(;;)
			{
				StreamedSound::UpdateAllExistingInstances();
				Timer::Wait(1);
			}		
		});
	}
	Console.GetChar();

#else
	(void)enableStreaming;
	emscripten_set_main_loop([]() {}, 30, 1);
#endif
}

void LoadAndPlaySound(StringView filePath, bool enableStreaming)
{
	if(enableStreaming)
	{
		Console.PrintLine("Инициализация...");
		static StreamedSound sound = StreamedSound::FromFile(filePath, 16384);
		sound.Play();
	}
	else
	{
		static Sound sound = SynthSoundFromMidi(filePath, true);
		auto inst = sound.CreateInstance();
		inst.Play();
	}
	Console.PrintLine("Воспроизведение...");
	MainLoop(enableStreaming);
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
	static Sound snd;
	snd = Sound(&buf);
	auto inst = snd.CreateInstance();
	inst.Play();
}

void PlayMusicStream(const Music& music)
{
	const auto sampleRate = StreamedSound::InternalSampleRate();
	Console.PrintLine("Частота дискретизации: ", sampleRate, " Гц");
	Console.PrintLine("Инициализация...");
	static StreamedSound sound;
	sound = StreamedSound(new MusicSoundSampleSource(music, sampleRate));
	sound.Play();
}

#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Emscripten)

extern "C" EMSCRIPTEN_KEEPALIVE void PlayUrl(const char* url, bool enableStreaming)
{
	StreamedSound::DeleteAllSounds();
	Sound::DeleteAllSounds();

	Array<byte> bb = DownloadFile(StringView(url));
	auto music = ReadMidiFile(bb.AsConstRange());
	PrintMusicInfo(music);
	if(enableStreaming) PlayMusicStream(music);
	else PlayMusic(music, true);
	Console.PrintLine("Воспроизведение...");
}

#endif


#if INTRA_DISABLED
void SoundTest()
{
	SoundBuffer buf(2000000, 44100);
	for(uint t=0; t<buf.Samples.Count(); t++)
		buf.Samples[t] = (byte( ( (((((t>>3)|(t>>7))*5)|(t>>4))&0xff)/4 + (((((t>>3)|(t>>12))*5)|(t>>7))&0xff)*3/4 ) )-128)/127.0f;
	Sound snd = Sound(&buf);
	snd.CreateInstance().Play();
}
#endif


static const StringView DefaultMidiName = "ABBA-Mamma_Mia.mid";

int INTRA_CRTDECL main()
{
#if(INTRA_PLATFORM_OS!=INTRA_PLATFORM_OS_Emscripten)

#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Emscripten)
#ifdef ENABLE_STREAMING
	PlayUrl(("http://gammaker.github.io/midi/"+DefaultMidiName).CStr(), true);
#else
	PlayUrl(("http://gammaker.github.io/midi/"+DefaultMidiName).CStr(), false);
#endif
	MainLoop(false);
#else
	//Errors::InitSignals();

#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
	SetConsoleCtrlHandler(ConsoleCloseHandler, true);
#endif

	auto args = GetCommandLineArguments();
	String filePath = GetMidiPath(args.Length()>=2? args[1]: DefaultMidiName);
	
	bool success = PrintMidiFileInfo(filePath);
	if(!success) return 1;
#ifdef ENABLE_STREAMING
	LoadAndPlaySound(filePath, true);
#else
	LoadAndPlaySound(filePath, false);
#endif

#endif
	CleanUpSoundSystem();

#endif
	return 0;
}
