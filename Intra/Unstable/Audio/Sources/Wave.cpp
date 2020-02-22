#include "Audio/Sources/Wave.h"
#include "Audio/SampleConversion.h"

#include "Core/Range/Map.h"

#include "Math/Math.h"

#include "Utils/Endianess.h"

INTRA_BEGIN
namespace Sources {
struct WaveHeader
{
	char RIFF[4];
	uint32LE WaveformChunkSize;
	char WAVE[4];

	char fmt[4];
	uint32LE FormatChunkSize;

	ushortLE FormatTag, Channels;
	uint32LE SampleRate, BytesPerSec;
	ushortLE BlockAlign, BitsPerSample;

	char data[4];
	uint32LE DataSize;
};


Wave::Wave(OnCloseResourceCallback onClose, CSpan<byte> srcFileData):
	BasicAudioSource(Move(onClose))
{
	const WaveHeader& header = *reinterpret_cast<const WaveHeader*>(srcFileData.Begin);

	const bool isValidHeader =
		StringView::FromBuffer(header.RIFF) == "RIFF" &&
		StringView::FromBuffer(header.WAVE) == "WAVE" &&
		StringView::FromBuffer(header.fmt) == "fmt " &&
		StringView::FromBuffer(header.data) == "data";
	
	if(!isValidHeader) return;

	mData = srcFileData.Drop(sizeof(WaveHeader)).Reinterpret<const byte>().Take(header.DataSize);
	mChannelCount = ushort(header.Channels);
	mSampleRate = header.SampleRate;
	mSampleCount = header.DataSize/sizeof(short)/mChannelCount;
	if(header.FormatTag == 1) switch(header.BitsPerSample)
	{
	case 8: mDataType = ValueType::SNorm8; break;
	case 16: mDataType = ValueType::SNorm16; break;
	case 24: mDataType = ValueType::SNorm24; break;
	case 32: mDataType = ValueType::SNorm32; break;
	default:
		mDataType = ValueType::Void;
		mSampleCount = 0;
		mData = null;
	}
	else if(header.FormatTag == 3)
	{
		if(header.BitsPerSample == 32) mDataType = ValueType::Float;
		else mDataType = ValueType::Double;
	}
}

size_t Wave::GetInterleavedSamples(Span<short> outShorts)
{
	INTRA_PRECONDITION(!outShorts.Empty());
	size_t valuesRead = 0;
	if(mDataType == ValueType::SNorm16)
	{
		const auto srcShorts = mData.Reinterpret<const short>().Drop(mCurrentDataPos);
		valuesRead = CopyTo(srcShorts, outShorts);
	}
	else if(mDataType == ValueType::Float)
	{
		const auto srcFloats = mData.Reinterpret<const float>().Drop(mCurrentDataPos).Take(outShorts.Length());
		valuesRead = srcFloats.Length();
		const auto srcShorts = Map(srcFloats, [](float x) {return short(x*32767);});
		CopyTo(srcShorts, outShorts.Take(valuesRead));
	}

	mCurrentDataPos += valuesRead;
	if(valuesRead < outShorts.Length()) mCurrentDataPos = 0;
	return valuesRead / mChannelCount;
}

size_t Wave::GetInterleavedSamples(Span<float> outFloats)
{
	INTRA_DEBUG_ASSERT(!outFloats.Empty());
	size_t valuesRead = 0;
	if(mDataType == ValueType::SNorm16)
	{
		const auto srcShorts = mData.Reinterpret<const short>().Drop(mCurrentDataPos).Take(outFloats.Length());
		valuesRead = srcShorts.Length();
		const auto srcFloats = Map(srcShorts, [](float x) {return x*(1/32767.0f);});
		CopyTo(srcFloats, outFloats.Take(valuesRead));
	}
	else if(mDataType == ValueType::Float)
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
	if(mDataType == ValueType::SNorm16)
	{
		const auto srcShorts = mData.Reinterpret<const short>().Drop(mCurrentDataPos)
			.Take(outFloatChannels.Length()*outSamplesCount);
		FixedArray<Span<float>> outFloatChannelsTempSpans = outFloatChannels;
		DeinterleaveShortsCastToFloats(srcShorts, outFloatChannelsTempSpans);
		valuesRead = srcShorts.Length();
	}
	else if(mDataType == ValueType::Float)
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
	ValueType* oType, bool* oInterleaved, size_t* oSamplesRead)
{
	const auto shortsToRead = Min(maxSamplesToRead, mSampleCount*mChannelCount - mCurrentDataPos);
	if(oSamplesRead) *oSamplesRead = shortsToRead / mChannelCount;
	if(oInterleaved) *oInterleaved = true;
	if(oType) *oType = mDataType;
	FixedArray<const void*> resultPtrs{mData.Begin + mCurrentDataPos};
	mCurrentDataPos += shortsToRead;
	if(shortsToRead < maxSamplesToRead) mCurrentDataPos = 0;
	return resultPtrs;
}



void WriteWave(IAudioSource& source, OutputStream& stream, ValueType sampleType)
{
	WaveHeader header;
	SpanOfBuffer(header.RIFF) << "RIFF";
	SpanOfBuffer(header.WAVE) << "WAVE";
	SpanOfBuffer(header.data) << "data";
	SpanOfBuffer(header.fmt) << "fmt ";
	header.Channels = ushort(source.ChannelCount());
	header.SampleRate = source.SampleRate();
	ushort bpp = sampleType.Size();
	header.BitsPerSample =  ushort(bpp*8);
	header.DataSize = uint(source.SamplesLeft()*bpp*header.Channels);
	header.BlockAlign = ushort(bpp*header.Channels);
	header.BytesPerSec = uint(bpp*header.Channels)*header.SampleRate;
	header.FormatTag = ushort(sampleType != ValueType::Float? 1: 3);
	header.FormatChunkSize = 16;
	header.WaveformChunkSize = uint(sizeof(WaveHeader) - sizeof(header.RIFF) - sizeof(header.WaveformChunkSize) + header.DataSize);

	if(header.FormatTag != 1)
	{
		INTRA_DEBUG_ASSERT(false); //Not supported yet
		return;
	}

	stream.RawWrite(header);

	if(sampleType == ValueType::Short || sampleType == ValueType::SNorm16)
	{
		short samples[1024];
		for(;;)
		{
			size_t samplesRead = source.GetInterleavedSamples(samples);
			stream.RawWriteFrom(Take(samples, samplesRead*header.Channels));
			if(samplesRead < size_t(1024/header.Channels)) return;
		}
	}

	/*if(sampleType == ValueType::Float)
	{
		float samples[1024];
		for(;;)
		{
			size_t samplesRead = source.GetInterleavedSamples(samples);
			stream.RawWriteFrom(Take(samples, samplesRead));
			if(samplesRead < 1024) return;
		}
	}*/

	INTRA_DEBUG_ASSERT(false); //Not supported yet
}
}
INTRA_END
