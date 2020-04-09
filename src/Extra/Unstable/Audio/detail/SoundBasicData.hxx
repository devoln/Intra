#pragma once

#include "Extra/Unstable/Audio/Sound.h"
#include "Extra/Unstable/Audio/SoundTypes.h"
#include "Extra/Unstable/Audio/AudioSource.h"

#include "Extra/Container/Sequential/List.h"

#include "Extra/Concurrency/Mutex.h"
#include "Extra/Concurrency/Synchronized.h"

INTRA_BEGIN
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
		Info(src.SamplesLeft().Unwrap(), src.SampleRate(), short(src.ChannelCount()), ValueType::Void) {}

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
	StreamedSoundBasicData(Unique<IAudioSource> source, index_t bufferSampleCount):
		Source(Move(source)), BufferSampleCount(bufferSampleCount) {}

	auto GetBufferSize() {return size_t(Source->ChannelCount()) * size_t(BufferSampleCount) * sizeof(short);}

	Unique<IAudioSource> Source;
	Shared<StreamedSound::Data> SelfRef;
	index_t BufferSampleCount;
	StreamedSound::OnStopCallback OnStop;

	StreamedSoundBasicData(const StreamedSoundBasicData&) = delete;
	StreamedSoundBasicData& operator=(const StreamedSoundBasicData&) = delete;
};

}

INTRA_END
