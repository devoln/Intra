#pragma once


#include "Core/Core.h"

#include "Data/ValueType.h"

INTRA_BEGIN
namespace Audio {

struct AudioBuffer;
class IAudioSource;

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

struct SoundInfo
{
	size_t SampleCount;
	uint SampleRate;
	ushort Channels;
	Data::ValueType SampleType;

	SoundInfo(null_t): SoundInfo() {}

	SoundInfo(size_t sampleCount=0, uint sampleRate=44100, ushort channels=1, Data::ValueType sampleType=Data::ValueType::Void):
		SampleCount(sampleCount), SampleRate(sampleRate), Channels(channels), SampleType(sampleType) {}

	size_t GetBufferSize() const {return SampleCount*Channels*SampleType.Size();}
	double Duration() const {return SampleRate == 0? 0: double(SampleCount)/SampleRate;}

	bool operator==(null_t) const noexcept {return SampleType == Data::ValueType::Void;}
	bool operator!=(null_t) const noexcept {return !operator==(null);}
};

INTRA_WARNING_POP

}}
