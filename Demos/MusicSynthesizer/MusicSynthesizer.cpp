#include "Cpp/PlatformDetect.h"

#include "System/Stopwatch.h"
#include "System/Environment.h"

#include "Range/Polymorphic/InputRange.h"
#include "Range/Polymorphic/ForwardRange.h"

#include "IO/ConsoleOutput.h"
#include "IO/ConsoleInput.h"
#include "IO/FileSystem.h"
#include "IO/FileReader.h"
#include "IO/FileWriter.h"
#include "IO/Networking.h"
#include "IO/Std.h"

#include "Concurrency/Thread.h"

#include "Audio/AudioBuffer.h"
#include "Audio/Sound.h"
#include "Audio/AudioSource.h"

#include "Audio/Sources/Wave.h"


#include "MusicSynthesizerCommon.h"
#include "MidiInstrumentMapping.h"

#include "Math/SineRange.h"
#include "Audio/Resample.h"
#include "Audio/Synth/WaveTableGeneration.h"
#include "Audio/Synth/InstrumentSet.h"

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

void PrintInfoAndPlayMidiStream(ForwardStream stream, bool enableStreaming)
{
	FatalErrorStatus status;
	auto info = PrintMidiInfo(stream, status);
	if(status.Handle())
	{
		Std.PrintLine(status.GetLog());
		ConsoleIn.GetChar();
		return;
	}

	Std.PrintLine("Частота дискретизации: ", Sound::DefaultSampleRate(), " Гц");
	if(enableStreaming) CreateStreamedSoundFromMidi(Cpp::Move(stream), 0.5f, true).Play();
	else CreateSoundFromMidi(Cpp::Move(stream), info.Duration, 0.5f, true).CreateInstance().Play();
	Std.PrintLine("Воспроизведение...");
	MainLoop(enableStreaming);
}

void PrintInfoAndPlayMidiFile(StringView filePath, bool enableStreaming)
{
	Std.PrintLine("Открытие MIDI файла: ", filePath);
	PrintInfoAndPlayMidiStream(OS.FileOpen(filePath, Error::Skip()), enableStreaming);
}

void PrintInfoAndConvertMidiStreamToWavStream(ForwardStream midiStream, uint sampleRate, OutputStream wavStream)
{
	FatalErrorStatus status;
	auto info = PrintMidiInfo(midiStream, status);
	if(status.Handle())
	{
		Std.PrintLine(status.GetLog());
		ConsoleIn.GetChar();
		return;
	}
	Std.PrintLine("Синтез...");
	Stopwatch sw;
	auto src = CreateMidiAudioSource(Cpp::Move(midiStream), info.Duration, 0.75f, status, sampleRate);
	Sources::WriteWave(*src, wavStream);
	Std.PrintLine("Время синтеза: ", StringOf(sw.ElapsedSeconds()*1000, 2), " мс.");
}

void PrintInfoAndConvertMidiFileToWav(StringView filePath, StringView outputPath)
{
	Std.PrintLine("Открытие MIDI файла: ", filePath);
	PrintInfoAndConvertMidiStreamToWavStream(OS.FileOpen(filePath, Error::Skip()), 48000, OS.FileOpenOverwrite(outputPath, Error::Skip()));
}

#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Emscripten)

extern "C" EMSCRIPTEN_KEEPALIVE void PlayMidiFileInMemory(const byte* data, size_t size, bool enableStreaming)
{
	StreamedSound::ReleaseAllSounds();
	Sound::ReleaseAllSounds();

	PrintInfoAndPlayMidiStream(SpanOfRaw<const char>(data, size), enableStreaming);
}

extern "C" EMSCRIPTEN_KEEPALIVE void PlayUrl(const char* url, bool enableStreaming)
{
	Array<byte> bb = DownloadFile(StringView(url));
	PlayMidiFileInMemory(bb.Data(), bb.Length(), enableStreaming);
}

#endif


//#if INTRA_DISABLED
void SoundTest()
{
	AudioBuffer buf(2000000, 44100);
	for(uint t=0; t < buf.Samples.Count(); t++)
		buf.Samples[t] = (Intra::byte( ( (((((t>>3)|(t>>7))*5)|(t>>4))&0xff)/4 + (((((t>>3)|(t>>12))*5)|(t>>7))&0xff)*3/4 ) )-128)/127.0f;
	Sound snd = Sound(buf);
	snd.CreateInstance().Play();
}

void SoundTest2()
{
	float rate = 1;
	AudioBuffer buf(size_t(240000/rate), 48000);
	Array<float> arr(240000);
	Math::SineRange<float> sine(0.9f, 0, float(2*Math::PI*440/rate/buf.SampleRate));
	Range::ReadTo(sine, arr);
	ResampleLinear(arr, buf.Samples);
	Sound snd = Sound(buf);
	snd.CreateInstance().Play();
	Std.PrintLine("Playing...");
	Sleep(5000);
}

void SoundTest3()
{
	Synth::WaveTable tbl;
	for(int q = 0; q<1; q++)
	{
		tbl.BaseLevelLength = 32768;
		tbl.Data.Reserve(tbl.BaseLevelLength * 2);
		tbl.Data.SetCount(tbl.BaseLevelLength / 2);
		float freq = 261;
		tbl.BaseLevelRatio = freq/44100;
		//tbl.Data[size_t(tbl.BaseLevelRatio*tbl.BaseLevelLength)] = 0.5f;
		//Synth::AddSineHarmonicGaussianProfile(tbl.Data, tbl.BaseLevelRatio, 1, 50);
		for(int i = 1; i<64; i++)
		{
			using namespace Math;
			float ampl = 1.0f/i;
			float formants = Exp(-Sqr((i*freq - 600) / 150)) +
				Exp(-Sqr((i*freq - 900) / 250)) +
				Exp(-Sqr((i*freq - 2200) / 200)) +
				Exp(-Sqr((i*freq - 2600) / 250)) +
				Exp(-Sqr((i*freq) / 3000)) / 10;
			ampl *= formants;
			Synth::AddSineHarmonicGaussianProfile(tbl.Data, tbl.BaseLevelRatio, float(i+1), 1, ampl, 60);
		}
		//for(size_t i = 0; i<64; i++)
		//Synth::AddSineHarmonicGaussianProfile(tbl.Data, tbl.BaseLevelRatio*2, 1, 100);
		//Synth::AddSineHarmonicGaussianProfile(tbl.Data, tbl.BaseLevelRatio*3, 1, 100);
		ConvertAmplitudesToSamples(tbl);
	}
	Sources::Wave src(null, 44100, 1, tbl.LevelSamples(0));
	Sound snd = Sound(src);
	snd.CreateInstance().Play(true);
	Std.PrintLine("Playing...");
	Sleep(80000);
}
//#endif

int INTRA_CRTDECL main()
{
	//SoundTest2();
	//SoundTest3();
#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Emscripten)
#else
	//System::InitSignals();

#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
	SetConsoleCtrlHandler(ConsoleCloseHandler, true);
#endif

	const StringView DefaultMidiPath = "Merry Christmas.mid";
	const String filePath = GetMidiPath(Environment.CommandLine.Get(1, DefaultMidiPath));

	if(Environment.CommandLine.Length() >= 3)
	{
		PrintInfoAndConvertMidiFileToWav(filePath, Environment.CommandLine[2]);
		return 0;
	}
	
	const bool enableStreaming = false;
	PrintInfoAndPlayMidiFile(filePath, enableStreaming);

#endif
	CleanUpSoundSystem();
	return 0;
}
