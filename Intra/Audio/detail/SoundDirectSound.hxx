#pragma once

#include "Cpp/Warnings.h"

#include "Utils/AnyPtr.h"
#include "Utils/Finally.h"

#include "Range/Mutation/Cast.h"
#include "Range/Mutation/Fill.h"

#include "Concurrency/Atomic.h"
#include "Concurrency/Mutex.h"

#include "Audio/Sound.h"
#include "Audio/AudioSource.h"

#include "SoundBasicData.hxx"

#include <stdio.h>

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
#include <Windows.h>
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


namespace Intra { namespace Audio {

using Data::ValueType;

uint Sound::DefaultSampleRate() {return 48000;}


struct SoundContext: detail::SoundBasicContext
{
private:
	SoundContext()
	{
		if(FAILED(DirectSoundCreate8(null, &mContext, null))) return;
		mContext->SetCooperativeLevel(GetDesktopWindow(), DSSCL_PRIORITY);

		const DSBUFFERDESC dsbd = {
			sizeof(DSBUFFERDESC), DSBCAPS_PRIMARYBUFFER|DSBCAPS_LOCSOFTWARE, 0, 0, null, {}
		};
		if(FAILED(mContext->CreateSoundBuffer(&dsbd, &mPrimary, null)))
		{
			mContext->Release();
			mContext = null;
			return;
		}
		const uint samples = Sound::DefaultSampleRate();
		const ushort channels = 2;
		const ushort bitsPerSample = 16;
		const ushort blockAlign = ushort(bitsPerSample/8*channels);
		const WAVEFORMATEX wfx = {WAVE_FORMAT_PCM, channels, samples,
			samples*blockAlign, blockAlign, bitsPerSample, sizeof(WAVEFORMATEX)};
		mPrimary->SetFormat(&wfx);
		mPrimary->Play(0, 0, DSBPLAY_LOOPING);
	}

	~SoundContext()
	{
		ReleaseAllSounds();
		ReleaseAllStreamedSounds();
		INTRA_SYNCHRONIZED(MyMutex)
		{
			mPrimary->Stop();
			mPrimary->Release();
			mContext->Release();
		}
	}

	SoundContext(const SoundContext&) = delete;
	SoundContext& operator=(const SoundContext&) = delete;

	IDirectSound8* mContext = null;
	IDirectSoundBuffer* mPrimary = null;


public:
	static SoundContext& Instance()
	{
		static SoundContext instance;
		return instance;
	}

	static forceinline IDirectSound8* Device() {return Instance().mContext;}

	void ReleaseAllSounds();
	void ReleaseAllStreamedSounds();
};

struct Sound::Data: SharedClass<Sound::Data>, detail::SoundBasicData{
	Data(IAudioSource& src): SoundBasicData(src)
	{
		INTRA_DEBUG_ASSERT(src.SampleCount() > 0);
		Info.SampleType = ValueType::SNorm16;
		const ushort blockAlign = ushort(sizeof(short)*Info.Channels);
		WAVEFORMATEX wfx = {
			WAVE_FORMAT_PCM, ushort(Info.Channels), Info.SampleRate,
			Info.SampleRate*blockAlign, blockAlign, 16, sizeof(WAVEFORMATEX)
		};
		const DSBUFFERDESC dsbd = {sizeof(DSBUFFERDESC),
			DSBCAPS_GLOBALFOCUS|DSBCAPS_CTRLFREQUENCY|DSBCAPS_CTRLPAN|
			DSBCAPS_CTRLPOSITIONNOTIFY|DSBCAPS_CTRLVOLUME/*|DSBCAPS_CTRL3D*/,
			Info.GetBufferSize(), 0, &wfx, {}};
		auto& context = SoundContext::Instance();
		if(!context.Device()) return;
		if(FAILED(context.Device()->CreateSoundBuffer(&dsbd, &Buffer, null))) return;

		void* dstData;
		DWORD lockedSize;
		Buffer->Lock(0, Info.GetBufferSize(), &dstData, &lockedSize, null, null, 0);
		INTRA_DEBUG_ASSERT(lockedSize == Info.GetBufferSize());
		auto dst = SpanOfRawElements<short>(dstData, Info.SampleCount*Info.Channels);
		src.GetInterleavedSamples(dst);
		Buffer->Unlock(dstData, Info.GetBufferSize(), null, 0);

		INTRA_SYNCHRONIZED(context.MyMutex)
			context.AllSounds.AddLast(this);
	}

	forceinline ~Data() {Release();}

	Data(const Data&) = delete;
	Data& operator=(const Data&) = delete;

