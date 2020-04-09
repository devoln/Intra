


#include "Extra/Unstable/Audio/Sound.h"

#include "Intra/Range/Mutation/Fill.h"
#include "Intra/Range/Mutation/Cast.h"

#include "Extra/Unstable/Data/ValueType.h"

#include "SoundBasicData.hxx"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

INTRA_BEGIN
namespace Audio {

using ValueType;

unsigned Sound::DefaultSampleRate() {return 0;}


struct SoundContext: detail::SoundBasicContext
{
	INTRA_FORCEINLINE void ReleaseAllSounds() {}
	INTRA_FORCEINLINE void ReleaseAllStreamedSounds() {}

	INTRA_FORCEINLINE SoundContext() {}

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
	INTRA_FORCEINLINE Data(IAudioSource& src): SoundBasicData(src) {}

	Data(const Data&) = delete;
	Data& operator=(const Data&) = delete;

	INTRA_FORCEINLINE void SetDataInterleaved(const void* data, ValueType type) {(void)data; (void)type;}
	INTRA_FORCEINLINE void Release() {}
	INTRA_FORCEINLINE void ReleaseAllInstances() {}
	INTRA_FORCEINLINE bool IsReleased() const {return false;}
};

struct Sound::Instance::Data: SharedClass<Sound::Instance::Data>, detail::SoundInstanceBasicData
{
	INTRA_FORCEINLINE Data(Shared<Sound::Data> parent): SoundInstanceBasicData(Move(parent)) {}

	Data(const Data&) = delete;
	Data& operator=(const Data&) = delete;

	INTRA_FORCEINLINE void Release() {}
	INTRA_FORCEINLINE void Play(bool loop) {(void)loop;}
	INTRA_FORCEINLINE bool IsPlaying() {return false;}
	INTRA_FORCEINLINE void Stop() {}
};


struct StreamedSound::Data: SharedClass<StreamedSound::Data>, detail::StreamedSoundBasicData
{
	Data(Unique<IAudioSource> source, size_t bufferSampleCount, bool autoStreamingEnabled = false):
		StreamedSoundBasicData(Move(source), bufferSampleCount) {(void)autoStreamingEnabled;}

	Data(const Data&) = delete;
	Data& operator=(const Data&) = delete;

	INTRA_FORCEINLINE void Release() {}
	INTRA_FORCEINLINE bool IsReleased() const {return false;}
	INTRA_FORCEINLINE void Play(bool loop) {(void)loop;}
	INTRA_FORCEINLINE bool IsPlaying() const {return false;}
	INTRA_FORCEINLINE void Stop() {}
	INTRA_FORCEINLINE void Update() {}
};

}}

INTRA_WARNING_POP
