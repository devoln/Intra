#pragma once

#include "Core/Core.h"
#include "Data/ValueType.h"

namespace Intra {

namespace IO {class IInputStream;}
struct SoundBuffer;
class SoundInstance;
class ASoundSampleSource;

struct SoundInfo
{
	size_t SampleCount;
	uint SampleRate;
	ushort Channels;
	ValueType SampleType;

	SoundInfo(size_t sampleCount=0, uint sampleRate=44100, ushort channels=1, ValueType sampleType=ValueType::Short):
		SampleCount(sampleCount), SampleRate(sampleRate), Channels(channels), SampleType(sampleType) {}

	size_t GetBufferSize() const {return SampleCount*Channels*SampleType.Size();}
	double Duration() const {return (double)SampleCount/SampleRate;}
};

}
