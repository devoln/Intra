#include "Cpp/PlatformDetect.h"
#include "Audio/SoundApi.h"
#include "Cpp/Warnings.h"
#include "Utils/AnyPtr.h"

#if(INTRA_LIBRARY_SOUND_SYSTEM==INTRA_LIBRARY_SOUND_SYSTEM_DirectSound)


#define INITGUID

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

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

#if(_MSC_VER<1900)
#pragma warning(disable: 4351)
#endif

#endif


namespace Intra { namespace Audio { namespace SoundAPI {

const Data::ValueType::I InternalBufferType = Data::ValueType::Short;
const int InternalChannelsInterleaved = true;
uint InternalSampleRate() {return 48000;}

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
	StreamedBuffer(IDirectSoundBuffer* buf, StreamingCallback callback, uint sampleCount, uint sampleRate, uint ch):
		mBuffer(buf),
		mSampleCount(sampleCount),
		mSampleRate(sampleRate),
		mChannels(ch),
		mStreamingCallback(callback),
		mNotifyLoadEvents{},
		mNotifyLoadWaits{}
	{
		InitializeCriticalSection(&mCritsec);
	}

	//! Размер в байтах половины буфера
	uint SizeInBytes() const {return uint(mSampleCount*mChannels*sizeof(short));}

	IDirectSoundBuffer* mBuffer;
	uint mSampleCount; //Размер половины буфера
	uint mSampleRate;
	uint mChannels;
	StreamingCallback mStreamingCallback;

	HANDLE mNotifyLoadEvents[2];
	HANDLE mNotifyLoadWaits[2];
	bool mDeleteOnStop = false;
	bool mLooping = false;
	byte mStopSoon = 0;

	byte mBuffersProcessed = 0;
	byte mNextBufferToFill = 1;

	CRITICAL_SECTION mCritsec;
	DWORD mLockedSize = 0;
	void* mLockedBits = null;
	DWORD mLockedSize2 = 0;
	void* mLockedBits2 = null;
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
		clean();
	}

