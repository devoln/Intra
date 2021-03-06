﻿#include "Audio/AudioBuffer.h"
#include "Range/Reduction.h"
#include "Range/Mutation/Cast.h"
#include "Utils/Span.h"
#include "Range/Mutation/Fill.h"
#include "Range/Mutation/Transform.h"

namespace Intra { namespace Audio {

AudioBuffer::AudioBuffer(size_t sampleCount,
	uint sampleRate, CSpan<float> initData):
	SampleRate(sampleRate), Samples()
{
	if(!initData.Empty()) Samples.AddLastRange(initData);
	Samples.SetCount(sampleCount);
}

void AudioBuffer::CopyFrom(size_t startSample,
	size_t sampleCount, AudioBuffer* src, size_t srcStartSample)
{
	INTRA_DEBUG_ASSERT(src != null);
	INTRA_DEBUG_ASSERT(srcStartSample + sampleCount <= src->Samples.Count());
	if(startSample + sampleCount > Samples.Count())
		Samples.SetCount(startSample + sampleCount);
	CopyTo(src->Samples.Drop(srcStartSample), sampleCount, Samples.Drop(startSample));
}

void AudioBuffer::ConvertToShorts(size_t first, Span<short> outSamples) const
{
	const auto numSamples = Math::Min(outSamples.Length(), Samples.Count()-first);
	CastFromNormalized(outSamples.Take(numSamples), Samples.Drop(first).Take(numSamples));
}

void AudioBuffer::CastToShorts(size_t first, Span<short> outSamples) const
{
	CastToAdvance(Samples.Drop(first).Take(outSamples.Length()), outSamples);
	FillZeros(outSamples);
}

void AudioBuffer::ShiftSamples(intptr samplesToShift)
{
	if(samplesToShift==0 || Samples==null) return;
	if(size_t(Math::Abs(samplesToShift))>=Samples.Count()) {Clear(); return;}
	if(samplesToShift<0)
	{
		const size_t firstFreeSampleIndex = size_t(intptr(Samples.Count())+samplesToShift);
		CopyTo(Samples.Drop(size_t(-samplesToShift)), firstFreeSampleIndex, Samples);
		Clear(firstFreeSampleIndex, size_t(-samplesToShift));
		return;
	}
	CopyTo(Samples, Samples.Drop(size_t(samplesToShift)));
	//C::memmove(Samples.Data()+samplesToShift, Samples.Data(),
		//(Samples.Count()-size_t(samplesToShift))*sizeof(float));
	Clear(0, size_t(samplesToShift));
}

void AudioBuffer::Clear(size_t startSample, size_t sampleCount)
{
	FillZeros(Samples.Drop(startSample).Take(sampleCount));
}

void AudioBuffer::MixWith(const AudioBuffer& rhs,
	size_t lhsStartSample, size_t rhsStartSample, size_t sampleCount)
{
	size_t endSample = lhsStartSample+sampleCount;
	if(Samples.Count()<endSample) Samples.SetCount(endSample);
	auto dst = Samples.Drop(lhsStartSample).Take(sampleCount);
	auto src = rhs.Samples.Drop(rhsStartSample);
	Add(dst, src);
}

Pair<float, float> AudioBuffer::GetMinMax(size_t startSample, size_t sampleCount) const
{
	if(startSample >= Samples.Count()) return {-1, 1};
	return MiniMax(Samples.Drop(startSample).Take(sampleCount));
}

void AudioBuffer::SetMinMax(float newMin, float newMax,
	size_t startSample, size_t sampleCount, Pair<float, float> minMax)
{
	auto range = Samples.Drop(startSample).Take(sampleCount);
	if(range.Empty()) return;
	if(minMax.first == 0 && minMax.second == 0) minMax = MiniMax(range.AsConstRange());

	const float multiplyer = (newMax - newMin) / (minMax.second - minMax.first);
	const float add = -minMax.first*multiplyer + newMin;

	MulAdd(range, multiplyer, add);
}

}}
