


#include "Audio/Sound.h"

#include "Core/Range/Mutation/Fill.h"
#include "Core/Range/Mutation/Cast.h"

#include "Data/ValueType.h"

#include "SoundBasicData.hxx"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

INTRA_BEGIN
namespace Audio {

using ValueType;

uint Sound::DefaultSampleRate() {return 0;}


struct SoundContext: detail::SoundBasicContext
{
	forceinline void ReleaseAllSounds() {}
	forceinline void ReleaseAllStreamedSounds() {}

	forceinline SoundContext() {}

	static SoundContext& Instance()
	{
		static SoundContext context;
		return context;
	}

	SoundContext(const SoundContext&) = delete;
	SoundContext& operator=(const SoundContext&) = delete;
};

struct Sound::Data: SharedClass<Sound::Data>, detail::SoundBasicData
{
	forceinline Data(IAudioSource& src): SoundBasicData(src) {}

	Data(const Data&) = delete;
	Data& operator=(const Data&) = delete;

	forceinline void SetDataInterleaved(const void* data, ValueType type) {(void)data; (void)type;}
	forceinline void Release() {}
	forceinline void ReleaseAllInstances() {}
	forceinline bool IsReleased() const {return false;}
};

struct Sound::Instance::Data: SharedClass<Sound::Instance::Data>, detail::SoundInstanceBasicData
{
	forceinline Data(Shared<Sound::Data> parent): SoundInstanceBasicData(Move(parent)) {}

	Data(const Data&) = delete;
	Data& operator=(const Data&) = delete;

	forceinline void Release() {}
	forceinline void Play(bool loop) {(void)loop;}
	forceinline bool IsPlaying() {return false;}
	forceinline void Stop() {}
};


struct StreamedSound::Data: SharedClass<StreamedSound::Data>, detail::StreamedSoundBasicData
{
	Data(Unique<IAudioSource> source, size_t bufferSampleCount, bool autoStreamingEnabled = false):
		StreamedSoundBasicData(Move(source), bufferSampleCount) {(void)autoStreamingEnabled;}

	Data(const Data&) = delete;
	Data& operator=(const Data&) = delete;

	forceinline void Release() {}
	forceinline bool IsReleased() const {return false;}
	forceinline void Play(bool loop) {(void)loop;}
	forceinline bool IsPlaying() const {return false;}
	forceinline void Stop() {}
	forceinline void Update() {}
};

}}

INTRA_WARNING_POP
