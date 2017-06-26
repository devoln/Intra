#pragma once

#include "Cpp/PlatformDetect.h"
#include "Data/ValueType.h"
#include "SoundApiDeclarations.h"

//! Используемая библиотека для вывода звука
#define INTRA_LIBRARY_SOUND_SYSTEM_Dummy 0
#define INTRA_LIBRARY_SOUND_SYSTEM_DirectSound 1
#define INTRA_LIBRARY_SOUND_SYSTEM_OpenAL 2
#define INTRA_LIBRARY_SOUND_SYSTEM_SDL 3
#define INTRA_LIBRARY_SOUND_SYSTEM_Qt 4
#define INTRA_LIBRARY_SOUND_SYSTEM_WebAudio 5
#define INTRA_LIBRARY_SOUND_SYSTEM_ALSA 6

//Пытаемся автоматически определить библиотеку для вывода звука
#ifndef INTRA_LIBRARY_SOUND_SYSTEM

#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
#define INTRA_LIBRARY_SOUND_SYSTEM INTRA_LIBRARY_SOUND_SYSTEM_DirectSound
#elif(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Emscripten)
#define INTRA_LIBRARY_SOUND_SYSTEM INTRA_LIBRARY_SOUND_SYSTEM_WebAudio
#else
#define INTRA_LIBRARY_SOUND_SYSTEM INTRA_LIBRARY_SOUND_SYSTEM_OpenAL
#endif

#endif

namespace Intra { namespace Audio { namespace SoundAPI {

extern const Data::ValueType::I InternalBufferType;
extern const int InternalChannelsInterleaved;

uint InternalSampleRate();

BufferHandle BufferCreate(size_t sampleCount, uint channels, uint sampleRate);
void BufferSetDataInterleaved(BufferHandle snd, const void* data, Data::ValueType type);
void BufferSetDataChannels(BufferHandle snd, const void* const* data, Data::ValueType type);
void* BufferLock(BufferHandle snd);
void BufferUnlock(BufferHandle snd);
void BufferDelete(BufferHandle snd);
InstanceHandle InstanceCreate(BufferHandle snd);
void InstanceDelete(InstanceHandle si);
void InstancePlay(InstanceHandle si, bool loop);
bool InstanceIsPlaying(InstanceHandle si);
void InstanceStop(InstanceHandle si);
void InstanceSetDeleteOnStop(InstanceHandle snd, bool del);


struct StreamingCallback
{
	typedef size_t(*CallbackFunctionType)(void** dstSamples, uint channels,
		Data::ValueType::I type, bool interleaved, size_t sampleCount, void* additionalData);

	CallbackFunctionType CallbackFunction;
	void* CallbackData;
};

StreamedBufferHandle StreamedBufferCreate(size_t sampleCount,
	uint channels, uint sampleRate, StreamingCallback callback);
void StreamedBufferSetDeleteOnStop(StreamedBufferHandle snd, bool del);
void StreamedBufferDelete(StreamedBufferHandle snd);
void StreamedSoundPlay(StreamedBufferHandle snd, bool loop);
bool StreamedSoundIsPlaying(StreamedBufferHandle snd);
void StreamedSoundStop(StreamedBufferHandle snd);
void StreamedSoundUpdate(StreamedBufferHandle snd);

void SoundSystemCleanUp();

}}}
