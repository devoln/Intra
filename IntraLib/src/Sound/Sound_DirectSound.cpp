#include "Core/Core.h"

#if(INTRA_LIBRARY_SOUND_SYSTEM==INTRA_LIBRARY_SOUND_SYSTEM_DirectSound)

#include "Sound/SoundApi.h"

#define INITGUID

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4668)
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
struct IUnknown;
#include <windows.h>
#include <MMSystem.h>
#include <dsound.h>

#ifdef _MSC_VER
#pragma comment(lib, "dsound.lib")
#pragma comment(lib, "user32.lib")
#pragma warning(pop)
#endif


namespace Intra { namespace SoundAPI {

const ValueType::I InternalBufferType = ValueType::Short;
const int InternalChannelsInterleaved = true;

struct Buffer
{
	Buffer(IDirectSoundBuffer* buf, uint sample_count, uint sample_rate, uint ch)
	{
		buffer = buf;
		sampleCount = sample_count;
		channels = ch;
		sampleRate = sample_rate;
	}

	uint SizeInBytes() const {return uint(sampleCount*channels*sizeof(short));}

	IDirectSoundBuffer* buffer;
	uint sampleCount;
	uint sampleRate;
	uint channels;

	void* lockedBits=null;
};

struct Instance
{
	Instance(IDirectSoundBuffer* dup_buf, BufferHandle my_parent)
	{
		dupBuffer = dup_buf;
		parent = my_parent;
	}

	IDirectSoundBuffer* dupBuffer;
	BufferHandle parent;

	HANDLE notifyOnStopEvent=null;
	HANDLE notifyOnStopWait=null;
	bool deleteOnStop=false;
};


struct StreamedBuffer
{
	StreamedBuffer(IDirectSoundBuffer* buf, StreamingCallback callback, uint sample_count, uint sample_rate, uint ch)
	{
		buffer = buf;
		streamingCallback = callback;
		sampleCount = sample_count;
		channels = ch;
		sampleRate = sample_rate;
		InitializeCriticalSection(&critsec);
	}

	//! Размер в байтах половины буфера
	uint SizeInBytes() const {return uint(sampleCount*channels*sizeof(short));}

	IDirectSoundBuffer* buffer;
	uint sampleCount; //Размер половины буфера
	uint sampleRate;
	uint channels;
	StreamingCallback streamingCallback;

	HANDLE notifyLoadEvent=null;
	HANDLE notifyLoadWait=null;
	bool deleteOnStop=false;
	bool looping=false;
	byte stop_soon=0;

	byte buffers_processed=1;
	byte next_buffer_to_fill=1;

	CRITICAL_SECTION critsec;
	DWORD lockedSize = 0;
	void* lockedBits = null;
	DWORD lockedSize2 = 0;
	void* lockedBits2 = null;
};

static IDirectSound8* context=null;
static CRITICAL_SECTION contextCritSect;

struct context_deleter
{
	context_deleter()
	{
		InitializeCriticalSection(&contextCritSect);
	}

