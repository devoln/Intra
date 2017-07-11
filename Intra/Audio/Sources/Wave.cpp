#include "Audio/Sources/Wave.h"
#include "Audio/SampleConversion.h"

#include "Math/Math.h"

#include "Cpp/Intrinsics.h"
#include "Cpp/Warnings.h"
#include "Cpp/Endianess.h"

#include "Range/Mutation/Cast.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio { namespace Sources {

struct WaveHeader
{
	char RIFF[4];
	uintLE WaveformChunkSize;
	char WAVE[4];

	char fmt[4];
	uintLE FormatChunkSize;

	ushortLE FormatTag, Channels;
	uintLE SampleRate, BytesPerSec;
	ushortLE BlockAlign, BitsPerSample;
	char data[4];
	uintLE DataSize;
};


Wave::Wave(OnCloseResourceCallback onClose, CSpan<byte> srcFileData):
	BasicAudioSource(Cpp::Move(onClose))
{
	const WaveHeader& header = *reinterpret_cast<const WaveHeader*>(srcFileData.Begin);

	const bool isValidHeader = (C::memcmp(header.RIFF, "RIFF", sizeof(header.RIFF)) == 0 &&
		C::memcmp(header.WAVE, "WAVE", sizeof(header.WAVE)) == 0 &&
		C::memcmp(header.fmt, "fmt ", sizeof(header.fmt)) == 0 &&
		C::memcmp(header.data, "data", sizeof(header.data)) == 0);
	
	if(!isValidHeader) return;

	mData = srcFileData.Drop(sizeof(WaveHeader)).Reinterpret<const byte>().Take(header.DataSize);
	mChannelCount = ushort(header.Channels);
	mSampleRate = header.SampleRate;
	mSampleCount = header.DataSize/sizeof(short)/mChannelCount;
	if(header.FormatTag == 1) switch(header.BitsPerSample)
	{
	case 8: mDataType = Data::ValueType::SNorm8; break;
	case 16: mDataType = Data::ValueType::SNorm16; break;
	case 24: mDataType = Data::ValueType::SNorm24; break;
	case 32: mDataType = Data::ValueType::SNorm32; break;
	default:
		mDataType = Data::ValueType::Void;
		mSampleCount = 0;
		mData = null;
	}
	else if(header.FormatTag == 3)
	{
		if(header.BitsPerSample == 32) mDataType = Data::ValueType::Float;
		else mDataType = Data::ValueType::Double;
	}
}

size_t Wave::GetInterleavedSamples(Span<short> outShorts)
{
	INTRA_DEBUG_ASSERT(!outShorts.Empty());
	size_t valuesRead = 0;
	if(mDataType == Data::ValueType::SNorm16)
	{
		const auto srcShorts = mData.Reinterpret<const short>().Drop(mCurrentDataPos);
		valuesRead = CopyTo(srcShorts, outShorts);
	}
	else if(mDataType == Data::ValueType::Float)
	{
		const auto srcFloats = mData.Reinterpret<const float>().Drop(mCurrentDataPos).Take(outShorts.Length());
		valuesRead = srcFloats.Length();
		CastFromNormalized(outShorts.Take(valuesRead), srcFloats);
	}

	mCurrentDataPos += valuesRead;
	if(valuesRead < outShorts.Length()) mCurrentDataPos = 0;
	return valuesRead / mChannelCount;
}

size_t Wave::GetInterleavedSamples(Span<float> outFloats)
{
	INTRA_DEBUG_ASSERT(!outFloats.Empty());
	size_t valuesRead = 0;
	if(mDataType == Data::ValueType::SNorm16)
	{
		const auto srcShorts = mData.Reinterpret<const short>().Drop(mCurrentDataPos).Take(outFloats.Length());
		valuesRead = srcShorts.Length();
		CastToNormalized(outFloats.Take(valuesRead), srcShorts);
	}
	else if(mDataType == Data::ValueType::Float)
	{
		const auto srcFloats = mData.Reinterpret<const float>().Drop(mCurrentDataPos);
		valuesRead = CopyTo(srcFloats, outFloats);
	}
	return valuesRead / mChannelCount;
}

size_t Wave::GetUninterleavedSamples(CSpan<Span<float>> outFloatChannels)
{
	INTRA_DEBUG_ASSERT(outFloatChannels.Length() == mChannelCount);
	const size_t outSamplesCount = outFloatChannels.First().Length();
	for(size_t i=1; i<mChannelCount; i++)
	{
		INTRA_DEBUG_ASSERT(outFloatChannels[i].Length() == outSamplesCount);
	}

	size_t valuesRead = 0;
	if(mDataType == Data::ValueType::SNorm16)
	{
		const auto srcShorts = mData.Reinterpret<const short>().Drop(mCurrentDataPos)
			.Take(outFloatChannels.Length()*outSamplesCount);
		FixedArray<Span<float>> outFloatChannelsTempSpans = outFloatChannels;
		DeinterleaveShortsCastToFloats(srcShorts, outFloatChannelsTempSpans);
		valuesRead = srcShorts.Length();
	}
	else if(mDataType == Data::ValueType::Float)
	{
		const auto srcFloats = mData.Reinterpret<const float>().Drop(mCurrentDataPos)
			.Take(outFloatChannels.Length()*outSamplesCount);
		FixedArray<Span<float>> outFloatChannelsTempSpans = outFloatChannels;
		DeinterleaveFloats(srcFloats, outFloatChannelsTempSpans);
		valuesRead = srcFloats.Length();
	}
	return valuesRead / mChannelCount;
}

FixedArray<const void*> Wave::GetRawSamplesData(size_t maxSamplesToRead,
	Data::ValueType* oType, bool* oInterleaved, size_t* oSamplesRead)
{
	const auto shortsToRead = Math::Min(maxSamplesToRead, mSampleCount*mChannelCount - mCurrentDataPos);
	if(oSamplesRead) *oSamplesRead = shortsToRead / mChannelCount;
	if(oInterleaved) *oInterleaved = true;
	if(oType) *oType = mDataType;
	FixedArray<const void*> resultPtrs{mData.Begin + mCurrentDataPos};
	mCurrentDataPos += shortsToRead;
	if(shortsToRead < maxSamplesToRead) mCurrentDataPos = 0;
	return resultPtrs;
}

}}}

INTRA_WARNING_POP
