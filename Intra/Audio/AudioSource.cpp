#include "Audio/AudioSource.h"
#include "SampleConversion.h"

namespace Intra { namespace Audio {



size_t SeparateFloatAudioSource::GetInterleavedSamples(Span<float> outFloats)
{
	if(mChannelCount == 1) return GetUninterleavedSamples({outFloats});

	float tempSamples[4096];
	const size_t samplesPerChannel = Concepts::LengthOf(tempSamples) / mChannelCount;
	Span<float> channels[16];
	const auto channelCSpan = Range::Take(channels, mChannelCount).Reinterpret<CSpan<float>>();
	INTRA_DEBUG_ASSERT(mChannelCount < 16);
	for(ushort i = 0; i<mChannelCount; i++)
		channels[i] = Range::Drop(tempSamples, samplesPerChannel*i).Take(samplesPerChannel);

	size_t totalSamplesRead = 0;
	while(!outFloats.Empty())
	{
		const size_t samplesRead = GetUninterleavedSamples(Range::Take(channels, mChannelCount));
		totalSamplesRead += samplesRead;
		InterleaveFloats(outFloats.Take(samplesRead*mChannelCount), channelCSpan);
	}
	return totalSamplesRead;
}

size_t SeparateFloatAudioSource::GetInterleavedSamples(Span<short> outShorts)
{
	float tempSamples[4096];
	const size_t samplesPerChannel = Concepts::LengthOf(tempSamples) / mChannelCount;
	Span<float> channels[16];
	const auto channelCSpan = Range::Take(channels, mChannelCount).Reinterpret<CSpan<float>>();
	INTRA_DEBUG_ASSERT(mChannelCount < 16);
	for(ushort i = 0; i<mChannelCount; i++)
		channels[i] = Range::Drop(tempSamples, samplesPerChannel*i).Take(samplesPerChannel);

	size_t totalSamplesRead = 0;
	while(!outShorts.Empty())
	{
		const size_t samplesRead = GetUninterleavedSamples(Range::Take(channels, mChannelCount));
		totalSamplesRead += samplesRead;
		InterleaveFloatsCastToShorts(outShorts.Take(samplesRead*mChannelCount), channelCSpan);
	}
	return totalSamplesRead;
}

}}