	~context_deleter()
	{
		if(context==null) return;
		EnterCriticalSection(&contextCritSect);
		auto cont=context;
		context=null;
		cont->Release();
		LeaveCriticalSection(&contextCritSect);
	}
} context_deleter;

static void init_context()
{
	if(context!=null) return;

	if(!TryEnterCriticalSection(&contextCritSect))
	{
		//Контекст уже создаётся в данный момент из другого потока. Дождёмся завершения и вернём управление
		EnterCriticalSection(&contextCritSect);
		LeaveCriticalSection(&contextCritSect);
		return;
	}

	IDirectSound8* result = null;
	if(FAILED(DirectSoundCreate8(null, &result, null)))
	{
		LeaveCriticalSection(&contextCritSect);
		return;
	}
	result->SetCooperativeLevel(GetDesktopWindow(), DSSCL_PRIORITY);

	DSBUFFERDESC dsbd = {sizeof(DSBUFFERDESC), DSBCAPS_PRIMARYBUFFER|DSBCAPS_LOCSOFTWARE, 0, 0, null, {}};
	IDirectSoundBuffer* primary;
	if(FAILED(result->CreateSoundBuffer(&dsbd, &primary, null)))
	{
		LeaveCriticalSection(&contextCritSect);
		result->Release();
		return;
	}
	const ushort samples=44100, channels=2, bitsPerSample=16, blockAlign = bitsPerSample/8*channels;
	const WAVEFORMATEX wfx = {WAVE_FORMAT_PCM, channels, samples,
		samples*blockAlign, blockAlign, bitsPerSample, sizeof(WAVEFORMATEX)};
	primary->SetFormat(&wfx);
	primary->Play(0, 0, DSBPLAY_LOOPING);
	//primary->Release();
	context=result;

	LeaveCriticalSection(&contextCritSect);
}

BufferHandle BufferCreate(size_t sampleCount, uint channels, uint sampleRate)
{
	if(sampleCount==0 || channels==0 || sampleRate==0) return null;
	init_context();
	if(context==null) return null;

	auto result = new Buffer(null, uint(sampleCount), sampleRate, channels);

	const ushort blockAlign = ushort(sizeof(ushort)*channels);
	WAVEFORMATEX wfx = {WAVE_FORMAT_PCM, ushort(channels), sampleRate, sampleRate*blockAlign, blockAlign, 16, sizeof(WAVEFORMATEX)};
	const DSBUFFERDESC dsbd={sizeof(DSBUFFERDESC),
		DSBCAPS_GLOBALFOCUS|DSBCAPS_CTRLFREQUENCY|DSBCAPS_CTRLPAN|DSBCAPS_CTRLPOSITIONNOTIFY|DSBCAPS_CTRLVOLUME/*|DSBCAPS_CTRL3D*/,
		result->SizeInBytes(), 0, &wfx, {}};
	if(FAILED(context->CreateSoundBuffer(&dsbd, &result->buffer, null))) return null;

	return result;
}

void BufferSetDataInterleaved(BufferHandle snd, const void* data, ValueType type)
{
	if(snd==null || data==null) return;
	auto lockedData = reinterpret_cast<short*>(BufferLock(snd));
	if(type==ValueType::Short)
		core::memcpy(lockedData, data, snd->sampleCount*type.Size());
	else if(type==ValueType::Float)
	{
		for(size_t i=0; i<snd->sampleCount; i++)
			lockedData[i] = short((reinterpret_cast<const float*>(data))[i]*32767.5f-0.5f);
	}
	BufferUnlock(snd);
}

void BufferSetDataChannels(BufferHandle snd, const void* const* data, ValueType type)
{
	if(snd->channels==1)
	{
		BufferSetDataInterleaved(snd, data[0], type);
		return;
	}

	auto lockedData = reinterpret_cast<short*>(BufferLock(snd));
	if(type==ValueType::Short)
	{
		for(size_t i=0, j=0; i<snd->sampleCount; i++)
			for(uint c=0; c<snd->channels; c++)
			{
				const short* const channelSamples = reinterpret_cast<const short*>(data[c]);
				lockedData[j++] = channelSamples[i];
			}
	}
	else if(type==ValueType::Float)
	{
		for(size_t i=0, j=0; i<snd->sampleCount; i++)
			for(uint c=0; c<snd->channels; c++)
			{
				const float* const channelSamples = reinterpret_cast<const float*>(data[c]);
				lockedData[j++] = short(channelSamples[i]*32767.5f-0.5f);
			}
	}
	BufferUnlock(snd);
}

void* BufferLock(BufferHandle snd)
{
	INTRA_ASSERT(snd!=null);
	if(snd==null) return null;
	DWORD lockedSize;
	snd->buffer->Lock(0, snd->SizeInBytes(), &snd->lockedBits, &lockedSize, null, null, 0);
	INTRA_ASSERT(lockedSize==snd->SizeInBytes());
	return snd->lockedBits;
}

void BufferUnlock(BufferHandle snd)
{
	snd->buffer->Unlock(snd->lockedBits, snd->SizeInBytes(), null, 0);
}

void BufferDelete(BufferHandle snd)
{
	if(context!=null) snd->buffer->Release();
	delete snd;
}



static void CALLBACK DeleteInstanceOnStopCallback(_In_  void* lpParameter, _In_  byte /*timerOrWaitFired*/)
{
	auto impl = reinterpret_cast<InstanceHandle>(lpParameter);
	if(impl->deleteOnStop) if(context!=null) InstanceDelete(impl);
}

InstanceHandle InstanceCreate(BufferHandle snd)
{
	if(snd==null) return null;
	IDirectSoundBuffer* bufferDup;
	context->DuplicateSoundBuffer(snd->buffer, &bufferDup);
	auto result = new Instance(bufferDup, snd);

	IDirectSoundNotify* notify; 
	if(SUCCEEDED(bufferDup->QueryInterface(IID_IDirectSoundNotify, reinterpret_cast<void**>(&notify))))
	{
		result->notifyOnStopEvent = CreateEventW(null, false, false, null);
		if(result->notifyOnStopEvent==null)
		{
			delete result;
			return null;
		}
		const DSBPOSITIONNOTIFY stopNotify = {DWORD(DSBPN_OFFSETSTOP), result->notifyOnStopEvent};
		RegisterWaitForSingleObject(&result->notifyOnStopWait,
			result->notifyOnStopEvent, DeleteInstanceOnStopCallback, result, INFINITE, 0);
		notify->SetNotificationPositions(1, &stopNotify);
		notify->Release();
	}
	return result;
}

void InstanceSetDeleteOnStop(InstanceHandle si, bool del)
{
	si->deleteOnStop = del;
}

void InstanceDelete(InstanceHandle si)
{
	(void)UnregisterWait(si->notifyOnStopWait);
	CloseHandle(si->notifyOnStopEvent);
	if(context!=null && TryEnterCriticalSection(&contextCritSect))
	{
		si->dupBuffer->Stop();
		si->dupBuffer->Release();
		LeaveCriticalSection(&contextCritSect);
	}
	delete si;
}

void InstancePlay(InstanceHandle si, bool loop)
{
	si->dupBuffer->Play(0, 0, DSBPLAY_LOOPING*uint(loop));
}

bool InstanceIsPlaying(InstanceHandle si)
{
	DWORD status;
	if(FAILED( si->dupBuffer->GetStatus(&status) )) return false;
	return (status & DSBSTATUS_PLAYING)!=0;
}

void InstanceStop(InstanceHandle si)
{
	if(si==null) return;
	si->dupBuffer->Stop();
}





static void CALLBACK WaitLoadCallback(_In_  void* lpParameter, _In_  byte /*timerOrWaitFired*/)
{
	auto snd = reinterpret_cast<StreamedBufferHandle>(lpParameter);
	EnterCriticalSection(&snd->critsec);
	snd->buffers_processed++;
	//StreamedSoundUpdate(snd);
	LeaveCriticalSection(&snd->critsec);
}

StreamedBufferHandle StreamedBufferCreate(size_t sampleCount,
	uint channels, uint sampleRate, StreamingCallback callback)
{
	if(sampleCount==0 || channels==0 || sampleRate==0 || callback.CallbackFunction==null) return null;
	init_context();

	const auto result = new StreamedBuffer(null, callback, uint(sampleCount), sampleRate, channels);

	const ushort blockAlign = ushort(sizeof(ushort)*channels);
	WAVEFORMATEX wfx = {WAVE_FORMAT_PCM, ushort(channels), sampleRate, sampleRate*blockAlign, blockAlign, 16, sizeof(WAVEFORMATEX)};
	const DSBUFFERDESC dsbd = {sizeof(DSBUFFERDESC), DSBCAPS_GLOBALFOCUS|DSBCAPS_CTRLPOSITIONNOTIFY, result->SizeInBytes()*2, 0, &wfx, {}};
	if(FAILED(context->CreateSoundBuffer(&dsbd, &result->buffer, null))) return null;


	DWORD lockedSize; void* lockedData;
	result->buffer->Lock(0, result->SizeInBytes()*2, &lockedData, &lockedSize, null, null, 0);
	callback.CallbackFunction(&lockedData, channels, ValueType::Short, true, sampleCount, callback.CallbackData);
	result->buffer->Unlock(lockedData, result->SizeInBytes()*2, null, 0);

	IDirectSoundNotify* notify;
	if(SUCCEEDED(result->buffer->QueryInterface(IID_IDirectSoundNotify, reinterpret_cast<void**>(&notify))))
	{
		result->notifyLoadEvent = CreateEventW(null, false, false, null);
		if(result->notifyLoadEvent==null)
		{
			delete result;
			return null;
		}

		DSBPOSITIONNOTIFY positionNotify[2] = {
			{result->SizeInBytes(), result->notifyLoadEvent},
			{result->SizeInBytes()*2-1, result->notifyLoadEvent}
		};
		RegisterWaitForSingleObject(&result->notifyLoadWait, result->notifyLoadEvent, WaitLoadCallback, result, INFINITE, 0);
		notify->SetNotificationPositions(2, positionNotify);
		notify->Release();
	}

	return result;
}

void StreamedBufferSetDeleteOnStop(StreamedBufferHandle snd, bool del)
{
	snd->deleteOnStop = del;
}

void StreamedSoundPlay(StreamedBufferHandle snd, bool loop)
{
	INTRA_ASSERT(snd==null);
	snd->looping = loop;
	snd->stop_soon = 0;
	snd->buffer->Play(0, 0, DSBPLAY_LOOPING);
}

bool StreamedSoundIsPlaying(StreamedBufferHandle snd)
{
	DWORD status;
	if(snd==null || FAILED( snd->buffer->GetStatus(&status) )) return false;
	return (status & DSBSTATUS_PLAYING)!=0;
}

void StreamedSoundStop(StreamedBufferHandle snd)
{
	if(snd==null) return;
	snd->buffer->Stop();
	if(snd->deleteOnStop) StreamedBufferDelete(snd);
}

void StreamedBufferDelete(StreamedBufferHandle snd)
{
	(void)UnregisterWait(snd->notifyLoadWait);
	if(TryEnterCriticalSection(&contextCritSect))
	{
		if(context!=null)
		{
		EnterCriticalSection(&snd->critsec);
			CloseHandle(snd->notifyLoadEvent);
			snd->buffer->Stop();
			snd->buffer->Release();
		LeaveCriticalSection(&snd->critsec);
		}
		LeaveCriticalSection(&contextCritSect);
	}
	delete snd;
}

AnyPtr lock_buffer(StreamedBufferHandle snd, uint no)
{
	size_t lockSampleStart = no==0? 0: snd->sampleCount;
	HRESULT lockResult = snd->buffer->Lock(uint(lockSampleStart*sizeof(short)*snd->channels),
		snd->SizeInBytes(), &snd->lockedBits, &snd->lockedSize, &snd->lockedBits2, &snd->lockedSize2, 0);
	INTRA_ASSERT(!FAILED(lockResult));
	if(FAILED(lockResult)) return null;
	return snd->lockedBits;
}

void unlock_buffer(StreamedBufferHandle snd)
{
	snd->buffer->Unlock(snd->lockedBits, snd->lockedSize, snd->lockedBits2, snd->lockedSize2);
}

void fill_next_buffer_data(StreamedBufferHandle snd)
{
	short* data = lock_buffer(snd, snd->next_buffer_to_fill);
	if(data==null) return;

	if(snd->stop_soon!=0)
	{
		//Достигнут конец потока данных, поэтому зануляем оставшийся буфер. Когда он закончится, воспроизведение будет остановлено
		core::memset(data, 0, snd->SizeInBytes());
		unlock_buffer(snd);
		return;
	}

	const size_t samplesRead = snd->streamingCallback.CallbackFunction(reinterpret_cast<void**>(&data), snd->channels,
		ValueType::Short, true, snd->sampleCount, snd->streamingCallback.CallbackData);
	if(samplesRead<snd->sampleCount)
	{
		void* ptr = data+samplesRead;
		if(snd->looping)
		{
			snd->streamingCallback.CallbackFunction(&ptr, 1, ValueType::Short, true,
				uint(snd->sampleCount-samplesRead), snd->streamingCallback.CallbackData);
		}
		else
		{
			core::memset(ptr, 0, (snd->sampleCount-samplesRead)*snd->channels*sizeof(short));
			snd->stop_soon=1;
		}
	}
	unlock_buffer(snd);
}

void StreamedSoundUpdate(StreamedBufferHandle snd)
{
	/*DWORD playPos;
	snd->buffer->GetCurrentPosition(&playPos, null);
	size_t updateSampleStart;
	byte bufferToLock;
	if(playPos>=snd->SizeInBytes())
	{
		updateSampleStart=0;
		//if(snd->last_playing_buffer==1) return;
		//snd->last_playing_buffer=1;
		bufferToLock=0;
	}
	else
	{
		updateSampleStart = snd->sampleCount;
		//if(snd->last_playing_buffer==0) return;
		//snd->last_playing_buffer=0;
		bufferToLock=1;
	}*/

	if(snd->stop_soon==2)
	{
		snd->stop_soon=0;
		StreamedSoundStop(snd);
		return;
	}

	int bufsProcessed = snd->buffers_processed;
	snd->buffers_processed = 0;
	while(bufsProcessed!=0)
	{
		fill_next_buffer_data(snd);
		snd->next_buffer_to_fill = byte(1 - snd->next_buffer_to_fill);
		bufsProcessed--;
	}
}


}}
#endif
