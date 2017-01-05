#pragma once

#include "Platform/CppWarnings.h"
#include "Core/FundamentalTypes.h"
#include "Data/ValueType.h"

namespace Intra { namespace Audio {

namespace IO {class IInputStream;}
struct AudioBuffer;
class SoundInstance;
class ASoundSource;

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

struct SoundInfo
{
	size_t SampleCount;
	uint SampleRate;
	ushort Channels;
	ValueType SampleType;

	SoundInfo(size_t sampleCount=0, uint sampleRate=44100, ushort channels=1, ValueType sampleType=ValueType::Short):
		SampleCount(sampleCount), SampleRate(sampleRate), Channels(channels), SampleType(sampleType) {}

	size_t GetBufferSize() const {return SampleCount*Channels*SampleType.Size();}
	double Duration() const {return double(SampleCount)/SampleRate;}
};

INTRA_WARNING_POP

}}