	void SetDataInterleaved(const void* data, ValueType type)
	{
		if(data == null) return;
		void* dstData;
		DWORD lockedSize;
		Buffer->Lock(0, Info.GetBufferSize(), &dstData, &lockedSize, null, null, 0);
		INTRA_DEBUG_ASSERT(lockedSize == Info.GetBufferSize());
		auto dst = SpanOfRawElements<short>(dstData, Info.SampleCount*Info.Channels);
		INTRA_FINALLY(Buffer->Unlock(dstData, Info.GetBufferSize(), null, 0));
		if(type == ValueType::SNorm16)
		{
			auto src = SpanOfRawElements<short>(data, dst.Length());
			CopyTo(src, dst);
			return;
		}
		if(type == ValueType::Float)
		{
			auto src = SpanOfRawElements<float>(data, dst.Length());
			CastFromNormalized(dst, src);
		}
	}

	void Release();

	bool IsReleased()
	{
		bool result = false;
		INTRA_SYNCHRONIZED(MyMutex)
		{
			result = (Buffer == null);
		}
		return result;
	}

	forceinline void Update() {}

	IDirectSoundBuffer* Buffer;

	void* LockedBits = null;

	Array<Sound::Instance::Data*> Instances;
};


struct Sound::Instance::Data: SharedClass<Sound::Instance::Data>, detail::SoundInstanceBasicData
{
	static void CALLBACK OnStopCallback(_In_ void* lpParameter, _In_ byte /*timerOrWaitFired*/)
	{
		auto impl = static_cast<Sound::Instance::Data*>(lpParameter);
		INTRA_SYNCHRONIZED(impl->MyMutex)
		{
			if(impl->OnStop) impl->OnStop();
		}
		impl->SelfRef = null;
	}

	Data(Shared<Sound::Data> parent): SoundInstanceBasicData(Cpp::Move(parent))
	{
		if(FAILED(SoundContext::Device()->DuplicateSoundBuffer(Parent->Buffer, &DupBuffer)))
		{
			return;
		}

		IDirectSoundNotify* notify;
		if(FAILED(DupBuffer->QueryInterface(IID_IDirectSoundNotify, reinterpret_cast<void**>(&notify))))
		{
			DupBuffer->Release();
			DupBuffer = null;
			return;
		}

		NotifyOnStopEvent = CreateEventW(null, false, false, null);
		if(NotifyOnStopEvent == null)
		{
			DupBuffer->Release();
			DupBuffer = null;
			return;
		}
		const DSBPOSITIONNOTIFY stopNotify = {DWORD(DSBPN_OFFSETSTOP), NotifyOnStopEvent};
		RegisterWaitForSingleObject(&NotifyOnStopWait,
			NotifyOnStopEvent, OnStopCallback, this, INFINITE, 0);
		notify->SetNotificationPositions(1, &stopNotify);
		notify->Release();

		INTRA_SYNCHRONIZED(Parent->MyMutex)
		{
			Parent->Instances.AddLast(this);
		}
	}

	~Data()
	{
		UnregisterWaitEx(NotifyOnStopWait, INVALID_HANDLE_VALUE);
		CloseHandle(NotifyOnStopEvent);
		Release();
	}

	Data(const Data&) = delete;
	Data& operator=(const Data&) = delete;

	void Release()
	{
		INTRA_SYNCHRONIZED(MyMutex)
		{
			if(!DupBuffer) return;
			DupBuffer->Stop();
			DupBuffer->Release();
			DupBuffer = null;
		}
		SelfRef = null;
	}

	bool IsReleased()
	{
		bool result = false;
		INTRA_SYNCHRONIZED(MyMutex)
		{
			result = (DupBuffer == null);
		}
		return result;
	}

	void Play(bool loop)
	{
		INTRA_SYNCHRONIZED(MyMutex)
		{
			if(!DupBuffer) return;
			SelfRef = SharedThis();
			DupBuffer->Play(0, 0, loop? DSBPLAY_LOOPING: 0u);
		}
	}

	bool IsPlayingST()
	{
		DWORD status;
		if(FAILED(DupBuffer->GetStatus(&status))) return false;
		return (status & DSBSTATUS_PLAYING) != 0;
	}

	bool IsPlaying()
	{
		DWORD status = 0;
		HRESULT hr = 0;
		INTRA_SYNCHRONIZED(MyMutex)
		{
			hr = DupBuffer->GetStatus(&status);
		}
		if(FAILED(hr)) return false;
		return (status & DSBSTATUS_PLAYING) != 0;
	}

	void Stop()
	{
		INTRA_SYNCHRONIZED(MyMutex)
		{
			if(!DupBuffer) return;
			DupBuffer->Stop();
		}
	}

	RecursiveMutex MyMutex;

	IDirectSoundBuffer* DupBuffer;

