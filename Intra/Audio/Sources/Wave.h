#pragma once

#include "Cpp/Warnings.h"
#include "Cpp/Fundamental.h"

#include "Utils/FixedArray.h"
#include "Range/Polymorphic/OutputRange.h"

#include "Data/ValueType.h"

#include "Audio/SoundTypes.h"
#include "Audio/AudioSource.h"

namespace Intra { namespace Audio { namespace Sources {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

class Wave: public BasicAudioSource
{
	CSpan<byte> mData;
	size_t mSampleCount = 0, mCurrentDataPos = 0;
	Data::ValueType mDataType;
public:
	Wave(OnCloseResourceCallback onClose, const SoundInfo& info, const void* data):
		BasicAudioSource(Cpp::Move(onClose), info.SampleRate, info.Channels),
		mData(SpanOfRaw(data, info.GetBufferSize())), mDataType(info.SampleType) {}

	Wave(OnCloseResourceCallback onClose, size_t sampleRate, ushort numChannels, CSpan<short> data):
		BasicAudioSource(Cpp::Move(onClose), sampleRate, numChannels),
		mData(data.Reinterpret<byte>()), mSampleCount(data.Length()), mDataType(Data::ValueType::SNorm16) {}

	Wave(OnCloseResourceCallback onClose, size_t sampleRate, ushort numChannels, CSpan<float> data):
		BasicAudioSource(Cpp::Move(onClose), sampleRate, numChannels),
		mData(data.Reinterpret<byte>()), mSampleCount(data.Length()), mDataType(Data::ValueType::Float) {}

	Wave(OnCloseResourceCallback onClose, CSpan<byte> srcFileData);

	Wave(const Wave&) = delete;
	Wave(Wave&&) = default;
	Wave& operator=(const Wave&) = delete;
	Wave& operator=(Wave&&) = default;

	size_t SampleCount() const override {return mSampleCount;}
	size_t SamplePosition() const override {return mCurrentDataPos/mChannelCount;}

	size_t GetInterleavedSamples(Span<short> outShorts) override;
	size_t GetInterleavedSamples(Span<float> outFloats) override;
	size_t GetUninterleavedSamples(CSpan<Span<float>> outFloats) override;

	FixedArray<const void*> GetRawSamplesData(size_t maxSamplesToRead,
		Data::ValueType* oType, bool* oInterleaved, size_t* oSamplesRead) override;
};

void WriteWave(IAudioSource& source, OutputStream& stream, Data::ValueType sampleType = Data::ValueType::SNorm16);

INTRA_WARNING_POP

}}}