	void clean()
	{
		if(context==null) return;
		EnterCriticalSection(&contextCritSect);
		auto cont = context;
		context = null;
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

void BufferSetDataInterleaved(BufferHandle snd, const void* data, Data::ValueType type)
{
	if(snd==null || data==null) return;
	auto lockedData = static_cast<short*>(BufferLock(snd));
	if(type == Data::ValueType::Short)
		memcpy(lockedData, data, snd->sampleCount*type.Size());
	else if(type == Data::ValueType::Float)
	{
		for(size_t i=0; i<snd->sampleCount; i++)
			lockedData[i] = short((static_cast<const float*>(data))[i]*32767.5f-0.5f);
	}
	BufferUnlock(snd);
}

void BufferSetDataChannels(BufferHandle snd, const void* const* data, Data::ValueType type)
{
	if(snd->channels==1)
	{
		BufferSetDataInterleaved(snd, data[0], type);
		return;
	}

	auto lockedData = static_cast<short*>(BufferLock(snd));
	if(type == Data::ValueType::Short)
	{
		for(size_t i=0, j=0; i<snd->sampleCount; i++)
			for(uint c=0; c<snd->channels; c++)
			{
				const short* const channelSamples = static_cast<const short*>(data[c]);
				lockedData[j++] = channelSamples[i];
			}
	}
	else if(type == Data::ValueType::Float)
	{
		for(size_t i=0, j=0; i<snd->sampleCount; i++)
			for(uint c=0; c<snd->channels; c++)
			{
				const float* const channelSamples = static_cast<const float*>(data[c]);
				lockedData[j++] = short(channelSamples[i]*32767.5f-0.5f);
			}
	}
	BufferUnlock(snd);
}

void* BufferLock(BufferHandle snd)
{
	INTRA_DEBUG_ASSERT(snd!=null);
	if(snd==null) return null;
	DWORD lockedSize;
	snd->buffer->Lock(0, snd->SizeInBytes(), &snd->lockedBits, &lockedSize, null, null, 0);
	INTRA_DEBUG_ASSERT(lockedSize==snd->SizeInBytes());
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
	auto impl = static_cast<InstanceHandle>(lpParameter);
	if(impl->deleteOnStop) if(context!=null) InstanceDelete(impl);
}

InstanceHandle InstanceCreate(BufferHandle snd)
{
	if(snd==null) return null;
	IDirectSoundBuffer* bufferDup = null;
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
	auto snd = static_cast<StreamedBufferHandle>(lpParameter);
	EnterCriticalSection(&snd->mCritsec);
	snd->mBuffersProcessed++;
	//StreamedSoundUpdate(snd);
	LeaveCriticalSection(&snd->mCritsec);
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
	if(FAILED(context->CreateSoundBuffer(&dsbd, &result->mBuffer, null))) return null;


	DWORD lockedSize; void* lockedData;
	result->mBuffer->Lock(0, result->SizeInBytes()*2, &lockedData, &lockedSize, null, null, 0);
	callback.CallbackFunction(&lockedData, channels, Data::ValueType::Short, true, sampleCount, callback.CallbackData);
	result->mBuffer->Unlock(lockedData, result->SizeInBytes()*2, null, 0);

	IDirectSoundNotify* notify;
	if(SUCCEEDED(result->mBuffer->QueryInterface(IID_IDirectSoundNotify, reinterpret_cast<void**>(&notify))))
	{
		result->mNotifyLoadEvents[0] = CreateEventW(null, false, false, null);
		result->mNotifyLoadEvents[1] = CreateEventW(null, false, false, null);
		if(result->mNotifyLoadEvents[0]==null || result->mNotifyLoadEvents[1]==null)
		{
			delete result;
			return null;
		}

		DSBPOSITIONNOTIFY positionNotify[2] = {
			{0, result->mNotifyLoadEvents[0]},
			{result->SizeInBytes(), result->mNotifyLoadEvents[1]}
		};
		RegisterWaitForSingleObject(&result->mNotifyLoadWaits[0], result->mNotifyLoadEvents[0], WaitLoadCallback, result, INFINITE, 0);
		RegisterWaitForSingleObject(&result->mNotifyLoadWaits[1], result->mNotifyLoadEvents[1], WaitLoadCallback, result, INFINITE, 0);
		notify->SetNotificationPositions(2, positionNotify);
		notify->Release();
	}

	return result;
}

void StreamedBufferSetDeleteOnStop(StreamedBufferHandle snd, bool del)
{
	snd->mDeleteOnStop = del;
}

void StreamedSoundPlay(StreamedBufferHandle snd, bool loop)
{
	INTRA_DEBUG_ASSERT(snd!=null);
	snd->mLooping = loop;
	snd->mStopSoon = 0;
	snd->mBuffer->Play(0, 0, DSBPLAY_LOOPING);
}

bool StreamedSoundIsPlaying(StreamedBufferHandle snd)
{
	DWORD status = 0;
	if(snd==null || FAILED( snd->mBuffer->GetStatus(&status) )) return false;
	return (status & DSBSTATUS_PLAYING)!=0;
}

void StreamedSoundStop(StreamedBufferHandle snd)
{
	if(snd==null) return;
	snd->mBuffer->Stop();
	if(snd->mDeleteOnStop) StreamedBufferDelete(snd);
}

void StreamedBufferDelete(StreamedBufferHandle snd)
{
	(void)UnregisterWait(snd->mNotifyLoadWaits[0]);
	(void)UnregisterWait(snd->mNotifyLoadWaits[1]);
	if(TryEnterCriticalSection(&contextCritSect))
	{
		if(context!=null)
		{
		EnterCriticalSection(&snd->mCritsec);
			CloseHandle(snd->mNotifyLoadEvents[0]);
			CloseHandle(snd->mNotifyLoadEvents[1]);
			snd->mBuffer->Stop();
			snd->mBuffer->Release();
		LeaveCriticalSection(&snd->mCritsec);
		}
		LeaveCriticalSection(&contextCritSect);
	}
	delete snd;
}

AnyPtr lock_buffer(StreamedBufferHandle snd, uint no)
{
	const size_t lockSampleStart = no==0? 0: snd->mSampleCount;
	const HRESULT lockResult = snd->mBuffer->Lock(uint(lockSampleStart*sizeof(short)*snd->mChannels),
		snd->SizeInBytes(), &snd->mLockedBits, &snd->mLockedSize, &snd->mLockedBits2, &snd->mLockedSize2, 0);
	INTRA_DEBUG_ASSERT(!FAILED(lockResult));
	if(FAILED(lockResult)) return null;
	return snd->mLockedBits;
}

void unlock_buffer(StreamedBufferHandle snd)
{
	snd->mBuffer->Unlock(snd->mLockedBits, snd->mLockedSize, snd->mLockedBits2, snd->mLockedSize2);
}

void fill_next_buffer_data(StreamedBufferHandle snd)
{
	short* data = lock_buffer(snd, snd->mNextBufferToFill);
	if(data==null) return;

	if(snd->mStopSoon!=0)
	{
		//Достигнут конец потока данных, поэтому зануляем оставшийся буфер. Когда он закончится, воспроизведение будет остановлено
		memset(data, 0, snd->SizeInBytes());
		unlock_buffer(snd);
		return;
	}

	const size_t samplesRead = snd->mStreamingCallback.CallbackFunction(reinterpret_cast<void**>(&data), snd->mChannels,
		Data::ValueType::Short, true, snd->mSampleCount, snd->mStreamingCallback.CallbackData);
	if(samplesRead<snd->mSampleCount)
	{
		void* ptr = data+samplesRead;
		if(snd->mLooping)
		{
			snd->mStreamingCallback.CallbackFunction(&ptr, 1, Data::ValueType::Short, true,
				uint(snd->mSampleCount-samplesRead), snd->mStreamingCallback.CallbackData);
		}
		else
		{
			memset(ptr, 0, (snd->mSampleCount-samplesRead)*snd->mChannels*sizeof(short));
			snd->mStopSoon=1;
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

	if(snd->mStopSoon == 2)
	{
		snd->mStopSoon = 0;
		StreamedSoundStop(snd);
		return;
	}

	int bufsProcessed = snd->mBuffersProcessed;
	snd->mBuffersProcessed = 0;
	while(bufsProcessed != 0)
	{
		fill_next_buffer_data(snd);
		if(snd->mNextBufferToFill == 1) snd->mNextBufferToFill = 0;
		else snd->mNextBufferToFill = 1;
		bufsProcessed--;
	}
}

void SoundSystemCleanUp()
{
	context_deleter.clean();
}


}}}

INTRA_WARNING_POP

#else

INTRA_DISABLE_LNK4221

#endif
