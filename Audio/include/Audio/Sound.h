#pragma once

#include "Platform/CppWarnings.h"
#include "Audio/SoundApiDeclarations.h"
#include "Container/Sequential/Array.h"
#include "Memory/SmartRef.hh"
#include "Container/Utility/SparseHandledArray.h"
#include "Audio/SoundTypes.h"
#include "Data/ValueType.h"
#include "Utils/Callback.h"

namespace Intra { namespace Audio {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

class Sound
{
	friend class SoundInstance;
public:
	Sound(null_t=null):
		mData(null), mInstances(), mLockedBits(null),
		mInfo{0,0,0,ValueType::Void}, mLockedSize(0) {}

	explicit Sound(const AudioBuffer* data);
    Sound(const SoundInfo& bufferInfo, const void* initData=null);

	Sound(Sound&& rhs):
		mData(null), mInstances(), mLockedBits(null),
		mInfo(rhs.mInfo), mLockedSize(0)
	{
		operator=(Meta::Move(rhs));
		rhs.mData = null;
	}

	~Sound();

	void Release();

    AnyPtr Lock();
	void Unlock();

	static Sound FromFile(StringView fileName);
	static Sound FromSource(ASoundSource* src);

	Sound& operator=(Sound&& rhs);
	Sound& operator=(null_t) {Release(); return *this;}

	bool operator==(null_t) const {return mData==null;}
	bool operator!=(null_t) const {return !operator==(null);}

	const SoundInfo& Info() const {return mInfo;}


	SoundInstance CreateInstance();

	ArrayRange<SoundInstance* const> Instances() const {return mInstances;}

	static void DeleteAllSounds()
	{
		for(auto sound: Sound::all_existing_sounds) *sound = null;
	}

private:
	SoundAPI::BufferHandle mData;
	Array<SoundInstance*> mInstances;
    short* mLockedBits;
	SoundInfo mInfo;
	uint mLockedSize;

	static Array<Sound*> all_existing_sounds;


private:
	Sound(const Sound&) = delete;
	Sound& operator=(const Sound&) = delete;
};

class SoundInstance
{
	friend class Sound;
private:
	SoundInstance(Sound* mySound, SoundAPI::InstanceHandle inst);
public:
	SoundInstance(null_t=null): mSound(null), mData(null) {}
	SoundInstance(SoundInstance&& rhs): mSound(null), mData(null) {operator=(Meta::Move(rhs));}

	SoundInstance& operator=(SoundInstance&& rhs)
	{
		if(mSound!=null) Release();
		mSound = rhs.mSound;
		mData = rhs.mData;
		rhs.mSound = null;
		rhs.mData = null;
		if(mSound!=null) Algo::Find(mSound->mInstances, &rhs).First() = this;
		return *this;
	}

	~SoundInstance();
	void Release(bool force=false);

	void Play(bool loop=false) const;
	bool IsPlaying() const;
	void Stop() const;

	Sound* MySound() const {return mSound;}

private:
	Sound* mSound;
	SoundAPI::InstanceHandle mData;

	SoundInstance(const SoundInstance&) = delete;
	SoundInstance& operator=(const SoundInstance&) = delete;
};

class StreamedSound
{
public:
	typedef Memory::UniqueRef<ASoundSource> SourceRef;
	typedef Utils::Callback<void()> OnCloseCallback;

	StreamedSound(null_t=null):
		mSampleSource(null), mOnClose(null), mData(null) {}

	StreamedSound(SourceRef&& src, size_t bufferSizeInSamples=16384, OnCloseCallback onClose=null);

	StreamedSound(StreamedSound&& rhs):
		mSampleSource(Meta::Move(rhs.mSampleSource)),
		mOnClose(rhs.mOnClose), mData(rhs.mData)
	{
		rhs.mData=null;
		rhs.mOnClose=null;
	}

	~StreamedSound() {release();}


	static StreamedSound FromFile(StringView fileName, size_t bufferSizeInSamples=16384);


	StreamedSound& operator=(StreamedSound&& rhs)
	{
		INTRA_ASSERT(this!=&rhs);
		release();
		if(rhs.mData!=null) rhs.unregister_instance();
		mSampleSource = Meta::Move(rhs.mSampleSource);
		mOnClose = rhs.mOnClose;
		rhs.mOnClose = null;
		mData = rhs.mData;
		rhs.mData = null;
		if(mData!=null) register_instance();
		return *this;
	}

	StreamedSound& operator=(const StreamedSound& rhs) = delete;

	void Play(bool loop=false) const;
	bool IsPlaying() const;
	void Stop() const;

	void UpdateBuffer() const;

	static ArrayRange<StreamedSound* const> AllExistingInstances() {return all_existing_instances;}

	static void UpdateAllExistingInstances()
	{
		for(auto snd: AllExistingInstances())
			snd->UpdateBuffer();
	}

	static uint InternalSampleRate();

	static void DeleteAllSounds()
	{
		for(auto sound: StreamedSound::all_existing_instances) *sound = null;
	}

private:
	SourceRef mSampleSource;
	OnCloseCallback mOnClose;
	SoundAPI::StreamedBufferHandle mData;

	StreamedSound(const StreamedSound&) = delete;

	void register_instance();
	void unregister_instance();
	static Array<StreamedSound*> all_existing_instances;

	void release();
};

void CleanUpSoundSystem();

INTRA_WARNING_POP

}}
