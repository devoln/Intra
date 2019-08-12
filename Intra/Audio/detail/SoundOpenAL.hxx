


#include "Audio/Sound.h"

#include "Memory/Allocator/Global.h"

#include "Core/Range/Mutation/Fill.h"
#include "Core/Range/Mutation/Cast.h"

#include "Data/ValueType.h"

#include "Concurrency/Mutex.h"
#include "Concurrency/Lock.h"

#include "SoundBasicData.hxx"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#ifdef _MSC_VER
#pragma warning(disable: 4668)
#endif

#ifdef __APPLE__
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#elif(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Linux)
#include <AL/al.h>
#include <AL/alc.h>
#else
#include <al.h>
#include <alc.h>
#endif

#ifdef _MSC_VER
#pragma comment(lib, "OpenAL32.lib")
#endif


INTRA_BEGIN
namespace Audio {

using Data::ValueType;

uint Sound::DefaultSampleRate() {return 44100;}


struct SoundContext: detail::SoundBasicContext
{
	SoundContext()
	{
		const ALCchar* defaultDevice = reinterpret_cast<const ALCchar*>(alcGetString(null, ALC_DEFAULT_DEVICE_SPECIFIER));
		ald = alcOpenDevice(defaultDevice);
		alc = alcCreateContext(ald, null);
		alcMakeContextCurrent(alc);
		alcProcessContext(alc);
		//alListenerfv(AL_POSITION, Vec3(0,0,0));
		//alListenerfv(AL_VELOCITY, Vec3(0,0,0));
		//alListenerfv(AL_ORIENTATION, Vec3(0,0,0));
	}

	~SoundContext()
	{
		Clean();
	}

	void Clean()
	{
		if(alc == null) return;
		ReleaseAllSounds();
		ReleaseAllStreamedSounds();
		alcMakeContextCurrent(null);
		alcDestroyContext(alc);
		alc = null;
		if(ald == null) return;
		alcCloseDevice(ald);
	}

	void ReleaseAllSounds();
	void ReleaseAllStreamedSounds();

	//Очистка уже не используемых буферов и источников звука
	void GC();

	static SoundContext& Instance()
	{
		static SoundContext context;
		return context;
	}

	SoundContext(const SoundContext&) = delete;
	SoundContext& operator=(const SoundContext&) = delete;


	ALCdevice* ald;
	ALCcontext* alc;
};

struct Sound::Data: SharedClass<Sound::Data>, detail::SoundBasicData
{
	Data(IAudioSource& src): SoundBasicData(src)
	{
		Info.SampleType = ValueType::SNorm16;
		auto& context = SoundContext::Instance();
		context.GC();
		uint buffer = 0;
		alGenBuffers(1, &buffer);
		INTRA_DEBUG_ASSERT(alGetError() == AL_NO_ERROR);
		AlFormat = ushort(Info.Channels == 1? AL_FORMAT_MONO16: AL_FORMAT_STEREO16);
		Buffer.Set(int(buffer));

		ValueType srcType;
		bool srcInterleaved;
		auto srcRawData = src.GetRawSamplesData(0, &srcType, &srcInterleaved, null);
		if(srcRawData != null && srcType == ValueType::SNorm16 && (srcInterleaved || src.ChannelCount() == 1))
		{
			SetDataInterleaved(srcRawData.First(), ValueType::SNorm16);
		}
		else
		{
			Array<short> dst;
			dst.SetCountUninitialized(Info.SampleCount*src.ChannelCount());
			src.GetInterleavedSamples(dst);
			SetDataInterleaved(dst.Data(), ValueType::SNorm16);
		}

		INTRA_SYNCHRONIZED(context.MyMutex)
		{
			context.AllSounds.AddLast(this);
		}
	}

	forceinline ~Data() {Release();}

	Data(const Data&) = delete;
	Data& operator=(const Data&) = delete;

	void SetDataInterleaved(const void* data, ValueType type)
	{
		(void)type;
		INTRA_DEBUG_ASSERT(data != null);
		if(type == ValueType::SNorm16)
		{
			alBufferData(uint(Buffer.Get()), AlFormat, data, int(Info.GetBufferSize()), int(Info.SampleRate));
		}
		if(type == ValueType::Float)
		{
			auto src = SpanOfRawElements<float>(data, Info.SampleCount*Info.Channels);
			Array<short> dst;
			dst.SetCountUninitialized(src.Length());
			CastFromNormalized(dst.AsRange(), src);
			alBufferData(uint(Buffer.Get()), AlFormat, dst.Data(), int(Info.GetBufferSize()), int(Info.SampleRate));
		}
		INTRA_DEBUG_ASSERT(alGetError() == AL_NO_ERROR);
	}

