#pragma once

#include "Cpp/Warnings.h"

#include "Funal/Delegate.h"
#include "Utils/ErrorStatus.h"
#include "Utils/Shared.h"

#include "Container/Sequential/Array.h"
#include "Container/Utility/SparseHandledArray.h"

#include "Data/ValueType.h"

#include "Audio/SoundTypes.h"

//! Используемая библиотека для вывода звука
#define INTRA_LIBRARY_SOUND_None 0
#define INTRA_LIBRARY_SOUND_Dummy 1
#define INTRA_LIBRARY_SOUND_DirectSound 2
#define INTRA_LIBRARY_SOUND_OpenAL 3
#define INTRA_LIBRARY_SOUND_SDL 4
#define INTRA_LIBRARY_SOUND_Qt 5
#define INTRA_LIBRARY_SOUND_WebAudio 6
#define INTRA_LIBRARY_SOUND_ALSA 7

//Пытаемся автоматически определить библиотеку для вывода звука
#ifndef INTRA_LIBRARY_SOUND

#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
#define INTRA_LIBRARY_SOUND INTRA_LIBRARY_SOUND_DirectSound
#elif(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Emscripten)
#define INTRA_LIBRARY_SOUND INTRA_LIBRARY_SOUND_WebAudio
#else
#define INTRA_LIBRARY_SOUND INTRA_LIBRARY_SOUND_Dummy
#endif

#endif

#if(INTRA_LIBRARY_SOUND == INTRA_LIBRARY_SOUND_None)
#error Sound support is disabled, this file must not be included!
#endif

#if(INTRA_LIBRARY_SOUND != INTRA_LIBRARY_SOUND_OpenAL)
#define INTRA_LIBRARY_SOUND_AUTO_STREAMING_SUPPORTED
#endif

namespace Intra { namespace Audio {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

class Sound
{
public:
	class Instance;
	struct Data;

	Sound(null_t=null);
	explicit Sound(const AudioBuffer& data);
    Sound(const SoundInfo& bufferInfo, const void* initData=null);
	Sound(Sound&& rhs);
	Sound(IAudioSource& src, ErrorStatus& status);
	Sound(Unique<IAudioSource> src, ErrorStatus& status): Sound(*src, status) {}

	~Sound();

	void Release();

	static Sound FromFile(StringView fileName, ErrorStatus& status);

	Sound& operator=(Sound&& rhs);
	Sound& operator=(null_t) {Release(); return *this;}

	bool operator==(null_t) const {return mData == null;}
	bool operator!=(null_t) const {return !operator==(null);}

	const SoundInfo& Info() const;

	static uint DefaultSampleRate();

	Instance CreateInstance();

	static void ReleaseAllSounds();

private:
	Shared<Data> mData;

	Sound(const Sound&) = delete;
	Sound& operator=(const Sound&) = delete;
};

class Sound::Instance
{
	friend class Sound;
public:
	Instance(null_t = null);
	Instance(Instance&&);
	Instance& operator=(Instance&&);
	Instance(Sound& sound);

	~Instance();

	void Play(bool loop=false) const;
	bool IsPlaying() const;
	void Stop() const;
	void Release();

	struct Data;
private:
	Shared<Data> mData;

	Instance(const Instance&) = delete;
	Instance& operator=(const Instance&) = delete;
};

class StreamedSound
{
public:
	typedef Delegate<void()> OnStopCallback;

	StreamedSound(null_t=null);
	StreamedSound(Unique<IAudioSource> src, size_t bufferSizeInSamples=16384);
	StreamedSound(StreamedSound&& rhs);
	~StreamedSound();


	static StreamedSound FromFile(StringView fileName, size_t bufferSizeInSamples, ErrorStatus& status);

	static StreamedSound FromFile(StringView fileName, ErrorStatus& status)
	{return FromFile(fileName, 16384, status);}


	StreamedSound& operator=(StreamedSound&& rhs);

	StreamedSound& operator=(const StreamedSound& rhs) = delete;

	void Play(bool loop=false) const;
	bool IsPlaying() const;
	void Stop() const;

	void UpdateBuffer() const;

	static void UpdateAllExistingInstances();

	static void ReleaseAllSounds();

	struct Data;
private:
	Shared<Data> mData;

	StreamedSound(const StreamedSound&) = delete;

	void release();
};

void CleanUpSoundSystem();

INTRA_WARNING_POP

}}
