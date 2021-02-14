#pragma once

#include "Intra/Core.h"

#include "Intra/Range/Mutation/Fill.h"
#include "Intra/Range/Mutation/Copy.h"
#include "Intra/Range/Map.h"

#include "Intra/Concurrency/Atomic.h"
#include "IntraX/Concurrency/Mutex.h"

#include "IntraX/Unstable/Audio/Sound.h"
#include "IntraX/Unstable/Audio/AudioSource.h"

#include "SoundBasicData.hxx"

#include <stdio.h>

#define INITGUID

INTRA_PUSH_DISABLE_ALL_WARNINGS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
struct IUnknown;
#include <Windows.h>
#include <mmsystem.h>
#include <dsound.h>

#ifdef _MSC_VER
#pragma comment(lib, "dsound.lib")
#pragma comment(lib, "user32.lib")
#endif
INTRA_WARNING_POP

namespace Intra { INTRA_BEGIN
unsigned Sound::DefaultSampleRate() {return 48000;}

struct SoundContext: detail::SoundBasicContext
{
private:
	SoundContext()
	{
		if(FAILED(DirectSoundCreate8(nullptr, &mContext, nullptr))) return;
		mContext->SetCooperativeLevel(GetDesktopWindow(), DSSCL_PRIORITY);

		const DSBUFFERDESC dsbd = {
			sizeof(DSBUFFERDESC), DSBCAPS_PRIMARYBUFFER|DSBCAPS_LOCSOFTWARE, 0, 0, nullptr, {}
		};
		if(FAILED(mContext->CreateSoundBuffer(&dsbd, &mPrimary, nullptr)))
		{
			mContext->Release();
			mContext = nullptr;
			return;
		}
		const unsigned samples = Sound::DefaultSampleRate();
		const uint16 channels = 2;
		const uint16 bitsPerSample = 16;
		const uint16 blockAlign = uint16(bitsPerSample/8*channels);
		const WAVEFORMATEX wfx = {WAVE_FORMAT_PCM, channels, samples,
			samples*blockAlign, blockAlign, bitsPerSample, sizeof(WAVEFORMATEX)};
		mPrimary->SetFormat(&wfx);
		mPrimary->Play(0, 0, DSBPLAY_LOOPING);
	}

	~SoundContext()
	{
		/*ReleaseAllSounds();
		ReleaseAllStreamedSounds();
		INTRA_SYNCHRONIZED(MyMutex)
		{
			mPrimary->Stop();
			mPrimary->Release();
			mContext->Release();
		}*/
	}

	SoundContext(const SoundContext&) = delete;
	SoundContext& operator=(const SoundContext&) = delete;

	IDirectSound8* mContext = nullptr;
	IDirectSoundBuffer* mPrimary = nullptr;


public:
	static SoundContext& Instance()
	{
		INTRA_IGNORE_WARN_GLOBAL_CONSTRUCTION
		static SoundContext instance;
		return instance;
	}

	static INTRA_FORCEINLINE IDirectSound8* Device() {return Instance().mContext;}

	void ReleaseAllSounds();
	void ReleaseAllStreamedSounds();
};

struct Sound::Data: SharedClass<Sound::Data>, detail::SoundBasicData
{
	Data(IAudioSource& src): SoundBasicData(src)
	{
		INTRA_PRECONDITION(src.SampleCount() != nullptr);
		Info.SampleType = ValueType::SNorm16;
		const uint16 blockAlign = uint16(sizeof(short)*uint16(Info.Channels));
		WAVEFORMATEX wfx = {
			WAVE_FORMAT_PCM, uint16(Info.Channels), unsigned(Info.SampleRate),
			unsigned(Info.SampleRate*blockAlign), blockAlign, 16, sizeof(WAVEFORMATEX)
		};
		const DSBUFFERDESC dsbd = {sizeof(DSBUFFERDESC),
			DSBCAPS_GLOBALFOCUS|DSBCAPS_CTRLFREQUENCY|DSBCAPS_CTRLPAN|
			DSBCAPS_CTRLPOSITIONNOTIFY|DSBCAPS_CTRLVOLUME/*|DSBCAPS_CTRL3D*/,
			DWORD(Info.GetBufferSize()), 0, &wfx, {}};
		auto& context = SoundContext::Instance();
		if(!context.Device()) return;
		if(FAILED(context.Device()->CreateSoundBuffer(&dsbd, &Buffer, nullptr)))
		{
			Buffer = nullptr;
			Info.SampleType = Intra::ValueType::Void;
			return;
		}

		void* dstData;
		DWORD lockedSize;
		Buffer->Lock(0, DWORD(Info.GetBufferSize()), &dstData, &lockedSize, nullptr, nullptr, 0);
		INTRA_DEBUG_ASSERT(lockedSize == Info.GetBufferSize());
		auto dst = SpanOfRawElements<short>(dstData, Info.SampleCount*Info.Channels);
		while(!dst.Empty()) src.GetInterleavedSamples(dst.TakeAdvance(32768));
		Buffer->Unlock(dstData, lockedSize, nullptr, 0);

		INTRA_SYNCHRONIZED(context.MyMutex)
			context.AllSounds.AddLast(this);
	}

