#pragma once

#include "Intra/Core.h"
#include "IntraX/Unstable/Data/ValueType.h"

namespace Intra { INTRA_BEGIN
struct AudioBuffer;
class IAudioSource;

struct SoundInfo
{
	index_t SampleCount;
	int SampleRate;
	short Channels;
	ValueType SampleType;

	SoundInfo(decltype(nullptr)): SoundInfo() {}

	SoundInfo(Index sampleCount = 0, NonNegative<int> sampleRate = 44100, NonNegative<short> channels=1, ValueType sampleType = ValueType::Void):
		SampleCount(sampleCount), SampleRate(sampleRate), Channels(channels), SampleType(sampleType) {}

	size_t GetBufferSize() const {return size_t(SampleCount*Channels*SampleType.Size());}
	double Duration() const {return SampleRate == 0? 0: double(SampleCount)/SampleRate;}

	bool operator==(decltype(nullptr)) const noexcept {return SampleType == ValueType::Void;}
	bool operator!=(decltype(nullptr)) const noexcept {return !operator==(nullptr);}
};
} INTRA_END