	HANDLE NotifyOnStopEvent = null;
	HANDLE NotifyOnStopWait = null;
};


struct StreamedSound::Data: SharedClass<StreamedSound::Data>, detail::StreamedSoundBasicData
{
	Data(Unique<IAudioSource> source, size_t bufferSampleCount, bool autoStreamingEnabled = true):
		StreamedSoundBasicData(Cpp::Move(source), bufferSampleCount),
		AutoStreamingEnabled(autoStreamingEnabled)
	{
		INTRA_DEBUG_ASSERT(Source != null);
		auto& context = SoundContext::Instance();
		if(!context.Device()) return;

		const ushort blockAlign = ushort(sizeof(ushort) * Source->ChannelCount());
		WAVEFORMATEX wfx = {
			WAVE_FORMAT_PCM, ushort(Source->ChannelCount()), Source->SampleRate(),
			Source->SampleRate()*blockAlign, blockAlign, 16, sizeof(WAVEFORMATEX)
		};
		const DSBUFFERDESC dsbd = {
			sizeof(DSBUFFERDESC), DSBCAPS_GLOBALFOCUS|DSBCAPS_CTRLPOSITIONNOTIFY,
			GetBufferSize()*2, 0, &wfx, {}
		};

		if(FAILED(context.Device()->CreateSoundBuffer(&dsbd, &Buffer, null))) return;

		DWORD lockedSize; void* lockedData;
		Buffer->Lock(0, GetBufferSize()*2, &lockedData, &lockedSize, null, null, 0);
		const auto dst = SpanOfRaw<short>(lockedData, lockedSize);
		if(Source) Source->GetInterleavedSamples(dst);
		else FillZeros(dst);
		Buffer->Unlock(lockedData, lockedSize, null, 0);

		IDirectSoundNotify* notify;
		if(FAILED(Buffer->QueryInterface(IID_IDirectSoundNotify, reinterpret_cast<void**>(&notify))))
			return;

		NotifyLoadEvents[0] = CreateEventW(null, false, false, null);
		NotifyLoadEvents[1] = CreateEventW(null, false, false, null);
		if(NotifyLoadEvents[0] == null || NotifyLoadEvents[1] == null)
			return;

		const DSBPOSITIONNOTIFY positionNotify[2] = {
			{0, NotifyLoadEvents[0]},
			{GetBufferSize(), NotifyLoadEvents[1]}
		};
		RegisterWaitForSingleObject(&NotifyLoadWaits[0], NotifyLoadEvents[0], WaitLoadCallback, this, INFINITE, 0);
		RegisterWaitForSingleObject(&NotifyLoadWaits[1], NotifyLoadEvents[1], WaitLoadCallback, this, INFINITE, 0);
		notify->SetNotificationPositions(2, positionNotify);
		notify->Release();

		INTRA_SYNCHRONIZED(context.MyMutex) context.AllStreamedSounds.AddLast(this);
	}

	Data(const Data&) = delete;
	Data& operator=(const Data&) = delete;

	~Data()
	{
		(void)UnregisterWait(NotifyLoadWaits[0]);
		(void)UnregisterWait(NotifyLoadWaits[1]);
		CloseHandle(NotifyLoadEvents[0]);
		CloseHandle(NotifyLoadEvents[1]);
		Release();
	}

	void Release()
	{
		INTRA_SYNCHRONIZED(MyMutex)
		{
			if(!Buffer) return;
			Buffer->Stop();
			Buffer->Release();
			Buffer = null;
		}
		auto& context = SoundContext::Instance();
		INTRA_SYNCHRONIZED(context.MyMutex)
		{
			context.AllStreamedSounds.FindAndRemoveUnordered(this);
		}
		SelfRef = null;
	}

	bool IsReleased()
	{
		bool result = false;
		INTRA_SYNCHRONIZED(MyMutex)
		{
			result = (Buffer == null);
		}
		return result;
	}

	void Play(bool loop)
	{
		INTRA_SYNCHRONIZED(MyMutex)
		{
			Looping = loop;
			StopSoon = 0;
			SelfRef = SharedThis();
			Buffer->Play(0, 0, DSBPLAY_LOOPING);
		}
	}

	bool IsPlaying()
	{
		DWORD status = 0;
		HRESULT hr = 0;
		INTRA_SYNCHRONIZED(MyMutex)
		{
			hr = Buffer->GetStatus(&status);
		}
		if(FAILED(hr)) return false;
		return (status & DSBSTATUS_PLAYING) != 0;
	}

	bool IsPlayingST()
	{
		DWORD status = 0;
		if(FAILED(Buffer->GetStatus(&status))) return false;
		return (status & DSBSTATUS_PLAYING) != 0;
	}