	INTRA_FORCEINLINE ~Data() {Release();}

	Data(const Data&) = delete;
	Data& operator=(const Data&) = delete;

	void SetDataInterleaved(const void* data, ValueType type)
	{
		if(data == nullptr) return;
		void* dstData;
		DWORD lockedSize;
		Buffer->Lock(0, DWORD(Info.GetBufferSize()), &dstData, &lockedSize, nullptr, nullptr, 0);
		INTRA_DEBUG_ASSERT(lockedSize == Info.GetBufferSize());
		auto dst = SpanOfRawElements<short>(dstData, Info.SampleCount*Info.Channels);
		INTRA_FINALLY{Buffer->Unlock(dstData, DWORD(Info.GetBufferSize()), nullptr, 0);};
		if(type == ValueType::SNorm16)
		{
			auto src = SpanOfRawElements<short>(data, dst.Length());
			CopyTo(src, dst);
			return;
		}
		if(type == ValueType::Float)
		{
			auto src = SpanOfRawElements<float>(data, dst.Length());
			auto srcShorts = Map(src, [](float x) {return x*32767;});
			CopyTo(srcShorts, dst);
		}
	}

	void Release();

	bool IsReleased()
	{
		bool result = false;
		INTRA_SYNCHRONIZED(MyMutex)
		{
			result = (Buffer == nullptr);
		}
		return result;
	}

	INTRA_FORCEINLINE void Update() {}

	IDirectSoundBuffer* Buffer;

	void* LockedBits = nullptr;

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
		impl->SelfRef = nullptr;
	}

