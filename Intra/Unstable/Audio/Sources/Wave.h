#pragma once


#include "Core/Core.h"

#include "Utils/FixedArray.h"
#include "Core/Range/Polymorphic/OutputRange.h"

#include "Data/ValueType.h"

#include "Audio/SoundTypes.h"
#include "Audio/AudioSource.h"

INTRA_BEGIN
namespace Sources {
class Wave: public BasicAudioSource
{
	CSpan<byte> mData;
	size_t mSampleCount = 0, mCurrentDataPos = 0;
	ValueType mDataType;
public:
	Wave(OnCloseResourceCallback onClose, const SoundInfo& info, const void* data):
		BasicAudioSource(Move(onClose), info.SampleRate, info.Channels),
		mData(SpanOfRaw(data, info.GetBufferSize())), mDataType(info.SampleType) {}

	Wave(OnCloseResourceCallback onClose, uint sampleRate, ushort numChannels, CSpan<short> data):
		BasicAudioSource(Move(onClose), sampleRate, numChannels),
		mData(data.Reinterpret<byte>()), mSampleCount(data.Length()), mDataType(ValueType::SNorm16) {}

	Wave(OnCloseResourceCallback onClose, uint sampleRate, ushort numChannels, CSpan<float> data):
		BasicAudioSource(Move(onClose), sampleRate, numChannels),
		mData(data.Reinterpret<byte>()), mSampleCount(data.Length()), mDataType(ValueType::Float) {}

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
		ValueType* oType, bool* oInterleaved, size_t* oSamplesRead) override;
};

void WriteWave(IAudioSource& source, OutputStream& stream, ValueType sampleType = ValueType::SNorm16);
}
INTRA_END
