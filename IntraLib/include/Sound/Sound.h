#pragma once

#include "Sound/SoundApiDeclarations.h"
#include "Containers/Array.h"
#include "Utils/Callback.h"
#include "Memory/SmartRef.h"
#include "Containers/SparseHandledArray.h"
#include "Sound/SoundTypes.h"
#include "Data/ValueType.h"

namespace Intra {

class Sound
{
	friend class SoundInstance;
public:
	Sound(null_t=null): info{0,0,0,ValueType::Void},
		data(null), locked_bits(null), locked_size(0) {}

	explicit Sound(const SoundBuffer* data);
    Sound(const SoundInfo& bufferInfo, const void* initData=null);

	Sound(Sound&& rhs): info(rhs.info), data(null) {operator=(core::move(rhs)); rhs.data=null;}

	~Sound();

	void Release();

    AnyPtr Lock();
	void Unlock();

	static Sound FromFile(StringView fileName);
	static Sound FromSource(ASoundSampleSource* src);

	Sound& operator=(Sound&& rhs);

	bool operator==(null_t) const {return data==null;}
	bool operator!=(null_t) const {return !operator==(null);}

	const SoundInfo& Info() const {return info;}


	SoundInstance CreateInstance();

	ArrayRange<SoundInstance* const> Instances() const {return instances;}

private:
	SoundAPI::BufferHandle data;

	Array<SoundInstance*> instances;
    short* locked_bits;
	SoundInfo info;
	uint locked_size;


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
	SoundInstance(null_t=null): my_sound(null), data(null) {}
	SoundInstance(SoundInstance&& rhs): my_sound(null) {operator=(core::move(rhs));}

	SoundInstance& operator=(SoundInstance&& rhs)
	{
		if(my_sound!=null) Release();
		my_sound = rhs.my_sound;
		data = rhs.data;
		rhs.my_sound = null;
		rhs.data = null;
		if(my_sound!=null) my_sound->instances().Find(&rhs).First()=this;
		return *this;
	}

	~SoundInstance();
	void Release();

	void Play(bool loop=false) const;
	bool IsPlaying() const;
	void Stop() const;

	Sound* MySound() const {return my_sound;}

private:
	Sound* my_sound;
	SoundAPI::InstanceHandle data;

	SoundInstance(const SoundInstance&) = delete;
	SoundInstance& operator=(const SoundInstance&) = delete;
};

class StreamedSound
{
public:
	typedef Memory::UniqueReference<ASoundSampleSource> SourceRef;
	typedef Utils::Callback<void()> OnCloseCallback;

	StreamedSound(null_t=null) {}
	StreamedSound(SourceRef&& src, size_t bufferSizeInSamples=16384, OnCloseCallback onClose=null);
	StreamedSound(StreamedSound&& rhs): sample_source(core::move(rhs.sample_source)),
		on_close(rhs.on_close), data(rhs.data) {rhs.data=null; rhs.on_close=null;}
	~StreamedSound() {release();}
	static StreamedSound FromFile(StringView fileName, size_t bufferSizeInSamples=16384);

	StreamedSound& operator=(StreamedSound&& rhs)
	{
		INTRA_ASSERT(this!=&rhs);
		release();
		if(rhs.data!=null) rhs.unregister_instance();
		sample_source = core::move(rhs.sample_source);
		on_close = rhs.on_close;
		rhs.on_close = null;
		data = rhs.data;
		rhs.data = null;
		if(data!=null) register_instance();
		return *this;
	}

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

private:
	SourceRef sample_source;
	OnCloseCallback on_close;
	SoundAPI::StreamedBufferHandle data=null;

	StreamedSound(const StreamedSound&) = delete;

	void register_instance();
	void unregister_instance();
	static Array<StreamedSound*> all_existing_instances;

	void release();
};

}