	Data(Shared<Sound::Data> parent): SoundInstanceBasicData(Move(parent))
	{
		if(FAILED(SoundContext::Device()->DuplicateSoundBuffer(Parent->Buffer, &DupBuffer)))
		{
			return;
		}

		IDirectSoundNotify* notify;
		if(FAILED(DupBuffer->QueryInterface(IID_IDirectSoundNotify, reinterpret_cast<void**>(&notify))))
		{
			DupBuffer->Release();
			DupBuffer = nullptr;
			return;
		}

		NotifyOnStopEvent = CreateEventW(nullptr, false, false, nullptr);
		if(NotifyOnStopEvent == nullptr)
		{
			DupBuffer->Release();
			DupBuffer = nullptr;
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
		auto ref = Move(SelfRef);
		INTRA_SYNCHRONIZED(MyMutex)
		{
			if(!DupBuffer) return;
			DupBuffer->Stop();
			DupBuffer->Release();
			DupBuffer = nullptr;
		}
	}

	bool IsReleased()
	{
		bool result = false;
		INTRA_SYNCHRONIZED(MyMutex)
		{
			result = (DupBuffer == nullptr);
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
		auto ref = Move(SelfRef);
		INTRA_SYNCHRONIZED(MyMutex)
		{
			if(!DupBuffer) return;
			DupBuffer->Stop();
		}
	}

	IDirectSoundBuffer* DupBuffer;

	HANDLE NotifyOnStopEvent = nullptr;
	HANDLE NotifyOnStopWait = nullptr;
};


struct StreamedSound::Data: SharedClass<StreamedSound::Data>, detail::StreamedSoundBasicData
{
	Data(Unique<IAudioSource> source, index_t bufferSampleCount, bool autoStreamingEnabled = true):
		StreamedSoundBasicData(Move(source), bufferSampleCount),
		AutoStreamingEnabled(autoStreamingEnabled)
	{
		INTRA_PRECONDITION(Source != nullptr);
		auto& context = SoundContext::Instance();
		if(!context.Device()) return;

		const uint16 blockAlign = uint16(sizeof(uint16) * uint16(Source->ChannelCount()));
		WAVEFORMATEX wfx = {
			WAVE_FORMAT_PCM, uint16(Source->ChannelCount()), unsigned(Source->SampleRate()),
			unsigned(Source->SampleRate())*blockAlign, blockAlign, 16, sizeof(WAVEFORMATEX)
		};
		const DSBUFFERDESC dsbd = {
			sizeof(DSBUFFERDESC), DSBCAPS_GLOBALFOCUS|DSBCAPS_CTRLPOSITIONNOTIFY,
			DWORD(GetBufferSize()*2), 0, &wfx, {}
		};

		if(FAILED(context.Device()->CreateSoundBuffer(&dsbd, &Buffer, nullptr))) return;

		DWORD lockedSize; void* lockedData;
		Buffer->Lock(0, DWORD(GetBufferSize()*2), &lockedData, &lockedSize, nullptr, nullptr, 0);
		const auto dst = SpanOfRaw<short>(lockedData, lockedSize);
		if(Source) Source->GetInterleavedSamples(dst);
		else FillZeros(dst);
		Buffer->Unlock(lockedData, lockedSize, nullptr, 0);

		IDirectSoundNotify* notify;
		if(FAILED(Buffer->QueryInterface(IID_IDirectSoundNotify, reinterpret_cast<void**>(&notify))))
			return;

		NotifyLoadEvents[0] = CreateEventW(nullptr, false, false, nullptr);
		NotifyLoadEvents[1] = CreateEventW(nullptr, false, false, nullptr);
		if(NotifyLoadEvents[0] == nullptr || NotifyLoadEvents[1] == nullptr)
			return;

		const DSBPOSITIONNOTIFY positionNotify[2] = {
			{0, NotifyLoadEvents[0]},
			{DWORD(GetBufferSize()), NotifyLoadEvents[1]}
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
		auto ref = Move(SelfRef);
		INTRA_SYNCHRONIZED(MyMutex)
		{
			if(!Buffer) return;
			Buffer->Stop();
			Buffer->Release();
			Buffer = nullptr;
		}
		auto& context = SoundContext::Instance();
		INTRA_SYNCHRONIZED(context.MyMutex)
		{
			context.AllStreamedSounds.FindAndRemoveUnordered(this);
		}
	}

	bool IsReleased()
	{
		bool result = false;
		INTRA_SYNCHRONIZED(MyMutex)
		{
			result = (Buffer == nullptr);
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
		auto ref = Move(SelfRef);
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

	AnyPtr lock(unsigned no, void*& lockedBits, DWORD& lockedSize, void*& lockedBits2, DWORD& lockedSize2)
	{
		const auto lockSampleStart = no == 0? 0: size_t(BufferSampleCount);
		const HRESULT lockResult = Buffer->Lock(unsigned(lockSampleStart*sizeof(short)*size_t(Source->ChannelCount())),
			DWORD(GetBufferSize()), &lockedBits, &lockedSize, &lockedBits2, &lockedSize2, 0);
		INTRA_DEBUG_ASSERT(!FAILED(lockResult));
		if(FAILED(lockResult)) return nullptr;
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
		if(data == nullptr) return;

		auto buffer = SpanOfRaw<short>(data, GetBufferSize());
		if(StopSoon != 0)
		{
			//Достигнут конец потока данных, поэтому зануляем оставшийся буфер.
			//Когда он закончится, воспроизведение будет остановлено.
			FillZeros(buffer);
			unlock(lockedBits, lockedSize, lockedBits2, lockedSize2);
			SelfRef = nullptr;
			return;
		}

		const auto samplesRead = Source->GetInterleavedSamples(buffer);
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
		if(Buffer == nullptr || StopSoon == 2)
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

	Mutex MyMutex;
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

void Sound::Data::Release()
{
	INTRA_SYNCHRONIZED(MyMutex)
	{
		if(!Buffer) return;
		Buffer->Stop();
		Buffer->Release();
		Buffer = nullptr;
		for(auto inst: Instances) inst->Release();
	}
	auto& context = SoundContext::Instance();
	INTRA_SYNCHRONIZED(context.MyMutex)
	{
		context.AllSounds.FindAndRemoveUnordered(this);
	}
}

} INTRA_END
