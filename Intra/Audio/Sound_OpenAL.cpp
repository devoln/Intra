#include "Cpp/PlatformDetect.h"
#include "Audio/SoundApi.h"
#include "Cpp/Warnings.h"
#include "Memory/Allocator/Global.h"

#if(INTRA_LIBRARY_SOUND_SYSTEM==INTRA_LIBRARY_SOUND_SYSTEM_OpenAL)

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#ifdef __APPLE__
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#elif(INTRA_PLATFORM_OS==PLATFORM_OS_Linux)
#include <AL/al.h>
#include <AL/alc.h>
#else
#include <al.h>
#include <alc.h>
#endif

#ifdef _MSC_VER
#pragma comment(lib, "OpenAL32.lib")
#endif


namespace Intra { namespace Audio {

using namespace Math;

namespace SoundAPI {

const ValueType::I InternalBufferType = ValueType::Short;
const int InternalChannelsInterleaved = true;
uint InternalSampleRate() {return 48000;}

struct Buffer
{
	Buffer(uint buf, uint sample_count, uint sample_rate, uint ch, ushort format):
		buffer(buf), sampleCount(sample_count), sampleRate(sample_rate), channels(ch), alformat(format) {}

	uint SizeInBytes() const {return uint(sampleCount*channels*sizeof(short));}

	uint buffer;
	uint sampleCount;
	uint sampleRate;
	uint channels;
	ushort alformat;

	void* locked_bits=null;
};

struct Instance
{
	Instance(uint src, BufferHandle my_parent): source(src), parent(my_parent) {}

	uint source;
	BufferHandle parent;

	bool deleteOnStop=false;
};


struct StreamedBuffer
{
	//StreamedBuffer(uint bufs[2], StreamingCallback callback, uint sample_count, uint sample_rate, ushort ch):
		//buffers{bufs[0], bufs[1]}, streamingCallback(callback), sampleCount(sampleCount), sampleRate(sample_rate), channels(ch) {}

	uint SizeInBytes() const {return uint(sampleCount*channels*sizeof(short));}

	uint buffers[2];
	uint source;
	uint sampleCount; //Размер половины буфера
	uint sampleRate;
	uint channels;
	StreamingCallback streamingCallback;
	short* temp_buffer;

	bool deleteOnStop=false;
	bool looping=false;
	byte stop_soon=0;
};


struct Context
{
	Context(): ald(null), alc(null) {}

	~Context()
	{
		Clean();
	}

	void Prepare()
	{
		if(alc!=null) return;
		const ALCchar* defaultDevice = reinterpret_cast<const ALCchar*>(alcGetString(null, ALC_DEFAULT_DEVICE_SPECIFIER));
		if(ald==null) ald = alcOpenDevice(defaultDevice);
		alc = alcCreateContext(ald, null);
		alcMakeContextCurrent(alc);
		alcProcessContext(alc);
		//alListenerfv(AL_POSITION, Vec3(0,0,0));
		//alListenerfv(AL_VELOCITY, Vec3(0,0,0));
		//alListenerfv(AL_ORIENTATION, Vec3(0,0,0));
	}

	void Clean()
	{
		if(alc==null) return;
		alcMakeContextCurrent(null);
		alcDestroyContext(alc);
		alc = null;
		if(ald==null) return;
		alcCloseDevice(ald);
	}

	Context(const Context&) = delete;
	Context& operator=(const Context&) = delete;


