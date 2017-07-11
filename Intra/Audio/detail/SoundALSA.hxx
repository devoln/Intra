#pragma once

#include "Sound.h"

#include "Cpp/Warnings.h"

#include "Memory/Allocator/Global.h"

#include "Range/Mutation/Fill.h"

#include "Data/ValueType.h"


#include <alsa/asoundlib.h>


namespace Intra { namespace Audio {

using namespace Intra::Math;

struct Buffer
{
	Buffer(uint buf, uint sample_count, uint sample_rate, uint ch, ushort format):
		buffer(buf), sampleCount(sample_count), sampleRate(sample_rate), channels(ch), alformat(format) {}

	uint SizeInBytes() const {return uint(sampleCount*channels*sizeof(short));}

	uint buffer;
	uint sampleCount;
	uint sampleRate;
	uint channels;

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
	Context(): Device(null), PrimaryBufferSampleCount(16384) {}

	~Context()
	{
		if(Device == null) return;
		snd_pcm_close(Device);
	}

	void Prepare()
	{
		if(Device != null) return;
		
		int err = snd_pcm_open(&Device, "default", SND_PCM_STREAM_PLAYBACK, 0);
		if(err < 0)
		{
			INTRA_DEBUGGER_BREAKPOINT;
			return;
		}

		snd_pcm_hw_params_t* hwParams;
		err = snd_pcm_hw_params_malloc(&hwParams);
		if(err < 0)
		{
			INTRA_DEBUGGER_BREAKPOINT;
			return;
		}

		err = snd_pcm_hw_params_any(Device, hwParams);
		if(err < 0)
		{
			INTRA_DEBUGGER_BREAKPOINT;
			return;
		}

		err = snd_pcm_hw_params_set_access(Device, hwParams, SND_PCM_ACCESS_RW_INTERLEAVED);
		if(err<0)
		{
			INTRA_DEBUGGER_BREAKPOINT;
			return;
		}

		err = snd_pcm_hw_params_set_format(Device, hwParams, SND_PCM_FORMAT_S16_LE);
		if(err<0)
		{
			INTRA_DEBUGGER_BREAKPOINT;
			return;
		}

		uint exactSampleRate = SW_OUTPUT_DEVICE_SAMPLE_RATE;
		err = snd_pcm_hw_params_set_rate_near(Device, hwParams, &exactSampleRate, 0);
		if(err < 0)
		{
			INTRA_DEBUGGER_BREAKPOINT;
			return;
		}

		err = snd_pcm_hw_params_set_channels(Device, hwParams, 2);
		if(err < 0)
		{
			INTRA_DEBUGGER_BREAKPOINT;
			return;
		}

		unsigned long exactSize = PrimaryBufferSampleCount*2; 
		err = snd_pcm_hw_params_set_buffer_size_near(Device, hwParams, &exactSize);
		if(err < 0)
		{
			INTRA_DEBUGGER_BREAKPOINT;
			return;
		}

		if(exactSize != PrimaryBufferSampleCount*2)
		{
			//Log_Write( "Warning! %d buffer size is not supported, using %d instead.", PrimaryBufferSampleCount, exactSize);
			PrimaryBufferSampleCount = exactSize/2;
		}
    
		err = snd_pcm_hw_params(Device, hwParams);
		if(err<0)
		{
			INTRA_DEBUGGER_BREAKPOINT;
			return;
		}

		snd_pcm_hw_params_free(hwParams);

		snd_pcm_sw_params_t* swParams;
		err = snd_pcm_sw_params_malloc(&swParams);
		if(err < 0)
		{
			INTRA_DEBUGGER_BREAKPOINT;
			return;
		}

		err = snd_pcm_sw_params_current(Device, swParams);
		if(err < 0)
		{
			INTRA_DEBUGGER_BREAKPOINT;
			return;
		}

		err = snd_pcm_sw_params_set_avail_min(Device, swParams, PrimaryBufferSampleCount);
		if(err < 0)
		{
			INTRA_DEBUGGER_BREAKPOINT;
			return;
		}

		err = snd_pcm_sw_params_set_start_threshold(Device, swParams, 0);
		if (err < 0)
		{
			INTRA_DEBUGGER_BREAKPOINT;
			return;
		}

		err = snd_pcm_sw_params(Device, swParams);
		if(err < 0)
		{
			INTRA_DEBUGGER_BREAKPOINT;
			return;
		}

		err = snd_pcm_prepare(Device);
		if(err < 0)
		{
			INTRA_DEBUGGER_BREAKPOINT;
			return;
		}
	}

	Context(const Context&) = delete;
	Context& operator=(const Context&) = delete;


	snd_pcm_t* Device;
    int PrimaryBufferSampleCount;
};
static Context context;


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
	auto size = snd->SizeInBytes();
    snd->locked_bits = Memory::GlobalHeap.Allocate(size, INTRA_SOURCE_INFO);
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
	size_t size = result->SizeInBytes();
	result->temp_buffer = Memory::GlobalHeap.Allocate(size, INTRA_SOURCE_INFO);
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
		snd->channels, Data::ValueType::Short, true, snd->sampleCount, snd->streamingCallback.CallbackData);
	if(samplesProcessed < snd->sampleCount)
	{
		short* const endOfData = snd->temp_buffer+snd->sampleCount*snd->channels;
		const size_t totalSamplesInBuffer = (snd->sampleCount-samplesProcessed)*snd->channels;
		FillZeros(Range::Take(endOfData, totalSamplesInBuffer));
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

}}

