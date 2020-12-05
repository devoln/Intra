#include "IntraX/Unstable/Audio/Sources/Wave.h"
#include "IntraX/Unstable/Audio/SampleConversion.h"

#include "Intra/Range/Map.h"
#include "Intra/Math/Math.h"
#include "IntraX/Utils/Endianess.h"

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

	mData = srcFileData.Drop(sizeof(WaveHeader)).ReinterpretUnsafe<const byte>().Take(index_t(header.DataSize));
	mChannelCount = short(header.Channels);
	mSampleRate = int(header.SampleRate);
	mSampleCount = index_t(header.DataSize / (sizeof(short)*size_t(mChannelCount)));
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

index_t Wave::GetInterleavedSamples(Span<short> outShorts)
{
	INTRA_PRECONDITION(!outShorts.Empty());
	index_t valuesRead = 0;
	if(mDataType == ValueType::SNorm16)
	{
		const auto srcShorts = mData.ReinterpretUnsafe<const short>().Drop(mCurrentDataPos);
		valuesRead = CopyTo(srcShorts, outShorts);
	}
	else if(mDataType == ValueType::Float)
	{
		const auto srcFloats = mData.ReinterpretUnsafe<const float>().Drop(mCurrentDataPos).Take(outShorts.Length());
		valuesRead = srcFloats.Length();
		const auto srcShorts = Map(srcFloats, [](float x) {return short(x*32767);});
		CopyTo(srcShorts, outShorts.Take(valuesRead));
	}

	mCurrentDataPos += valuesRead;
	if(valuesRead < outShorts.Length()) mCurrentDataPos = 0;
	return index_t(size_t(valuesRead) / size_t(mChannelCount));
}

index_t Wave::GetInterleavedSamples(Span<float> outFloats)
{
	INTRA_PRECONDITION(!outFloats.Empty());
	index_t valuesRead = 0;
	if(mDataType == ValueType::SNorm16)
	{
		const auto srcShorts = mData.ReinterpretUnsafe<const short>().Drop(mCurrentDataPos).Take(outFloats.Length());
		valuesRead = srcShorts.Length();
		const auto srcFloats = Map(srcShorts, [](float x) {return x*(1/32767.0f);});
		CopyTo(srcFloats, outFloats.Take(valuesRead));
	}
	else if(mDataType == ValueType::Float)
	{
		const auto srcFloats = mData.ReinterpretUnsafe<const float>().Drop(mCurrentDataPos);
		valuesRead = CopyTo(srcFloats, outFloats);
	}
	return index_t(size_t(valuesRead) / size_t(mChannelCount));
}

index_t Wave::GetUninterleavedSamples(CSpan<Span<float>> outFloatChannels)
{
	INTRA_PRECONDITION(outFloatChannels.Length() == mChannelCount);
	const auto outSamplesCount = outFloatChannels.First().Length();
	for(index_t i = 1; i < mChannelCount; i++)
	{
		INTRA_PRECONDITION(outFloatChannels[i].Length() == outSamplesCount);
	}

	index_t valuesRead = 0;
	if(mDataType == ValueType::SNorm16)
	{
		const auto srcShorts = mData.ReinterpretUnsafe<const short>().Drop(mCurrentDataPos)
			.Take(outFloatChannels.Length()*outSamplesCount);
		FixedArray<Span<float>> outFloatChannelsTempSpans = outFloatChannels;
		DeinterleaveShortsCastToFloats(srcShorts, outFloatChannelsTempSpans);
		valuesRead = srcShorts.Length();
	}
	else if(mDataType == ValueType::Float)
	{
		const auto srcFloats = mData.ReinterpretUnsafe<const float>().Drop(mCurrentDataPos)
			.Take(outFloatChannels.Length()*outSamplesCount);
		FixedArray<Span<float>> outFloatChannelsTempSpans = outFloatChannels;
		DeinterleaveFloats(srcFloats, outFloatChannelsTempSpans);
		valuesRead = srcFloats.Length();
	}
	return valuesRead / mChannelCount;
}

FixedArray<const void*> Wave::GetRawSamplesData(Size maxSamplesToRead,
	Optional<ValueType&> oType, Optional<bool&> oInterleaved, Optional<index_t&> oSamplesRead)
{
	const auto shortsToRead = Min(size_t(maxSamplesToRead), size_t(mSampleCount*mChannelCount - mCurrentDataPos));
	if(oSamplesRead) oSamplesRead.Unwrap() = index_t(shortsToRead / size_t(mChannelCount));
	if(oInterleaved) oInterleaved.Unwrap() = true;
	if(oType) oType.Unwrap() = mDataType;
	FixedArray<const void*> resultPtrs{mData.Begin + mCurrentDataPos};
	mCurrentDataPos += shortsToRead;
	if(shortsToRead < size_t(maxSamplesToRead)) mCurrentDataPos = 0;
	return resultPtrs;
}



void WriteWave(IAudioSource& source, OutputStream& stream, ValueType sampleType)
{
	WaveHeader header;
	SpanOfBuffer(header.RIFF) << "RIFF";
	SpanOfBuffer(header.WAVE) << "WAVE";
	SpanOfBuffer(header.data) << "data";
	SpanOfBuffer(header.fmt) << "fmt ";
	header.Channels = uint16(source.ChannelCount());
	header.SampleRate = uint32(source.SampleRate());
	uint16 bpp = sampleType.Size();
	header.BitsPerSample =  uint16(bpp*8);
	header.DataSize = uint32(source.SamplesLeft().Unwrap()*bpp*header.Channels); //TODO: handle the case when source length is unknown
	header.BlockAlign = uint16(bpp*header.Channels);
	header.BytesPerSec = unsigned(bpp*header.Channels)*header.SampleRate;
	header.FormatTag = uint16(sampleType != ValueType::Float? 1: 3);
	header.FormatChunkSize = 16;
	header.WaveformChunkSize = unsigned(sizeof(WaveHeader) - sizeof(header.RIFF) - sizeof(header.WaveformChunkSize) + header.DataSize);

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
			const auto samplesRead = source.GetInterleavedSamples(samples);
			stream.RawWriteFrom(Take(samples, samplesRead*header.Channels));
			if(samplesRead < index_t(1024 / header.Channels)) return;
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