	void Release()
	{
		uint buffer = uint(Buffer.GetSet(0));
		if(!buffer) return;
		ReleaseAllInstances();
		alDeleteBuffers(1, &buffer);
		auto& context = SoundContext::Instance();
		INTRA_SYNCHRONIZED(context.MyMutex)
		{
			context.AllSounds.FindAndRemoveUnordered(this);
		}
	}

	void ReleaseAllInstances();

	forceinline bool IsReleased() const {return Buffer.Get() == 0;}

	//Убирает ссылки на экземпляры, воспроизведение которых было завершено.
	//Если на текущий буфер не останется ссылок, он автоматически будет удалён.
	void GC();

	AtomicInt Buffer;
	ushort AlFormat;

	Synchronized<Array<Sound::Instance::Data*>> PlayingSources;
};

struct Sound::Instance::Data: SharedClass<Sound::Instance::Data>, detail::SoundInstanceBasicData
{
	Data(Shared<Sound::Data> parent): SoundInstanceBasicData(Move(parent))
	{
		INTRA_DEBUG_ASSERT(Parent != null);
		uint source = 0;
		SoundContext::Instance().GC();
		alGenSources(1, &source);
		//alSource3f(source, AL_POSITION, 0, 0, 0);
		//alSource3f(source, AL_VELOCITY, 0, 0, 0);
		//alSource3f(source, AL_DIRECTION, 0, 0, 0);
		//alSourcef(source, AL_ROLLOFF_FACTOR, 0);
		//alSourcei(source, AL_SOURCE_RELATIVE, true);
		alSourcei(source, AL_BUFFER, int(Parent->Buffer.Get()));
		INTRA_DEBUG_ASSERT(alGetError() == AL_NO_ERROR);
		AlSource.Set(int(source));
	}

	~Data() {Release();}

	Data(const Data&) = delete;
	Data& operator=(const Data&) = delete;

	void Release()
	{
		uint source = uint(AlSource.GetSet(0));
		if(!source) return;
		Parent->PlayingSources->FindAndRemoveUnordered(this);
		alDeleteSources(1, &source);
		SelfRef = null;
	}

	static void checkError()
	{
#ifdef INTRA_DEBUG
		const auto err = alGetError();
		INTRA_DEBUG_ASSERT(err == AL_NO_ERROR || err == AL_INVALID_NAME);
#endif
	}

	void Play(bool loop)
	{
		const uint source = uint(AlSource.Get());
		if(!source) return;
		if(IsPlaying()) return;
		SelfRef = SharedThis();
		Parent->PlayingSources->AddLast(this);
		alSourcei(source, AL_LOOPING, loop);
		checkError();
		alSourcePlay(source);
		checkError();
	}

	bool IsPlaying()
	{
		ALenum state;
		alGetSourcei(uint(AlSource.Get()), AL_SOURCE_STATE, &state);
		return state == AL_PLAYING;
	}

	void Stop()
	{
		const uint source = uint(AlSource.Get());
		if(!source) return;
		Parent->PlayingSources->FindAndRemoveUnordered(this);
		alSourceStop(source);
		SelfRef = null;
	}

	Shared<Data> SelfRef;
	AtomicInt AlSource;
};


struct StreamedSound::Data: SharedClass<StreamedSound::Data>, detail::StreamedSoundBasicData
{
	Data(Unique<IAudioSource> source, size_t bufferSampleCount, bool autoStreamingEnabled = false):
		StreamedSoundBasicData(Move(source), bufferSampleCount),
		TempBuffer(Source->ChannelCount() * BufferSampleCount)
	{
		(void)autoStreamingEnabled; //not supported

		INTRA_DEBUG_ASSERT(Source != null);
		INTRA_DEBUG_ASSERT(Source->ChannelCount() <= 2);
		auto& context = SoundContext::Instance();
		alGenBuffers(2, AlBuffers);
		uint alsource = 0;
		alGenSources(1, &alsource);
		AlSource.SetRelaxed(int(alsource));

		INTRA_SYNCHRONIZED(context.MyMutex)
		{
			context.AllStreamedSounds.AddLast(this);
		}
	}

	Data(const Data&) = delete;
	Data& operator=(const Data&) = delete;

	~Data() {Release();}

