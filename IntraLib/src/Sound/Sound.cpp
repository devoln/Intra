#include "Sound/Sound.h"
#include "IO/File.h"
#include "Sound/SoundBuilder.h"
#include "Sound/SoundApi.h"
#include "Sound/SoundSource.h"
#include "Sound/Music.h"
#include "Sound/Midi.h"


namespace Intra {

using namespace Math;
using namespace IO;



Sound::Sound(const SoundInfo& bufferInfo, const void* initData):
	data(SoundAPI::BufferCreate(bufferInfo.SampleCount, bufferInfo.Channels, bufferInfo.SampleRate)),
	instances(),
	locked_bits(null),
	info(bufferInfo),
	locked_size(0)
{
	if(initData!=null) SoundAPI::BufferSetDataInterleaved(data, initData, info.SampleType);
}

Sound::Sound(const SoundBuffer* dataBuffer):
	data(null),
	instances(),
	locked_bits(null),
	info(dataBuffer==null? SoundInfo(): dataBuffer->Info()),
	locked_size(0)
{
	data = SoundAPI::BufferCreate(info.SampleCount, info.Channels, info.SampleRate);
	SoundAPI::BufferSetDataInterleaved(data, dataBuffer->Samples.begin(), info.SampleType);
	info.SampleType = ValueType::Short;
}

Sound::~Sound() {Release();}

AnyPtr Sound::Lock()
{
	return SoundAPI::BufferLock(data);
}

void Sound::Unlock()
{
	SoundAPI::BufferUnlock(data);
}

void Sound::Release()
{
	if(data==null) return;
	while(!instances.Empty()) instances[0]->Release();
	SoundAPI::BufferDelete(data);
	data=null;
}

Sound& Sound::operator=(Sound&& rhs)
{
	Release();
	data = rhs.data;
	locked_bits = rhs.locked_bits;
	locked_size = rhs.locked_size;
	instances = core::move(rhs.instances);
	for(auto inst: instances)
		inst->my_sound = this;
	info = rhs.info;
	rhs.data = null;
	return *this;
}

Sound Sound::FromFile(StringView fileName)
{
	Sound result=null;
	DiskFile::Reader file(fileName);
	if(file==null) return result;
	auto fileData = file.Map<byte>();
	ASoundSampleSource* source=null;

#ifndef INTRA_NO_WAVE_LOADER
	if(fileData.StartsWith(StringView("RIFF")))
		source = new WaveSoundSampleSource(fileData);
	else
#endif
#if(INTRA_LIBRARY_VORBIS_DECODER!=INTRA_LIBRARY_VORBIS_DECODER_None)
	if(fileData.StartsWith(StringView("OggS")))
		source = new VorbisSoundSampleSource(fileData);
	else 
#endif
#ifndef INTRA_NO_MUSIC_LOADER
	if(fileData.StartsWith(StringView("MThd")))
		source = new MusicSoundSampleSource(ReadMidiFile(fileData), 44100);
	else
#endif
		return null;

	result = FromSource(source);
	delete source;
	file.Unmap();
	return result;
}

Sound Sound::FromSource(ASoundSampleSource* src)
{
	Sound result = Sound({src->SampleCount(), src->SampleRate(), ushort(src->ChannelCount()), SoundAPI::InternalBufferType}, null);
	auto lockedData = result.Lock();
	if(SoundAPI::InternalBufferType==ValueType::Short && (SoundAPI::InternalChannelsInterleaved || result.Info().Channels==1))
	{
		ArrayRange<short> dst = ArrayRange<short>(lockedData, result.Info().SampleCount*src->ChannelCount());
		src->GetInterleavedSamples(dst);
	}
	else if(SoundAPI::InternalBufferType==ValueType::Float && (SoundAPI::InternalChannelsInterleaved || result.Info().Channels==1))
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
	INTRA_ASSERT(data!=null);
	return SoundInstance(this, SoundAPI::InstanceCreate(data));
}

SoundInstance::SoundInstance(Sound* mySound, SoundAPI::InstanceHandle inst):
	my_sound(mySound), data(inst)
{
	if(mySound==null) return;
	mySound->instances.AddLast(this);
}

void SoundInstance::Release()
{
	if(data!=null)
	{
		if(IsPlaying()) SoundAPI::InstanceSetDeleteOnStop(data, true);
		else SoundAPI::InstanceDelete(data);
		data=null;
	}
	my_sound->instances.FindAndRemoveUnordered(this);
	my_sound = null;
}

SoundInstance::~SoundInstance() {Release();}

void SoundInstance::Play(bool loop) const
{
	if(data==null) return;
	SoundAPI::InstancePlay(data, loop);
}

bool SoundInstance::IsPlaying() const
{
	if(data==null) return false;
	return SoundAPI::InstanceIsPlaying(data);
}

void SoundInstance::Stop() const
{
	if(data==null) return;
	SoundAPI::InstanceStop(data);
}



size_t StreamingLoadCallback(void** dstSamples, uint channels,
	ValueType::I type, bool interleaved, size_t sampleCount, void* additionalData)
{
	auto src = reinterpret_cast<ASoundSampleSource*>(additionalData);
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
	sample_source(core::move(src)), on_close(onClose), data(null)
{
	data = SoundAPI::StreamedBufferCreate(bufferSizeInSamples, sample_source->ChannelCount(),
		sample_source->SampleRate(), {StreamingLoadCallback, sample_source.ptr});
	register_instance();
}

StreamedSound StreamedSound::FromFile(StringView fileName, size_t bufSize)
{
	auto file = new DiskFile::Reader(fileName);
	if(*file==null) {delete file; return null;}
	auto fileData = file->Map<byte>();
	SourceRef source=null;
#ifndef INTRA_NO_WAVE_LOADER
	if(fileData.StartsWith(StringView("RIFF")))
		source = SourceRef(new WaveSoundSampleSource(fileData));
	else
#endif
#if(INTRA_LIBRARY_VORBIS_DECODER!=INTRA_LIBRARY_VORBIS_DECODER_None)
	if(fileData.StartsWith(StringView("OggS")))
		source = SourceRef(new VorbisSoundSampleSource(fileData));
	else 
#endif
#ifndef INTRA_NO_MUSIC_LOADER
	if(fileData.StartsWith(StringView("MThd")))
		source = SourceRef(new MusicSoundSampleSource(ReadMidiFile(fileData), 44100));
	else
#endif
	{
		file->Unmap();
		delete file;
		return null;
	}

	return StreamedSound(core::move(source), bufSize, StreamedSound::OnCloseCallback(file, [](void* o)
	{
		auto mappedFile = reinterpret_cast<DiskFile::Reader*>(o);
		mappedFile->Unmap();
		delete mappedFile;
	}));
}

void StreamedSound::release()
{
	if(on_close!=null) on_close();
	if(data==null) return;
	unregister_instance();
	SoundAPI::StreamedBufferDelete(data);
}

void StreamedSound::Play(bool loop) const
{
	if(data==null) return;
	SoundAPI::StreamedSoundPlay(data, loop);
}

bool StreamedSound::IsPlaying() const
{
	if(data==null) return false;
	return SoundAPI::StreamedSoundIsPlaying(data);
}

void StreamedSound::Stop() const
{
	if(data==null) return;
	SoundAPI::StreamedSoundStop(data);
}

void StreamedSound::UpdateBuffer() const
{
	if(data==null) return;
	SoundAPI::StreamedSoundUpdate(data);
}

void StreamedSound::register_instance()
{
	INTRA_ASSERT(!all_existing_instances().Contains(this));
	all_existing_instances.AddLast(this);
}

void StreamedSound::unregister_instance()
{
	INTRA_ASSERT(all_existing_instances().Contains(this));
	all_existing_instances.FindAndRemoveUnordered(this);
}

Array<StreamedSound*> StreamedSound::all_existing_instances;

void CleanUpSoundSystem()
{
	SoundAPI::SoundSystemCleanUp();
}

}
