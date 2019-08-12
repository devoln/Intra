#pragma once

#include "Audio/Sound.h"
#include "Audio/SoundTypes.h"
#include "Audio/AudioSource.h"

#include "Container/Sequential/List.h"

#include "Concurrency/Mutex.h"
#include "Concurrency/Synchronized.h"

INTRA_BEGIN
namespace Audio {

namespace detail {

struct SoundBasicContext
{
#if(INTRA_LIBRARY_MUTEX != INTRA_LIBRARY_MUTEX_None)
	Mutex MyMutex;
#endif
	Array<Sound::Data*> AllSounds;
	Array<StreamedSound::Data*> AllStreamedSounds;

	SoundBasicContext() = default;
	SoundBasicContext(const SoundBasicContext&) = delete;
	SoundBasicContext& operator=(const SoundBasicContext&) = delete;
};

struct SoundBasicData
{
	SoundBasicData(IAudioSource& src):
		Info(src.SamplesLeft(), src.SampleRate(), ushort(src.ChannelCount()), Data::ValueType::Void) {}

#if(INTRA_LIBRARY_MUTEX != INTRA_LIBRARY_MUTEX_None)
	Mutex MyMutex;
#endif

	SoundInfo Info;

	SoundBasicData(const SoundBasicData&) = delete;
	SoundBasicData& operator=(const SoundBasicData&) = delete;
};

struct SoundInstanceBasicData
{
	SoundInstanceBasicData(Shared<Sound::Data> parent): Parent(Move(parent)) {}

#if(INTRA_LIBRARY_MUTEX != INTRA_LIBRARY_MUTEX_None)
	Mutex MyMutex;
#endif
	Shared<Sound::Data> Parent;
	Shared<Sound::Instance::Data> SelfRef;
	Delegate<void()> OnStop;

	SoundInstanceBasicData(const SoundInstanceBasicData&) = delete;
	SoundInstanceBasicData& operator=(const SoundInstanceBasicData&) = delete;
};

struct StreamedSoundBasicData
{
	StreamedSoundBasicData(Unique<IAudioSource> source, size_t bufferSampleCount):
		Source(Move(source)), BufferSampleCount(bufferSampleCount) {}

	size_t GetBufferSize() {return Source->ChannelCount() * BufferSampleCount * sizeof(short);}

	Unique<IAudioSource> Source;
	Shared<StreamedSound::Data> SelfRef;
	size_t BufferSampleCount;
	StreamedSound::OnStopCallback OnStop;

	StreamedSoundBasicData(const StreamedSoundBasicData&) = delete;
	StreamedSoundBasicData& operator=(const StreamedSoundBasicData&) = delete;
};

}

}}

