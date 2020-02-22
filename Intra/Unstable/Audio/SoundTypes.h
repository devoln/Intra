#pragma once

#include "Core/Core.h"
#include "Data/ValueType.h"

INTRA_BEGIN
struct AudioBuffer;
class IAudioSource;

struct SoundInfo
{
	size_t SampleCount;
	uint SampleRate;
	ushort Channels;
	ValueType SampleType;

	SoundInfo(null_t): SoundInfo() {}

	SoundInfo(size_t sampleCount=0, uint sampleRate=44100, ushort channels=1, ValueType sampleType=ValueType::Void):
		SampleCount(sampleCount), SampleRate(sampleRate), Channels(channels), SampleType(sampleType) {}

	size_t GetBufferSize() const {return SampleCount*Channels*SampleType.Size();}
	double Duration() const {return SampleRate == 0? 0: double(SampleCount)/SampleRate;}

	bool operator==(null_t) const noexcept {return SampleType == ValueType::Void;}
	bool operator!=(null_t) const noexcept {return !operator==(null);}
};
INTRA_END