	void Stop()
	{
		INTRA_SYNCHRONIZED(MyMutex)
		{
			if(!IsPlayingST()) return;
			Buffer->Stop();
		}
	}

	static void CALLBACK WaitLoadCallback(_In_ void* lpParameter, _In_  byte /*timerOrWaitFired*/)
	{
		auto snd = static_cast<StreamedSound::Data*>(lpParameter);
		snd->BuffersProcessed.Increment();
		if(snd->AutoStreamingEnabled) snd->Update();
	}

	AnyPtr lock(uint no, void*& lockedBits, DWORD& lockedSize, void*& lockedBits2, DWORD& lockedSize2)
	{
		const size_t lockSampleStart = no == 0? 0: BufferSampleCount;
		const HRESULT lockResult = Buffer->Lock(uint(lockSampleStart*sizeof(short)*Source->ChannelCount()),
			GetBufferSize(), &lockedBits, &lockedSize, &lockedBits2, &lockedSize2, 0);
		INTRA_DEBUG_ASSERT(!FAILED(lockResult));
		if(FAILED(lockResult)) return null;
		return lockedBits;
	}

	void unlock(void* lockedBits, DWORD lockedSize, void* lockedBits2, DWORD lockedSize2)
	{
		Buffer->Unlock(lockedBits, lockedSize, lockedBits2, lockedSize2);
	}

	void fill_next_buffer_data()
	{
		void* lockedBits;
		DWORD lockedSize;
		void* lockedBits2;
		DWORD lockedSize2;
		short* data = lock(NextBufferToFill, lockedBits, lockedSize, lockedBits2, lockedSize2);
		if(data == null) return;

		auto buffer = SpanOfRaw<short>(data, GetBufferSize());
		if(StopSoon != 0)
		{
			//Достигнут конец потока данных, поэтому зануляем оставшийся буфер.
			//Когда он закончится, воспроизведение будет остановлено.
			FillZeros(buffer);
			unlock(lockedBits, lockedSize, lockedBits2, lockedSize2);
			SelfRef = null;
			return;
		}

		const size_t samplesRead = Source->GetInterleavedSamples(buffer);
		if(samplesRead == BufferSampleCount)
		{
			unlock(lockedBits, lockedSize, lockedBits2, lockedSize2);
			return;
		}

		buffer.PopFirstExactly(samplesRead * Source->ChannelCount());
		if(!Looping)
		{
			FillZeros(buffer);
			StopSoon = 1;
			unlock(lockedBits, lockedSize, lockedBits2, lockedSize2);
			return;
		}
		Source->GetInterleavedSamples(buffer);
	}

	void UpdateST()
	{
		if(Buffer == null || StopSoon == 2)
		{
			StopSoon = 0;
			Stop();
			return;
		}

		int bufsProcessed = BuffersProcessed.GetSet(0);
		while(bufsProcessed --> 0)
		{
			fill_next_buffer_data();
			if(NextBufferToFill == 1) NextBufferToFill = 0;
			else NextBufferToFill = 1;
		}
	}

	void Update()
	{
		INTRA_SYNCHRONIZED(MyMutex) UpdateST();
	}

	IDirectSoundBuffer* Buffer;

	HANDLE NotifyLoadEvents[2]{};
	HANDLE NotifyLoadWaits[2]{};
	bool Looping = false;
	byte StopSoon = 0;

	AtomicInt BuffersProcessed{0};
	byte NextBufferToFill = 1;
	bool AutoStreamingEnabled = true;

	RecursiveMutex MyMutex;
};

void SoundContext::ReleaseAllSounds()
{
	Array<Sound::Data*> soundsToRelease;
	INTRA_SYNCHRONIZED(MyMutex)
	{
		soundsToRelease = Cpp::Move(AllSounds);
	}
	for(auto snd: soundsToRelease) snd->Release();
}

void SoundContext::ReleaseAllStreamedSounds()
{
	Array<StreamedSound::Data*> soundsToRelease;
	INTRA_SYNCHRONIZED(MyMutex)
	{
		soundsToRelease = Cpp::Move(AllStreamedSounds);
	}
	for(auto snd: soundsToRelease) snd->Release();
}

void Sound::Data::Release()
{
	INTRA_SYNCHRONIZED(MyMutex)
	{
		if(!Buffer) return;
		Buffer->Stop();
		Buffer->Release();
		Buffer = null;
		for(auto inst: Instances) inst->Release();
	}
	auto& context = SoundContext::Instance();
	INTRA_SYNCHRONIZED(context.MyMutex)
	{
		context.AllSounds.FindAndRemoveUnordered(this);
	}
}

}}

INTRA_WARNING_POP

