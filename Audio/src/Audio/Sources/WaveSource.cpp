#include "Audio/Sources/WaveSource.h"
#include "Math/Math.h"
#include "Platform/Intrinsics.h"
#include "Platform/CppWarnings.h"
#include "Platform/Endianess.h"

namespace Intra { namespace Audio { namespace Sources {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#ifndef INTRA_NO_WAVE_LOADER

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


WaveSource::WaveSource(ArrayRange<const byte> srcFileData):
	mData(srcFileData), mSampleCount(0), mCurrentDataPos(0)
{
	const WaveHeader& header = *reinterpret_cast<const WaveHeader*>(mData.Begin);

	if(C::memcmp(header.RIFF, "RIFF", sizeof(header.RIFF))!=0 ||
		C::memcmp(header.WAVE, "WAVE", sizeof(header.WAVE))!=0 ||
		C::memcmp(header.fmt, "fmt ", sizeof(header.fmt))!=0 ||
		C::memcmp(header.data, "data", sizeof(header.data))!=0) return;
	if(mData.Length()!=header.DataSize+sizeof(WaveHeader)) return;

	mChannelCount = ushort(header.Channels);
	mSampleRate = header.SampleRate;
	mSampleCount = header.DataSize/sizeof(short)/mChannelCount;
}

size_t WaveSource::GetInterleavedSamples(ArrayRange<short> outShorts)
{
	INTRA_ASSERT(!outShorts.Empty());
	const auto shortsToRead = Math::Min(outShorts.Length(), mSampleCount*mChannelCount-mCurrentDataPos);
	const short* const streamStart = reinterpret_cast<const short*>(mData.Begin+sizeof(WaveHeader));
	Algo::CopyTo(ArrayRange<const short>(streamStart+mCurrentDataPos, shortsToRead), outShorts);
	mCurrentDataPos += shortsToRead;
	if(shortsToRead<outShorts.Length()) mCurrentDataPos=0;
	return shortsToRead/mChannelCount;
}

size_t WaveSource::GetInterleavedSamples(ArrayRange<float> outFloats)
{
	INTRA_ASSERT(!outFloats.Empty());
	Array<short> outShorts;
	outShorts.SetCountUninitialized(outFloats.Length());
	auto result = GetInterleavedSamples(outShorts);
	for(size_t i=0; i<outFloats.Length(); i++)
		outFloats[i] = (outShorts[i]+0.5f)/32767.5f;
	return result;
}

size_t WaveSource::GetUninterleavedSamples(ArrayRange<const ArrayRange<float>> outFloats)
{
	INTRA_ASSERT(outFloats.Length()==mChannelCount);
	const size_t outSamplesCount = outFloats.First().Length();
	for(size_t i=1; i<mChannelCount; i++)
	{
		INTRA_ASSERT(outFloats[i].Length()==outSamplesCount);
	}

	Array<float> outShorts;
	outShorts.SetCountUninitialized(outSamplesCount*mChannelCount);
	auto result = GetInterleavedSamples(outShorts);
	for(size_t i=0, j=0; i<outShorts.Count(); i++)
	{
		for(ushort c=0; c<mChannelCount; c++)
			outFloats[c][i] = (outShorts[j++]+0.5f)/32767.5f;
	}
	return result;
}

Array<const void*> WaveSource::GetRawSamplesData(size_t maxSamplesToRead,
	ValueType* oType, bool* oInterleaved, size_t* oSamplesRead)
{
	const auto shortsToRead = Math::Min(maxSamplesToRead, mSampleCount*mChannelCount-mCurrentDataPos);
	if(oSamplesRead!=null) *oSamplesRead = shortsToRead/mChannelCount;
	if(oInterleaved!=null) *oInterleaved = true;
	if(oType!=null) *oType = ValueType::Short;
	Array<const void*> resultPtrs;
	const short* const streamStart = reinterpret_cast<const short*>(mData.Begin+sizeof(WaveHeader));
	resultPtrs.AddLast(streamStart+mCurrentDataPos);
	mCurrentDataPos += shortsToRead;
	if(shortsToRead<maxSamplesToRead) mCurrentDataPos = 0;
	return resultPtrs;
}

#endif

INTRA_WARNING_POP

}}}
