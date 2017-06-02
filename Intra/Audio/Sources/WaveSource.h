#pragma once

#include "Cpp/Warnings.h"
#include "Cpp/Fundamental.h"

#include "Utils/FixedArray.h"

#include "Audio/AudioSource.h"

namespace Intra { namespace Audio { namespace Sources {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#ifndef INTRA_NO_WAVE_LOADER

class WaveSource: public ASoundSource
{
	CSpan<short> mData;
	size_t mSampleCount, mCurrentDataPos;
public:
	WaveSource(CSpan<byte> srcFileData);

	size_t SampleCount() const override {return mSampleCount;}
	size_t CurrentSamplePosition() const override {return mCurrentDataPos/mChannelCount;}

	size_t GetInterleavedSamples(Span<short> outShorts) override;
	size_t GetInterleavedSamples(Span<float> outFloats) override;
	size_t GetUninterleavedSamples(CSpan<Span<float>> outFloats) override;
	FixedArray<const void*> GetRawSamplesData(size_t maxSamplesToRead,
		Data::ValueType* oType, bool* oInterleaved, size_t* oSamplesRead) override;

	WaveSource& operator=(const WaveSource& rhs) = delete;
};

#endif

INTRA_WARNING_POP

}}}
