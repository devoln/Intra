#pragma once

#include "Platform/CppWarnings.h"
#include "Core/FundamentalTypes.h"
#include "Audio/AudioSource.h"

namespace Intra { namespace Audio { namespace Sources {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#ifndef INTRA_NO_WAVE_LOADER

class WaveSource: public ASoundSource
{
	ArrayRange<const byte> mData;
	size_t mSampleCount, mCurrentDataPos;
public:
	WaveSource(ArrayRange<const byte> srcFileData);

	size_t SampleCount() const override {return mSampleCount;}
	size_t CurrentSamplePosition() const override {return mCurrentDataPos/mChannelCount;}

	size_t GetInterleavedSamples(ArrayRange<short> outShorts) override;
	size_t GetInterleavedSamples(ArrayRange<float> outFloats) override;
	size_t GetUninterleavedSamples(ArrayRange<const ArrayRange<float>> outFloats) override;
	Array<const void*> GetRawSamplesData(size_t maxSamplesToRead,
		ValueType* oType, bool* oInterleaved, size_t* oSamplesRead) override;

	WaveSource& operator=(const WaveSource& rhs) = delete;
};

#endif

INTRA_WARNING_POP

}}}
