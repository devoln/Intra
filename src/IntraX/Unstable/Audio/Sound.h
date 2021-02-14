﻿#pragma once

#include "IntraX/Utils/Delegate.h"
#include "IntraX/System/Error.h"
#include "IntraX/Utils/Shared.h"

#include "IntraX/Container/Sequential/Array.h"
#include "IntraX/Container/Utility/SparseHandledArray.h"

#include "IntraX/Unstable/Data/ValueType.h"
#include "IntraX/Unstable/Audio/SoundTypes.h"

/// Используемая библиотека для вывода звука
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

#ifdef _WIN32
#define INTRA_LIBRARY_SOUND INTRA_LIBRARY_SOUND_DirectSound
#elif defined(__EMSCRIPTEN__)
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

namespace Intra { INTRA_BEGIN
class Sound
{
public:
	class Instance;
	struct Data;

	Sound(decltype(nullptr)=nullptr);
	explicit Sound(const AudioBuffer& data);
    Sound(const SoundInfo& bufferInfo, const void* initData=nullptr);
	Sound(Sound&& rhs);
	Sound(IAudioSource& src, ErrorReporter err);
	Sound(Unique<IAudioSource> src, ErrorReporter err): Sound(*src, err) {}

	~Sound();

	void Release();

	static Sound FromFile(StringView fileName, ErrorReporter err);

	Sound& operator=(Sound&& rhs);
	Sound& operator=(decltype(nullptr)) {Release(); return *this;}

	bool operator==(decltype(nullptr)) const {return mData == nullptr;}
	bool operator!=(decltype(nullptr)) const {return !operator==(nullptr);}

	const SoundInfo& Info() const;

	static unsigned DefaultSampleRate();

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
	Instance(decltype(nullptr) = nullptr);
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

	StreamedSound(decltype(nullptr)=nullptr);
	StreamedSound(Unique<IAudioSource> src, Size bufferSizeInSamples=16384);
	StreamedSound(StreamedSound&& rhs);
	~StreamedSound();


	static StreamedSound FromFile(StringView fileName, Size bufferSizeInSamples, ErrorReporter err);

	static StreamedSound FromFile(StringView fileName, ErrorReporter err)
	{return FromFile(fileName, 16384, err);}


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
} INTRA_END
