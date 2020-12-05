#include "Sound.h"
#include "AudioBuffer.h"
#include "AudioSource.h"
#include "Sources/Wave.h"
#include "Sources/Vorbis.h"
#include "SoundTypes.h"

#include "Intra/Functional.h"
#include "Intra/Range/Comparison.h"
#include "Intra/Range/Search/Single.h"

#include "IntraX/IO/FileSystem.h"

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


INTRA_BEGIN
Sound::Sound(decltype(null)) {}

Sound::Sound(const SoundInfo& bufferInfo, const void* initData)
{
	Sources::Wave src(null, bufferInfo, initData);
	mData = Shared<Data>::New(src);
}

Sound::Sound(const AudioBuffer& dataBuffer)
{
	Sources::Wave src(null, dataBuffer.SampleRate, 1, dataBuffer.Samples);
	mData = Shared<Data>::New(src);
}

Sound::Sound(Sound&& rhs): mData(Move(rhs.mData)) {}

Sound::Sound(IAudioSource& src, ErrorReporter err):
	mData(Shared<Data>::New(src))
{
	if(mData->Info != null) return;
	mData = null;
	err.Error({ErrorCode::Other}, String::Concat("Couldn't create sound buffer with size of ", StringOf(src.SamplesLeft()), " samples!"), INTRA_SOURCE_INFO);
}

Sound::~Sound() {Release();}

void Sound::Release() {mData = null;}

Sound& Sound::operator=(Sound&&) = default;

Sound Sound::FromFile(StringView fileName, ErrorReporter err)
{
	auto fileMapping = OS.MapFile(fileName, err);
	if(fileMapping==null) return null;

	auto fileSignature = fileMapping.AsRangeOf<char>();

	Unique<IAudioSource> source;

	if(fileSignature.StartsWith("RIFF"))
		source = new Sources::Wave(null, fileMapping.AsRange());
	else
#if(INTRA_LIBRARY_VORBIS_DECODER != INTRA_LIBRARY_VORBIS_DECODER_None)
	if(fileSignature.StartsWith("OggS"))
		source = new Sources::Vorbis(fileData);
	else
#endif
	{
		err.Error({ErrorCode::NotSupported}, "Unsupported audio format of file " + fileName + "!", INTRA_SOURCE_INFO);
		return null;
	}

	return Sound(*source, err);
}

void Sound::ReleaseAllSounds()
{
	SoundContext::Instance().ReleaseAllSounds();
}

Sound::Instance Sound::CreateInstance()
{
	INTRA_DEBUG_ASSERT(mData != null);
	if(mData == null) return null;
	return Instance(*this);
}

Sound::Instance::Instance(Sound& sound):
	mData(Shared<Data>::New(sound.mData)) {}

Sound::Instance::Instance(decltype(null)) {}
Sound::Instance::Instance(Instance&&) = default;
Sound::Instance& Sound::Instance::operator=(Instance&&) = default;

void Sound::Instance::Release() {mData = null;}

Sound::Instance::~Instance() {Release();}

void Sound::Instance::Play(bool loop) const {if(mData) mData->Play(loop);}
bool Sound::Instance::IsPlaying() const {return mData && mData->IsPlaying();}
void Sound::Instance::Stop() const {if(mData) mData->Stop();}

StreamedSound::StreamedSound(decltype(null)) {}

StreamedSound::StreamedSound(StreamedSound&&) = default;

StreamedSound::StreamedSound(Unique<IAudioSource> src, Size bufferSizeInSamples):
	mData(Shared<Data>::New(Move(src), index_t(bufferSizeInSamples))) {}

StreamedSound::~StreamedSound() {release();}

StreamedSound& StreamedSound::operator=(StreamedSound&&) = default;

StreamedSound StreamedSound::FromFile(StringView fileName, Size bufSize, ErrorReporter err)
{
	auto fileMapping = OS.MapFile(fileName, err);
	if(fileMapping == null) return null;
	Unique<IAudioSource> source;
	auto fileSignature = fileMapping.AsRangeOf<char>();
#ifndef INTRA_NO_WAVE_LOADER
	if(fileSignature.StartsWith("RIFF"))
		source = new Sources::Wave(Value(Move(fileMapping)), fileMapping);
	else
#endif
#if(INTRA_LIBRARY_VORBIS_DECODER != INTRA_LIBRARY_VORBIS_DECODER_None)
	if(fileSignature.StartsWith("OggS"))
		source = new Sources::Vorbis(Funal::Value(Move(fileMapping)), fileMapping);
	else 
#endif
	{
		err.Error({ErrorCode::NotSupported}, "Unsupported audio format of file " + fileName + "!", INTRA_SOURCE_INFO);
		return null;
	}

	return StreamedSound(Move(source), bufSize);
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
#if(INTRA_LIBRARY_SOUND != INTRA_LIBRARY_SOUND_WebAudio)
	mData->Update();
#endif
}

void StreamedSound::UpdateAllExistingInstances()
{
#if(INTRA_LIBRARY_SOUND != INTRA_LIBRARY_SOUND_WebAudio && INTRA_LIBRARY_SOUND != INTRA_LIBRARY_SOUND_Dummy)
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
			soundsToUpdate.AddLast(Move(ptr)); //Объект будет жить как минимум до завершения его обновления.
		}
	}

	//Эта часть медленная, поэтому мы и вынесли её из критической секции.
	for(auto& ptr: soundsToUpdate)
	{
		ptr->Update();
		ptr = null;
	}
#endif
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
INTRA_END
