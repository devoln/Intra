#include "Cpp/PlatformDetect.h"
#include "Audio/SoundApi.h"
#include "Cpp/Warnings.h"
#include "Container/Utility/IndexAllocator.h"
#include "Range/Mutation/Fill.h"
#include "Range/Decorators/Take.h"

#if(INTRA_LIBRARY_SOUND_SYSTEM==INTRA_LIBRARY_SOUND_SYSTEM_WebAudio)

#include <emscripten.h>


INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio { namespace SoundAPI {

const ValueType::I InternalBufferType = ValueType::Float;
const int InternalChannelsInterleaved = false;

struct Buffer
{
	size_t id;
	uint sampleCount;
	uint sampleRate;
	uint channels;
	void* lockedData;

	size_t SizeInBytes() const {return sampleCount*channels*sizeof(float);}
};

struct Instance
{
	BufferHandle myBuf;
	size_t id;
};

struct StreamedBuffer
{
	StreamedBuffer(size_t bufid, StreamingCallback callback, uint sample_count, uint sample_rate, uint ch)
	{
		id = bufid;
		streamingCallback = callback;
		sampleCount = sample_count;
		channels = ch;
		sampleRate = sample_rate;
	}

	size_t SizeInBytes() const {return sampleCount*channels*sizeof(float);}

	size_t id;
	uint sampleCount;
	uint sampleRate;
	uint channels;
	StreamingCallback streamingCallback;
	Array<float> tempBuffer;

	bool deleteOnStop = false;
	bool looping = false;
	bool stopSoon =false;
}; 

static bool AudioContextInited = false;
static IndexAllocator<ushort> BufferIdalloc;
static IndexAllocator<ushort> InstanceIdalloc;
static IndexAllocator<ushort> StreamedSoundIdalloc;


static void init_context()
{
	if(AudioContextInited) return;

	AudioContextInited = true;

	EM_ASM(
		Module.gWebAudioContext = new (window.AudioContext || window.webkitAudioContext)();
	    Module.gWebAudioBufferArray = [];
		Module.gWebAudioInstanceArray = [];
		Module.gWebAudioStreamArray = [];
	);
}

uint InternalSampleRate()
{
	init_context();
	return uint(EM_ASM_INT({return Module.gWebAudioContext.sampleRate;}, 0));
}


BufferHandle BufferCreate(size_t sampleCount, uint channels, uint sampleRate)
{
	if(sampleCount==0 || channels==0 || sampleRate==0) return null;
	init_context();

	BufferHandle result = new Buffer{0, sampleCount, sampleRate, channels, null};
	result->id = BufferIdalloc.Allocate();

	EM_ASM_({
		var frameCount = $0;
		var buffer = Module.gWebAudioContext.createBuffer($1, frameCount, $2);
		Module.gWebAudioBufferArray[$3] = buffer;
	}, sampleCount, channels, sampleRate, result->id);

	return result;
}

void BufferSetDataInterleaved(BufferHandle snd, const void* data, ValueType type)
{
	if(snd->channels==1)
	{
		BufferSetDataChannels(snd, &data, type);
		return;
	}

	if(type==ValueType::Float)
	{
		EM_ASM_({
			var bufferData = [];
		    for(var c=0; c<$0; c++) bufferData[c] = Module.gWebAudioBufferArray[$3].getChannelData(c);
			var j=0;
			for(var i=0; i<$2; i++)
				for(var c=0; c<$0; c++)
					bufferData[c][i] = Module.HEAPF32[$1 + j++];
		}, snd->channels, reinterpret_cast<size_t>(data)/sizeof(float), snd->sampleCount, snd->id);
	}
	else if(type==ValueType::Short)
	{
		EM_ASM_({
			var bufferData = [];
		    for(var c=0; c<$0; c++) bufferData[c] = Module.gWebAudioBufferArray[$3].getChannelData(c);
			var j=0;
			for(var i=0; i<$2; i++)
				for(var c=0; c<$0; c++)
					bufferData[c][i] = (Module.HEAP16[$1+i]+0.5)/32767.5;
		}, snd->channels, reinterpret_cast<size_t>(data)/sizeof(short), snd->sampleCount, snd->id);
	}
}

void BufferSetDataChannels(BufferHandle snd, const void* const* data, ValueType type)
{
	for(uint c=0; c<snd->channels; c++)
	{
		if(type==ValueType::Float)
		{
			EM_ASM_({
				var buffer = Module.gWebAudioBufferArray[$3];
				if(buffer.copyToChannel===undefined)
				{
					var bufferData = buffer.getChannelData($0);
					bufferData.set(Module.HEAPF32, $1, $2);
				}
				else buffer.copyToChannel(Module.HEAPF32.subarray($1, $1+$2), $0);
			}, c, reinterpret_cast<size_t>(data[c])/sizeof(float), snd->sampleCount, snd->id);
		}
		else if(type==ValueType::Short)
		{
			EM_ASM_({
				var buffer = Module.gWebAudioBufferArray[$3];
				var bufferData = buffer.getChannelData($0);
				for(var i=0; i<$2; i++) bufferData[i] = (Module.HEAP16[$1+i]+0.5)/32767.5;
			}, c, reinterpret_cast<size_t>(data[c])/sizeof(short), snd->sampleCount, snd->id);
		}
	}
}

void* BufferLock(BufferHandle snd)
{
	INTRA_DEBUG_ASSERT(snd!=null);
	if(snd==null) return null;
	if(snd->lockedData!=null) return snd->lockedData;
	size_t bytesToAllocate = snd->SizeInBytes();
	snd->lockedData = Memory::GlobalHeap.Allocate(bytesToAllocate, INTRA_SOURCE_INFO);
	return snd->lockedData;
}

void BufferUnlock(BufferHandle snd)
{
	if(snd==null || snd->lockedData==null) return;
	EM_ASM_({
		if(Module.gWebAudioBufferArray[$0].copyToChannel === undefined)
		{
			var bufferData = Module.gWebAudioBufferArray[$0].getChannelData(0);
			bufferData.set(Module.HEAPF32.subarray($1, $1+$2));
		}
		else Module.gWebAudioBufferArray[$0].copyToChannel(Module.HEAPF32.subarray($1, $1+$2), 0);
	}, snd->id, reinterpret_cast<size_t>(snd->lockedData)/sizeof(float), snd->sampleCount);
	Memory::GlobalHeap.Free(snd->lockedData, snd->SizeInBytes());
	snd->lockedData = null;
}

void BufferDelete(BufferHandle snd)
{
	if(AudioContextInited)
		EM_ASM_({
		    Module.gWebAudioBufferArray[$0] = null;
		}, snd->id);
	BufferIdalloc.Deallocate(ushort(snd->id));
	delete snd;
}



InstanceHandle InstanceCreate(BufferHandle snd)
{
	if(snd==null) return null;
	InstanceHandle result = new Instance{snd, 0};
	result->id = InstanceIdalloc.Allocate();
	EM_ASM_({
		var source = Module.gWebAudioContext.createBufferSource();
		source.buffer = Module.gWebAudioBufferArray[$0];
		source.connect(Module.gWebAudioContext.destination);
		source.__is_playing = false;
		source.onended = function() {source.__is_playing = false;};
		Module.gWebAudioInstanceArray[$1] = source;
	}, snd->id, result->id);
	return result;
}

void InstanceSetDeleteOnStop(InstanceHandle inst, bool del)
{
	(void)inst; (void)del;
	//inst->deleteOnStop = del;
}

void InstanceDelete(InstanceHandle inst)
{
	InstanceStop(inst);
	EM_ASM_({
		Module.gWebAudioInstanceArray[$0] = null;
	}, inst->id);
	InstanceIdalloc.Deallocate(ushort(inst->id));
	delete inst;
}

void InstancePlay(InstanceHandle inst, bool loop)
{
	INTRA_DEBUG_ASSERT(inst!=null);
	EM_ASM_({
		var src = Module.gWebAudioInstanceArray[$0];
		src.loop = $1;
		src.start();
		src.__is_playing = true;
	}, inst->id, loop);
}

bool InstanceIsPlaying(InstanceHandle inst)
{
	if(inst==null) return false;
	return bool(EM_ASM_INT({
		return Module.gWebAudioInstanceArray[$0].__is_playing;
	}, inst->id));
}

void InstanceStop(InstanceHandle inst)
{
	if(inst==null) return;
	EM_ASM_({
		Module.gWebAudioInstanceArray[$0].stop();
	}, inst->id);
}





extern "C" size_t EMSCRIPTEN_KEEPALIVE Emscripten_StreamedSoundLoadCallback(StreamedBufferHandle snd)
{
	if(snd->stopSoon)
	{
		snd->stopSoon = false;
		StreamedSoundStop(snd);
		return 0;
	}

	void* tempPtrs[16];
	for(size_t c=0; c<snd->channels; c++)
		tempPtrs[c] = snd->tempBuffer.Data()+snd->sampleCount*c;
	size_t floatsRead = snd->streamingCallback.CallbackFunction(tempPtrs, snd->channels,
		ValueType::Float, false, snd->sampleCount, snd->streamingCallback.CallbackData);
	if(floatsRead<snd->sampleCount)
	{
		for(size_t i=0; i<snd->channels; i++)
			reinterpret_cast<float*&>(tempPtrs[i]) += floatsRead;
		if(!snd->looping)
		{
			snd->stopSoon = true;
			for(size_t i=0; i<snd->channels; i++)
				Algo::FillZeros(Range::Take(tempPtrs[i], snd->sampleCount-floatsRead));
		}
		else
		{
			floatsRead += snd->streamingCallback.CallbackFunction(tempPtrs, snd->channels,
				ValueType::Float, false, snd->sampleCount-floatsRead, snd->streamingCallback.CallbackData);
		}
	}
	return floatsRead;
}

StreamedBufferHandle StreamedBufferCreate(size_t sampleCount,
	uint channels, uint sampleRate, StreamingCallback callback)
{
	if(sampleCount==0 || channels==0 || sampleRate==0 || callback.CallbackFunction==null) return null;
	init_context();
	if(sampleCount>16384) sampleCount = 16384;

	StreamedBufferHandle result = new StreamedBuffer(StreamedSoundIdalloc.Allocate(), callback, sampleCount, sampleRate, channels);
	result->tempBuffer.SetCount(sampleCount*channels);

	EM_ASM_({
		var result = Module.gWebAudioContext.createScriptProcessor($1, 0, $2);
		result.__is_playing = false;
		Module.gWebAudioStreamArray[$0] = result;
		result.onaudioprocess = function(audioProcessingEvent)
		{
			Module._Emscripten_StreamedSoundLoadCallback($3);
			var outputBuffer = audioProcessingEvent.outputBuffer;
			for(var ch=0; ch<$2; ch++)
			{
				if(outputBuffer.copyToChannel===undefined)
				{
					var outputData = outputBuffer.getChannelData(ch);
					outputData.set(Module.HEAPF32.subarray($4+ch*$1, $4+ch*$1+$1));
				}
				else outputBuffer.copyToChannel(Module.HEAPF32.subarray($4+ch*$1, $4+ch*$1+$1), ch);
			}
		}
	}, result->id, sampleCount, channels, result, reinterpret_cast<size_t>(result->tempBuffer.Data())/sizeof(float));

	return result;
}

void StreamedBufferSetDeleteOnStop(StreamedBufferHandle snd, bool del)
{
	snd->deleteOnStop = del;
}

void StreamedSoundPlay(StreamedBufferHandle snd, bool loop)
{
	snd->looping = loop;
	EM_ASM_({
		var snd = Module.gWebAudioStreamArray[$0];
		snd.__is_playing = true;
		snd.__is_looping = $1;
		snd.connect(Module.gWebAudioContext.destination);
	}, snd->id, loop);
}

bool StreamedSoundIsPlaying(StreamedBufferHandle snd)
{
	if(snd==null) return false;
	return bool(EM_ASM_INT({
		return Module.gWebAudioStreamArray[$0].__is_playing;
	}, snd->id));
}

void StreamedSoundStop(StreamedBufferHandle snd)
{
	if(snd==null) return;
	EM_ASM_({
		var snd = Module.gWebAudioStreamArray[$0];
		if(snd.__is_playing) snd.disconnect(Module.gWebAudioContext.destination);
		snd.__is_playing = false;
	}, snd->id);
}

void StreamedBufferDelete(StreamedBufferHandle snd)
{
	if(snd==null) return;
	StreamedSoundStop(snd);
	EM_ASM_({
		Module.gWebAudioStreamArray[$0] = null;
	}, snd->id);
	StreamedSoundIdalloc.Deallocate(ushort(snd->id));
	delete snd;
}

void SoundSystemCleanUp()
{

}

INTRA_WARNING_POP

}}}

#else

INTRA_DISABLE_LNK4221

#endif
