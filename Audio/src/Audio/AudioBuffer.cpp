#include "Audio/AudioBuffer.h"
#include "Algo/Reduction.h"
#include "Algo/Mutation/Cast.h"
#include "Range/Generators/ArrayRange.h"
#include "Algo/Mutation/Fill.h"
#include "Algo/Mutation/Transform.h"

namespace Intra { namespace Audio {

using namespace Math;
using namespace Algo;
using namespace Range;

AudioBuffer::AudioBuffer(size_t sampleCount,
	uint sampleRate, ArrayRange<const float> initData):
	SampleRate(sampleRate), Samples()
{
	if(!initData.Empty()) Samples.AddLastRange(initData);
	Samples.SetCount(sampleCount);
}

void AudioBuffer::CopyFrom(size_t startSample,
	size_t sampleCount, AudioBuffer* src, size_t srcStartSample)
{
	INTRA_ASSERT(src!=null);
	INTRA_ASSERT(srcStartSample+sampleCount<=src->Samples.Count());
	if(startSample+sampleCount>Samples.Count())
		Samples.SetCount(startSample+sampleCount);
	CopyTo(Drop(src->Samples, srcStartSample), sampleCount, Drop(Samples, startSample));
}

void AudioBuffer::ConvertToShorts(size_t first, ArrayRange<short> outSamples) const
{
	const auto numSamples = Min(outSamples.Length(), Samples.Count()-first);
	CastFromNormalized(outSamples.Take(numSamples), Drop(Samples, first).Take(numSamples));
}

void AudioBuffer::CastToShorts(size_t first, ArrayRange<short> outSamples) const
{
	CastToAdvance(Drop(Samples, first).Take(outSamples.Length()), outSamples);
	FillZeros(outSamples);
}

void AudioBuffer::ShiftSamples(intptr samplesToShift)
{
	if(samplesToShift==0 || Samples==null) return;
	if(size_t(Abs(samplesToShift))>=Samples.Count()) {Clear(); return;}
	if(samplesToShift<0)
	{
		const size_t firstFreeSampleIndex = size_t(intptr(Samples.Count())+samplesToShift);
		CopyTo(Drop(Samples, size_t(-samplesToShift)), firstFreeSampleIndex, Samples);
		Clear(firstFreeSampleIndex, size_t(-samplesToShift));
		return;
	}
	C::memmove(Samples.Data()+samplesToShift, Samples.Data(),
		(Samples.Count()-size_t(samplesToShift))*sizeof(float));
	Clear(0, size_t(samplesToShift));
}

void AudioBuffer::Clear(size_t startSample, size_t sampleCount)
{
	FillZeros(Drop(Samples, startSample).Take(sampleCount));
}

void AudioBuffer::MixWith(const AudioBuffer* rhs,
	size_t lhsStartSample, size_t rhsStartSample, size_t sampleCount)
{
	if(rhs==null) return;
	size_t endSample = lhsStartSample+sampleCount;
	if(Samples.Count()<endSample) Samples.SetCount(endSample);
	auto dst = Samples.begin()+lhsStartSample, dstEnd = dst+endSample;
	auto src = rhs->Samples.begin()+rhsStartSample, srcEnd=rhs->Samples.end();
	dstEnd = Min(dstEnd, dst+(srcEnd-src));

	Add({dst, dstEnd}, {src, size_t(dstEnd-dst)});
}

Meta::Pair<float, float> AudioBuffer::GetMinMax(size_t startSample, size_t sampleCount) const
{
	if(startSample>=Samples.Count()) return {-1,1};
	if(sampleCount>Samples.Count()-startSample) sampleCount = Samples.Count()-startSample;
	size_t endSample = Min(startSample+sampleCount, Samples.Count());

	Meta::Pair<float, float> result;
	MiniMax(Samples(startSample, endSample), &result.first, &result.second);
	return result;
}

void AudioBuffer::SetMinMax(float newMin, float newMax,
	size_t startSample, size_t sampleCount, Meta::Pair<float, float> minMax)
{
	if(startSample>=Samples.Count()) return;
	if(sampleCount==Meta::NumericLimits<size_t>::Max()) sampleCount = Samples.Count()-startSample;
	if(minMax.first==0 && minMax.second==0) minMax = GetMinMax();
	const size_t endSample = Min(startSample+sampleCount, Samples.Count());

	const auto multiplyer = (newMax-newMin) / (minMax.second-minMax.first);
	const auto add = -minMax.first*multiplyer+newMin;

	MulAdd(Samples(startSample, endSample), multiplyer, add);
}

}}
