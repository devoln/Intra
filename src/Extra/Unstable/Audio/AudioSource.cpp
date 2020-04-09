#include "AudioSource.h"
#include "SampleConversion.h"

#include "Intra/Range/Operations.h"

INTRA_BEGIN
index_t SeparateFloatAudioSource::GetInterleavedSamples(Span<float> outFloats)
{
	INTRA_PRECONDITION(mChannelCount < 16);
	if(mChannelCount == 1) return GetUninterleavedSamples({outFloats});

	float tempSamples[4096];
	const index_t samplesPerChannel = LengthOf(tempSamples) / mChannelCount;
	Span<float> channels[16];
	const auto channelCSpan = Take(channels, mChannelCount).ReinterpretUnsafe<CSpan<float>>();
	for(uint16 i = 0; i < mChannelCount; i++)
		channels[i] = Drop(tempSamples, samplesPerChannel*i).Take(samplesPerChannel);

	index_t totalSamplesRead = 0;
	while(!outFloats.Empty())
	{
		const auto samplesRead = GetUninterleavedSamples(Take(channels, mChannelCount));
		totalSamplesRead += samplesRead;
		if(samplesRead < samplesPerChannel) break;
		InterleaveFloats(outFloats.Take(samplesRead*mChannelCount), channelCSpan);
	}
	return totalSamplesRead;
}

index_t SeparateFloatAudioSource::GetInterleavedSamples(Span<short> outShorts)
{
	INTRA_PRECONDITION(mChannelCount < 16);
	float tempSamples[16384];
	const index_t samplesPerChannel = FMin(LengthOf(tempSamples), outShorts.Length()) / mChannelCount;
	Span<float> channels[16];
	const auto channelCSpan = Take(channels, mChannelCount).ReinterpretUnsafe<CSpan<float>>();
	for(uint16 i = 0; i < mChannelCount; i++)
		channels[i] = Drop(tempSamples, samplesPerChannel*i).Take(samplesPerChannel);

	index_t totalSamplesRead = 0;
	while(!outShorts.Empty())
	{
		const auto samplesRead = GetUninterleavedSamples(Take(channels, mChannelCount));
		totalSamplesRead += samplesRead;
		if(samplesRead < samplesPerChannel) break;
		InterleaveFloatsCastToShorts(outShorts.TakeAdvance(samplesRead*mChannelCount), channelCSpan);
	}
	return totalSamplesRead;
}
INTRA_END