	ALCdevice* ald;
	ALCcontext* alc;
};
static Context context;


static ushort get_format(uint channels)
{
	INTRA_DEBUG_ASSERT(channels>0 && channels<=2);
    if(channels==1) return AL_FORMAT_MONO16;
    if(channels==2) return AL_FORMAT_STEREO16;
    return 0;
}

BufferHandle BufferCreate(size_t sampleCount, uint channels, uint sampleRate)
{
	context.Prepare();
	uint buffer;
	alGenBuffers(1, &buffer);
	return new Buffer(buffer, uint(sampleCount), sampleRate, channels, get_format(channels));
}

void BufferSetDataInterleaved(BufferHandle snd, const void* data, ValueType type)
{
	(void)type;
	INTRA_DEBUG_ASSERT(data!=null);
	INTRA_DEBUG_ASSERT(type==ValueType::Short);
    alBufferData(snd->buffer, snd->alformat, data, int(snd->SizeInBytes()), int(snd->sampleRate));
    INTRA_DEBUG_ASSERT(alGetError()==AL_NO_ERROR);
}

void* BufferLock(BufferHandle snd)
{
	INTRA_DEBUG_ASSERT(snd!=null);
	size_t bytesToAllocate = snd->SizeInBytes();
    snd->locked_bits = Memory::GlobalHeap.Allocate(bytesToAllocate, INTRA_SOURCE_INFO);
	return snd->locked_bits;
}

void BufferUnlock(BufferHandle snd)
{
	INTRA_DEBUG_ASSERT(snd!=null);
    alBufferData(snd->buffer, snd->alformat, snd->locked_bits, int(snd->SizeInBytes()), int(snd->sampleRate));
    INTRA_DEBUG_ASSERT(alGetError()==AL_NO_ERROR);
    Memory::GlobalHeap.Free(snd->locked_bits, snd->SizeInBytes());
    snd->locked_bits=null;
}

void BufferDelete(BufferHandle snd)
{
	if(snd==null) return;
	alDeleteBuffers(1, &snd->buffer);
	delete snd;
}

InstanceHandle InstanceCreate(BufferHandle snd)
{
	INTRA_DEBUG_ASSERT(snd!=null);
	uint source;
	alGenSources(1, &source);
	//alSource3f(source, AL_POSITION, 0, 0, 0);
	//alSource3f(source, AL_VELOCITY, 0, 0, 0);
	//alSource3f(source, AL_DIRECTION, 0, 0, 0);
	//alSourcef(source, AL_ROLLOFF_FACTOR, 0);
	//alSourcei(source, AL_SOURCE_RELATIVE, true);
	alSourcei(source, AL_BUFFER, int(snd->buffer));
	INTRA_DEBUG_ASSERT(alGetError()==AL_NO_ERROR);
	return new Instance(source, snd);
}

void InstanceSetDeleteOnStop(InstanceHandle si, bool del)
{
	si->deleteOnStop = del;
}

void InstanceDelete(InstanceHandle si)
{
	if(si==null) return;
	alDeleteSources(1, &si->source);
	delete si;
}

void InstancePlay(InstanceHandle si, bool loop)
{
	INTRA_DEBUG_ASSERT(si!=null);
	alSourcei(si->source, AL_LOOPING, loop);
	alSourcePlay(si->source);
	INTRA_DEBUG_ASSERT(alGetError()==AL_NO_ERROR);
}

bool InstanceIsPlaying(InstanceHandle si)
{
	if(si==null) return false;
	ALenum state;
	alGetSourcei(si->source, AL_SOURCE_STATE, &state);
	return (state==AL_PLAYING);
}

void InstanceStop(InstanceHandle si)
{
	if(si==null) return;
	alSourceStop(si->source);
}

StreamedBufferHandle StreamedBufferCreate(size_t sampleCount,
	uint channels, uint sampleRate, StreamingCallback callback)
{
	context.Prepare();
	if(sampleCount==0 || channels==0 ||
		sampleRate==0 || callback.CallbackFunction==null)
			return null;
	INTRA_DEBUG_ASSERT(channels<=2);
	StreamedBufferHandle result = new StreamedBuffer;
	alGenBuffers(2, result->buffers);
	alGenSources(1, &result->source);
	result->sampleCount = uint(sampleCount);
	result->sampleRate = sampleRate;
	result->channels = channels;
	result->streamingCallback = callback;
	size_t bytesToAllocate = result->SizeInBytes();
	result->temp_buffer = Memory::GlobalHeap.Allocate(bytesToAllocate, INTRA_SOURCE_INFO);
	return result;
}

void StreamedBufferSetDeleteOnStop(StreamedBufferHandle snd, bool del)
{
	snd->deleteOnStop = del;
}

void StreamedBufferDelete(StreamedBufferHandle snd)
{
	alDeleteSources(1, &snd->source);
	alDeleteBuffers(2, snd->buffers);
	Memory::GlobalHeap.Free(snd->temp_buffer, snd->SizeInBytes());
}


static void load_buffer(StreamedBufferHandle snd, size_t index)
{
	const int alFmt = snd->channels==1? AL_FORMAT_MONO16: AL_FORMAT_STEREO16;
	const size_t samplesProcessed = snd->streamingCallback.CallbackFunction(reinterpret_cast<void**>(&snd->temp_buffer),
		snd->channels, ValueType::Short, true, snd->sampleCount, snd->streamingCallback.CallbackData);
	if(samplesProcessed<snd->sampleCount)
	{
		short* const endOfData = snd->temp_buffer+snd->sampleCount*snd->channels;
		const size_t totalSamplesInBuffer = (snd->sampleCount-samplesProcessed)*snd->channels;
		C::memset(endOfData, 0, totalSamplesInBuffer*sizeof(short));
		snd->stop_soon = true;
	}
	alBufferData(snd->buffers[index], alFmt, snd->temp_buffer, int(snd->SizeInBytes()), int(snd->sampleRate));
}


void StreamedSoundPlay(StreamedBufferHandle snd, bool loop)
{
	INTRA_DEBUG_ASSERT(snd!=null);
	load_buffer(snd, 0);
	if(!snd->stop_soon) load_buffer(snd, 1);
	alSourceQueueBuffers(snd->source, 2, snd->buffers);
	alSourcePlay(snd->source);
	snd->looping = loop;
}

bool StreamedSoundIsPlaying(StreamedBufferHandle si)
{
	int state;
	alGetSourcei(si->source, AL_SOURCE_STATE, &state);
	return state==AL_PLAYING;
}

void StreamedSoundStop(StreamedBufferHandle snd)
{
	if(!StreamedSoundIsPlaying(snd)) return;
	alSourceStop(snd->source);
	alSourceUnqueueBuffers(snd->source, 2, snd->buffers);
	snd->stop_soon = false;
	if(snd->deleteOnStop) StreamedBufferDelete(snd);
}

void StreamedSoundUpdate(StreamedBufferHandle snd)
{
	int countProcessed;
	alGetSourcei(snd->source, AL_BUFFERS_PROCESSED, &countProcessed);
	while(countProcessed--!=0)
	{
		if(snd->stop_soon && !snd->looping)
		{
			StreamedSoundStop(snd);
			snd->stop_soon = false;
			return;
		}
		alSourceUnqueueBuffers(snd->source, 1, snd->buffers);
		load_buffer(snd, 0);
		alSourceQueueBuffers(snd->source, 1, snd->buffers);
		Cpp::Swap(snd->buffers[0], snd->buffers[1]);
	}
}

void SoundSystemCleanUp()
{
	context.Clean();
}

INTRA_WARNING_POP

}}}

#else

INTRA_DISABLE_LNK4221

#endif
