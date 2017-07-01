#include "Sound.h"
#include "AudioBuffer.h"
#include "AudioSource.h"
#include "Music.h"
#include "Midi.h"
#include "Sources/WaveSource.h"
#include "Sources/VorbisSource.h"
#include "Sources/MusicSynthSource.h"
#include "SoundTypes.h"

#include "Funal/ValueRef.h"

#include "Range/Comparison/StartsWith.h"
#include "Range/Search/Single.h"

#include "Cpp/Warnings.h"

#include "IO/FileSystem.h"

#if(INTRA_LIBRARY_SOUND == INTRA_LIBRARY_SOUND_DirectSound)
#include "detail/SoundDirectSound.hxx"
#elif(INTRA_LIBRARY_SOUND == INTRA_LIBRARY_SOUND_OpenAL)
#include "detail/SoundOpenAL.hxx"
#elif(INTRA_LIBRARY_SOUND == INTRA_LIBRARY_SOUND_ALSA)
#include "detail/SoundALSA.hxx"
#elif(INTRA_LIBRARY_SOUND == INTRA_LIBRARY_SOUND_WebAudio)
#include "detail/SoundWebAudio.hxx"
#elif(INTRA_LIBRARY_SOUND == INTRA_LIBRARY_SOUND_Qt)
#include "detail/SoundQt.hxx"
#elif(INTRA_LIBRARY_SOUND == INTRA_LIBRARY_SOUND_SDL)
#include "detail/SoundSDL.hxx"
#elif(INTRA_LIBRARY_SOUND == INTRA_LIBRARY_SOUND_Dummy)
#include "detail/SoundDummy.hxx"
#endif


INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio {

using namespace Math;
using namespace IO;
using Data::ValueType;

Sound::Sound(null_t) {}

Sound::Sound(const SoundInfo& bufferInfo, const void* initData):
	mData(Shared<Data>::New(bufferInfo))
{
	if(initData) mData->SetDataInterleaved(initData, bufferInfo.SampleType);
}

Sound::Sound(const AudioBuffer& dataBuffer)
{
	SoundInfo info(dataBuffer.Samples.Length(), dataBuffer.SampleRate, 1, ValueType::Short);
	mData = Shared<Data>::New(info);
	mData->SetDataInterleaved(dataBuffer.Samples.Data(), ValueType::Float);
}

Sound::Sound(Sound&& rhs): mData(Cpp::Move(rhs.mData)) {}

Sound::~Sound() {Release();}

AnyPtr Sound::Lock() {return mData->Lock();}
void Sound::Unlock() {mData->Unlock();}
void Sound::Release() {mData = null;}

Sound& Sound::operator=(Sound&& rhs) = default;

Sound Sound::FromFile(StringView fileName, ErrorStatus& status)
{
	auto fileMapping = OS.MapFile(fileName, status);
	if(fileMapping==null) return null;

	auto fileSignature = fileMapping.AsRangeOf<char>();

	Unique<AAudioSource> source;

#ifndef INTRA_NO_WAVE_LOADER
	if(fileSignature.StartsWith("RIFF"))
		source = new Sources::WaveSource(fileMapping.AsRange());
	else
#endif
#if(INTRA_LIBRARY_VORBIS_DECODER!=INTRA_LIBRARY_VORBIS_DECODER_None)
	if(fileSignature.StartsWith("OggS"))
		source = new Sources::VorbisSource(fileData);
	else
#endif
#ifndef INTRA_NO_MUSIC_LOADER
	if(fileSignature.StartsWith("MThd"))
		source = new Sources::MusicSynthSource(ReadMidiFile(fileMapping.AsRange(), status), DefaultSampleRate());
	else
#endif
	{
		status.Error("Unsupported audio format of file " + fileName + "!", INTRA_SOURCE_INFO);
		return null;
	}

	return FromSource(*source);
}

Sound Sound::FromSource(AAudioSource& src)
{
	INTRA_WARNING_DISABLE_UNREACHABLE_CODE
	INTRA_WARNING_DISABLE_CONSTANT_CONDITION

	const SoundInfo info = {src.SampleCount(), src.SampleRate(),
		ushort(src.ChannelCount()), InternalBufferType};

	Sound result = Sound(info, null);
	void* lockedData = result.Lock();
	INTRA_FINALLY(result.Unlock());
	if(InternalBufferType == ValueType::Short &&
		(InternalChannelsInterleaved || info.Channels == 1))
	{
		Span<short> dst = SpanOfRawElements<short>(lockedData, info.SampleCount*src.ChannelCount());
		src.GetInterleavedSamples(dst);
		return result;
	}
	if(InternalBufferType == ValueType::Float &&
		(InternalChannelsInterleaved || info.Channels == 1))
	{
		Span<float> dst = SpanOfRawElements<float>(lockedData, info.SampleCount*src.ChannelCount());
		src.GetInterleavedSamples(dst);
		return result;
	}
	//else if(InternalBufferType == ValueType::Float && !InternalChannelsInterleaved)
		//src.GetUninterleavedSamples(SpanOfRawElements<float>(lockedData, result.SampleCount*src.ChannelCount()));
	return result;
}

void Sound::ReleaseAllSounds()
{
	SoundContext::Instance().ReleaseAllSounds();
}

Sound::Instance Sound::CreateInstance()
{
	INTRA_DEBUG_ASSERT(mData != null);
	return Instance(*this);
}

Sound::Instance::Instance(Sound& sound):
	mData(Shared<Data>::New(sound.mData)) {}

Sound::Instance::Instance(null_t) {}
Sound::Instance::Instance(Instance&&) = default;
Sound::Instance& Sound::Instance::operator=(Instance&&) = default;

void Sound::Instance::Release() {mData = null;}

Sound::Instance::~Instance() {Release();}

void Sound::Instance::Play(bool loop) const {if(mData) mData->Play(loop);}
bool Sound::Instance::IsPlaying() const {return mData && mData->IsPlaying();}
void Sound::Instance::Stop() const {if(mData) mData->Stop();}

StreamedSound::StreamedSound(null_t) {}

StreamedSound::StreamedSound(StreamedSound&&) = default;

StreamedSound::StreamedSound(SourceRef&& src, size_t bufferSizeInSamples):
	mData(Shared<Data>::New(Cpp::Move(src), bufferSizeInSamples)) {}

StreamedSound::~StreamedSound() {release();}

StreamedSound& StreamedSound::operator=(StreamedSound&& rhs) = default;

StreamedSound StreamedSound::FromFile(StringView fileName, size_t bufSize, ErrorStatus& status)
{
	auto fileMapping = OS.MapFile(fileName, status);
	if(status.WasError()) return null;
	SourceRef source = null;
#ifndef INTRA_NO_WAVE_LOADER
	if(fileMapping.AsRangeOf<char>().StartsWith("RIFF"))
		source = new Sources::WaveSource(fileMapping.AsRange());
	else
#endif
#if(INTRA_LIBRARY_VORBIS_DECODER != INTRA_LIBRARY_VORBIS_DECODER_None)
	if(fileMapping.AsRangeOf<char>().StartsWith("OggS"))
		source = new Sources::VorbisSource(fileMapping.AsRange());
	else 
#endif
#ifndef INTRA_NO_MUSIC_LOADER
	if(fileMapping.AsRangeOf<char>().StartsWith("MThd"))
		source = new Sources::MusicSynthSource(ReadMidiFile(fileMapping.AsRange(), status), 48000);
	else
#endif
	{
		status.Error("Unsupported audio format of file " + fileName + "!", INTRA_SOURCE_INFO);
		return null;
	}

	source->OnCloseResource = Funal::Value(Cpp::Move(fileMapping));
	return StreamedSound(Cpp::Move(source), bufSize);
}

void StreamedSound::release()
{
	mData = null;
}

void StreamedSound::Play(bool loop) const
{
	if(!mData) return;
	mData->Play(loop);
}

bool StreamedSound::IsPlaying() const
{
	if(!mData) return false;
	return mData->IsPlaying();
}

void StreamedSound::Stop() const
{
	if(!mData) return;
	mData->Stop();
}

void StreamedSound::UpdateBuffer() const
{
	if(!mData) return;
	mData->Update();
}

void StreamedSound::UpdateAllExistingInstances()
{
	auto& context = SoundContext::Instance();
	Array<Shared<StreamedSound::Data>> soundsToUpdate;

	//Этот мьютекс захватывается в деструкторе каждого объекта.
	INTRA_SYNCHRONIZED(context.MyMutex)
	{
		//Когда мы попадаем сюда, каждый элемент context->AllStreamedSounds
		//либо жив, либо находится в процессе удаления до захвата этого мьютекса.
		soundsToUpdate.Reserve(context.AllStreamedSounds.Length());
		for(auto snd: context.AllStreamedSounds)
		{
			auto ptr = snd->SharedThis();
			if(!ptr || ptr->IsReleased()) continue; //Объект в процессе удаления, обновлять его не нужно.
			soundsToUpdate.AddLast(Cpp::Move(ptr)); //Объект будет жить как минимум до завершения его обновления.
		}
	}

	//Эта часть медленная, поэтому мы и вынесли её из критической секции.
	for(auto& ptr: soundsToUpdate)
	{
		ptr->Update();
		ptr = null;
	}
}

void StreamedSound::ReleaseAllSounds()
{
	SoundContext::Instance().ReleaseAllStreamedSounds();
}

void CleanUpSoundSystem()
{
	Sound::ReleaseAllSounds();
	StreamedSound::ReleaseAllSounds();
}

}}

INTRA_WARNING_POP
