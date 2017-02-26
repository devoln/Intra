#include "Audio/Sound.h"
#include "IO/File.h"
#include "Audio/AudioBuffer.h"
#include "Audio/SoundApi.h"
#include "Audio/AudioSource.h"
#include "Audio/Music.h"
#include "Audio/Midi.h"
#include "Audio/Sources/WaveSource.h"
#include "Audio/Sources/VorbisSource.h"
#include "Audio/Sources/MusicSynthSource.h"
#include "Algo/Comparison/StartsWith.h"
#include "Algo/Search/Single.h"
#include "Platform/CppWarnings.h"


namespace Intra { namespace Audio {

using namespace Intra::Math;
using namespace Intra::IO;

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

Array<Sound*> Sound::all_existing_sounds;

Sound::Sound(const SoundInfo& bufferInfo, const void* initData):
	mData(SoundAPI::BufferCreate(bufferInfo.SampleCount, bufferInfo.Channels, bufferInfo.SampleRate)),
	mInstances(),
	mLockedBits(null),
	mInfo(bufferInfo),
	mLockedSize(0)
{
	if(initData!=null) SoundAPI::BufferSetDataInterleaved(mData, initData, mInfo.SampleType);
	all_existing_sounds.AddLast(this);
}

Sound::Sound(const AudioBuffer* dataBuffer):
	mData(null),
	mInstances(),
	mLockedBits(null),
	mInfo(dataBuffer==null? SoundInfo(): dataBuffer->Info()),
	mLockedSize(0)
{
	if(dataBuffer==null) return;
	mData = SoundAPI::BufferCreate(mInfo.SampleCount, mInfo.Channels, mInfo.SampleRate);
	SoundAPI::BufferSetDataInterleaved(mData, dataBuffer->Samples.Data(), mInfo.SampleType);
	mInfo.SampleType = ValueType::Short;
	all_existing_sounds.AddLast(this);
}

Sound::~Sound() {Release();}

AnyPtr Sound::Lock()
{
	return SoundAPI::BufferLock(mData);
}

void Sound::Unlock()
{
	SoundAPI::BufferUnlock(mData);
}

void Sound::Release()
{
	if(mData==null) return;
	Sound::all_existing_sounds.FindAndRemoveUnordered(this);
	while(!mInstances.Empty()) mInstances.Last()->Release(true);
	SoundAPI::BufferDelete(mData);
	mData = null;
	mInfo = SoundInfo();
}

Sound& Sound::operator=(Sound&& rhs)
{
	Release();
	mData = rhs.mData;
	mLockedBits = rhs.mLockedBits;
	mLockedSize = rhs.mLockedSize;
	mInstances = Meta::Move(rhs.mInstances);
	for(auto inst: mInstances)
		inst->mSound = this;
	mInfo = rhs.mInfo;
	rhs.mData = null;
	auto found = Algo::Find(Sound::all_existing_sounds, &rhs);
	if(!found.Empty()) found.First() = this;
	return *this;
}

Sound Sound::FromFile(StringView fileName)
{
	Sound result=null;
	DiskFile::Reader file(fileName);
	if(file==null) return result;
	auto fileData = file.Map<byte>();
	ASoundSource* source=null;

#ifndef INTRA_NO_WAVE_LOADER
	if(Algo::StartsWith(fileData.Reinterpret<char>(), StringView("RIFF")))
		source = new Sources::WaveSource(fileData);
	else
#endif
#if(INTRA_LIBRARY_VORBIS_DECODER!=INTRA_LIBRARY_VORBIS_DECODER_None)
	if(Algo::StartsWith(fileData.Reinterpret<char>(), "OggS"))
		source = new Sources::VorbisSource(fileData);
	else 
#endif
#ifndef INTRA_NO_MUSIC_LOADER
	if(Algo::StartsWith(fileData.Reinterpret<char>(), StringView("MThd")))
		source = new Sources::MusicSynthSource(ReadMidiFile(fileData), SoundAPI::InternalSampleRate());
	else
#endif
		return null;

	result = FromSource(source);
	delete source;
	file.Unmap();
	return result;
}

Sound Sound::FromSource(ASoundSource* src)
{
	Sound result = Sound({src->SampleCount(), src->SampleRate(), ushort(src->ChannelCount()), SoundAPI::InternalBufferType}, null);
	auto lockedData = result.Lock();
	if(SoundAPI::InternalBufferType==ValueType::Short &&
		(SoundAPI::InternalChannelsInterleaved || result.Info().Channels==1))
	{
		ArrayRange<short> dst = ArrayRange<short>(lockedData, result.Info().SampleCount*src->ChannelCount());
		src->GetInterleavedSamples(dst);
	}
	else if(SoundAPI::InternalBufferType==ValueType::Float &&
		(SoundAPI::InternalChannelsInterleaved || result.Info().Channels==1))
	{
		ArrayRange<float> dst = ArrayRange<float>(lockedData, result.Info().SampleCount*src->ChannelCount());
		src->GetInterleavedSamples(dst);
	}
	//else if(SoundAPI::InternalBufferType==ValueType::Float && !SoundAPI::InternalChannelsInterleaved)
		//src->GetUninterleavedSamples(ArrayRange<float>(lockedData, result.SampleCount*src->ChannelCount()));
	result.Unlock();
	return result;
}



SoundInstance Sound::CreateInstance()
{
	INTRA_DEBUG_ASSERT(mData!=null);
	return SoundInstance(this, SoundAPI::InstanceCreate(mData));
}

SoundInstance::SoundInstance(Sound* mySound, SoundAPI::InstanceHandle inst):
	mSound(mySound), mData(inst)
{
	if(mSound==null) return;
	mSound->mInstances.AddLast(this);
}

void SoundInstance::Release(bool force)
{
	if(mData!=null)
	{
		if(IsPlaying() && !force) SoundAPI::InstanceSetDeleteOnStop(mData, true);
		else SoundAPI::InstanceDelete(mData);
		mData = null;
	}
	mSound->mInstances.FindAndRemoveUnordered(this);
	mSound = null;
}

SoundInstance::~SoundInstance() {Release();}

void SoundInstance::Play(bool loop) const
{
	if(mData==null) return;
	SoundAPI::InstancePlay(mData, loop);
}

bool SoundInstance::IsPlaying() const
{
	if(mData==null) return false;
	return SoundAPI::InstanceIsPlaying(mData);
}

void SoundInstance::Stop() const
{
	if(mData==null) return;
	SoundAPI::InstanceStop(mData);
}



size_t StreamingLoadCallback(void** dstSamples, uint channels,
	ValueType::I type, bool interleaved, size_t sampleCount, void* additionalData)
{
	auto src = reinterpret_cast<ASoundSource*>(additionalData);
	if(interleaved || channels==1)
	{
		if(type==ValueType::Float)
			return src->GetInterleavedSamples({reinterpret_cast<float*>(dstSamples[0]), sampleCount*src->ChannelCount()});
		if(type==ValueType::Short)
			return src->GetInterleavedSamples({reinterpret_cast<short*>(dstSamples[0]), sampleCount*src->ChannelCount()});
	}
	else
	{
		if(type==ValueType::Float)
		{
			ArrayRange<float> ranges[16];
			for(ushort c=0; c<channels; c++)
				ranges[c] = ArrayRange<float>(reinterpret_cast<float*>(dstSamples[c]), sampleCount);
			return src->GetUninterleavedSamples({ranges, channels});
		}
		if(type==ValueType::Short)
		{
			/*ArrayRange<short> ranges[16];
			for(ushort c=0; c<channels; c++)
				ranges[c] = ArrayRange<short>(reinterpret_cast<short*>(dstSamples[c]), sampleCount);
			return src->GetUninterleavedSamples({ranges, channels});*/
		}
	}
	return 0;
}


StreamedSound::StreamedSound(SourceRef&& src, size_t bufferSizeInSamples, OnCloseCallback onClose):
	mSampleSource(Meta::Move(src)), mOnClose(onClose), mData(null)
{
	mData = SoundAPI::StreamedBufferCreate(bufferSizeInSamples, mSampleSource->ChannelCount(),
		mSampleSource->SampleRate(), {StreamingLoadCallback, mSampleSource.Ptr()});
	register_instance();
}

StreamedSound StreamedSound::FromFile(StringView fileName, size_t bufSize)
{
	auto file = new DiskFile::Reader(fileName);
	if(*file==null) {delete file; return null;}
	auto fileData = file->Map<byte>();
	SourceRef source=null;
#ifndef INTRA_NO_WAVE_LOADER
	if(Algo::StartsWith(fileData.Reinterpret<char>(), "RIFF"))
		source = SourceRef(new Sources::WaveSource(fileData));
	else
#endif
#if(INTRA_LIBRARY_VORBIS_DECODER!=INTRA_LIBRARY_VORBIS_DECODER_None)
	if(Algo::StartsWith(fileData.Reinterpret<char>(), "OggS"))
		source = SourceRef(new Sources::VorbisSource(fileData));
	else 
#endif
#ifndef INTRA_NO_MUSIC_LOADER
	if(Algo::StartsWith(fileData.Reinterpret<char>(), "MThd"))
		source = SourceRef(new Sources::MusicSynthSource(ReadMidiFile(fileData), 48000));
	else
#endif
	{
		file->Unmap();
		delete file;
		return null;
	}

	return StreamedSound(Meta::Move(source), bufSize, StreamedSound::OnCloseCallback(file, [](void* o)
	{
		auto mappedFile = reinterpret_cast<DiskFile::Reader*>(o);
		mappedFile->Unmap();
		delete mappedFile;
	}));
}

void StreamedSound::release()
{
	if(mOnClose!=null) mOnClose();
	if(mData==null) return;
	unregister_instance();
	SoundAPI::StreamedBufferDelete(mData);
}

void StreamedSound::Play(bool loop) const
{
	if(mData==null) return;
	SoundAPI::StreamedSoundPlay(mData, loop);
}

bool StreamedSound::IsPlaying() const
{
	if(mData==null) return false;
	return SoundAPI::StreamedSoundIsPlaying(mData);
}

void StreamedSound::Stop() const
{
	if(mData==null) return;
	SoundAPI::StreamedSoundStop(mData);
}

void StreamedSound::UpdateBuffer() const
{
	if(mData==null) return;
	SoundAPI::StreamedSoundUpdate(mData);
}

uint StreamedSound::InternalSampleRate()
{
	return SoundAPI::InternalSampleRate();
}

void StreamedSound::register_instance()
{
	INTRA_DEBUG_ASSERT(!Algo::Contains(all_existing_instances, this));
	all_existing_instances.AddLast(this);
}

void StreamedSound::unregister_instance()
{
	INTRA_DEBUG_ASSERT(Algo::Contains(all_existing_instances, this));
	all_existing_instances.FindAndRemoveUnordered(this);
}

Array<StreamedSound*> StreamedSound::all_existing_instances;

void CleanUpSoundSystem()
{
	StreamedSound::DeleteAllSounds();
	Sound::DeleteAllSounds();
	SoundAPI::SoundSystemCleanUp();
}

INTRA_WARNING_POP

}}
