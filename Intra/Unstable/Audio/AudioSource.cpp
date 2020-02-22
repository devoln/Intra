#include "Audio/AudioSource.h"
#include "SampleConversion.h"

#include "Core/Range/Operations.h"

INTRA_BEGIN
size_t SeparateFloatAudioSource::GetInterleavedSamples(Span<float> outFloats)
{
	if(mChannelCount == 1) return GetUninterleavedSamples({outFloats});

	float tempSamples[4096];
	const size_t samplesPerChannel = LengthOf(tempSamples) / mChannelCount;
	Span<float> channels[16];
	const auto channelCSpan = Take(channels, mChannelCount).Reinterpret<CSpan<float>>();
	INTRA_DEBUG_ASSERT(mChannelCount < 16);
	for(ushort i = 0; i<mChannelCount; i++)
		channels[i] = Drop(tempSamples, samplesPerChannel*i).Take(samplesPerChannel);

	size_t totalSamplesRead = 0;
	while(!outFloats.Empty())
	{
		const size_t samplesRead = GetUninterleavedSamples(Take(channels, mChannelCount));
		totalSamplesRead += samplesRead;
		if(samplesRead < samplesPerChannel) break;
		InterleaveFloats(outFloats.Take(samplesRead*mChannelCount), channelCSpan);
	}
	return totalSamplesRead;
}

size_t SeparateFloatAudioSource::GetInterleavedSamples(Span<short> outShorts)
{
	float tempSamples[16384];
	const size_t samplesPerChannel = FMin(LengthOf(tempSamples), outShorts.Length()) / mChannelCount;
	Span<float> channels[16];
	const auto channelCSpan = Take(channels, mChannelCount).Reinterpret<CSpan<float>>();
	INTRA_DEBUG_ASSERT(mChannelCount < 16);
	for(ushort i = 0; i<mChannelCount; i++)
		channels[i] = Drop(tempSamples, samplesPerChannel*i).Take(samplesPerChannel);

	size_t totalSamplesRead = 0;
	while(!outShorts.Empty())
	{
		const size_t samplesRead = GetUninterleavedSamples(Take(channels, mChannelCount));
		totalSamplesRead += samplesRead;
		if(samplesRead < samplesPerChannel) break;
		InterleaveFloatsCastToShorts(outShorts.TakeAdvance(samplesRead*mChannelCount), channelCSpan);
	}
	return totalSamplesRead;
}
INTRA_END