	void Release()
	{
		uint source = uint(AlSource.GetSet(0));
		if(!source) return;
		alDeleteSources(1, &source);
		alDeleteBuffers(2, AlBuffers);
		TempBuffer = null;
		auto& context = SoundContext::Instance();
		INTRA_SYNCHRONIZED(context.MyMutex)
		{
			context.AllStreamedSounds.FindAndRemoveUnordered(this);
		}
	}

	forceinline bool IsReleased() const {return AlSource.Get() == 0;}

	bool loadBuffer(size_t index)
	{
		const int alFmt = Source->ChannelCount() == 1? AL_FORMAT_MONO16: AL_FORMAT_STEREO16;
		const size_t samplesProcessed = Source->GetInterleavedSamples(TempBuffer);
		bool stopSoon = false;
		if(samplesProcessed < BufferSampleCount)
		{
			FillZeros(TempBuffer.AsRange().Drop(samplesProcessed));
			stopSoon = Status.CompareSet(STATUS_PLAYING, STATUS_STOPPING);
		}
		alBufferData(AlBuffers[index], alFmt, TempBuffer.Data(),
			int(GetBufferSize()), int(Source->SampleRate()));
		return stopSoon;
	}

	void Play(bool loop)
	{
		uint source = uint(AlSource.Get());
		if(!source) return;
		if(Status.GetSet(loop? STATUS_LOOPING: STATUS_PLAYING) != STATUS_STOPPED) return;
		SelfRef = SharedThis();
		bool stopSoon = loadBuffer(0);
		if(!stopSoon) loadBuffer(1);
		alSourceQueueBuffers(source, 2, AlBuffers);
		alSourcePlay(source);
	}

	forceinline bool IsPlaying() const {return Status.Get() != STATUS_STOPPED;}

	void Stop()
	{
		if(Status.GetSet(STATUS_STOPPED) == STATUS_STOPPED) return;
		const uint source = uint(AlSource.Get());
		alSourceStop(source);
		alSourceUnqueueBuffers(source, 2, AlBuffers);
		SelfRef = null;
	}

	void Update()
	{
		int countProcessed;
		const uint source = uint(AlSource.Get());
		alGetSourcei(source, AL_BUFFERS_PROCESSED, &countProcessed);
		while(countProcessed --> 0)
		{
			if(Status.Get() == STATUS_STOPPING)
			{
				Stop();
				return;
			}
			alSourceUnqueueBuffers(source, 1, AlBuffers);
			loadBuffer(0);
			alSourceQueueBuffers(source, 1, AlBuffers);
			Core::Swap(AlBuffers[0], AlBuffers[1]);
		}
	}

	uint AlBuffers[2];
	AtomicInt AlSource;
	FixedArray<short> TempBuffer;

	enum {STATUS_STOPPED, STATUS_PLAYING, STATUS_LOOPING, STATUS_STOPPING};
	AtomicInt Status{STATUS_STOPPED};
};

void SoundContext::ReleaseAllSounds()
{
	Array<Sound::Data*> soundsToRelease;
	INTRA_SYNCHRONIZED(MyMutex)
	{
		soundsToRelease = Move(AllSounds);
	}
	for(auto snd: soundsToRelease) snd->Release();
}

void SoundContext::ReleaseAllStreamedSounds()
{
	Array<StreamedSound::Data*> soundsToRelease;
	INTRA_SYNCHRONIZED(MyMutex)
	{
		soundsToRelease = Move(AllStreamedSounds);
	}
	for(auto snd: soundsToRelease) snd->Release();
}

void SoundContext::GC()
{
	INTRA_SYNCHRONIZED(MyMutex)
	{
		for(auto snd: AllSounds) snd->GC();
	}
}

void Sound::Data::GC()
{
	Array<Shared<Sound::Instance::Data>> stoppedSources;
	INTRA_SYNCHRONIZED(PlayingSources)
	{
		for(size_t i = 0; i < PlayingSources.Value.Length(); i++)
		{
			auto src = PlayingSources.Value[i];
			if(src->IsPlaying()) continue;
			stoppedSources.AddLast(src->SharedThis());
			src->SelfRef = null;
			PlayingSources.Value.RemoveUnordered(i--);
		}
	}
}

void Sound::Data::ReleaseAllInstances()
{
	Array<Shared<Sound::Instance::Data>> playingSources;
	INTRA_SYNCHRONIZED(PlayingSources)
	{
		playingSources.Reserve(PlayingSources.Value.Length());
		for(auto src: PlayingSources.Value) playingSources.AddLast(src->SharedThis());
		PlayingSources.Value = null;
	}
	for(auto& src: playingSources) src->Release();
}

}}

INTRA_WARNING_POP
