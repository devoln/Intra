#include "Sound/SoundBuilder.h"
#include "Algo/Reduction.h"
#include "Algo/Mutation/Cast.h"
#include "Range/ArrayRange.h"
#include "Algo/Mutation/Fill.h"
#include "Algo/Mutation/Transform.h"

#define OPTIMIZE

#ifdef INTRA_USE_PDO

#include "Math/Simd.h"

#if INTRA_PLATFORM_ARCH==INTRA_PLATFORM_X86 || INTRA_PLATFORM_ARCH==INTRA_PLATFORM_X86_64

#elif INTRA_PLATFORM_ARCH==INTRA_PLATFORM_ARM
#undef OPTIMIZE
#else
#undef OPTIMIZE
#endif

#endif

namespace Intra {

using namespace Math;

SoundBuffer::SoundBuffer(size_t sampleCount,
	uint sampleRate, ArrayRange<const float> initData):
	SampleRate(sampleRate), Samples()
{
	if(!initData.Empty()) Samples.AddLastRange(initData);
	Samples.SetCount(sampleCount);
}

void SoundBuffer::CopyFrom(size_t startSample,
	size_t sampleCount, SoundBuffer* src, size_t srcStartSample)
{
	INTRA_ASSERT(src!=null);
	INTRA_ASSERT(srcStartSample+sampleCount<=src->Samples.Count());
	if(startSample+sampleCount>Samples.Count())
		Samples.SetCount(startSample+sampleCount);
	Memory::CopyBits(
		Samples().Drop(startSample).Take(sampleCount),
		src->Samples.AsConstRange().Drop(srcStartSample).Take(sampleCount));
}

void SoundBuffer::ConvertToShorts(size_t first, ArrayRange<short> outSamples) const
{
	const auto numSamples = Math::Min(outSamples.Length(), Samples.Count()-first);
	Algo::CastFromNormalized(outSamples.Take(numSamples), Samples(first, first+numSamples));
}

void SoundBuffer::CastToShorts(size_t first, ArrayRange<short> outSamples) const
{
	Algo::CastToAdvance(Samples().Drop(first).Take(outSamples.Length()), outSamples);
	Algo::FillZeros(outSamples);
}

void SoundBuffer::ShiftSamples(intptr samplesToShift)
{
	if(samplesToShift==0 || Samples==null) return;
	if(size_t(Math::Abs(samplesToShift))>=Samples.Count()) {Clear(); return;}
	if(samplesToShift<0)
	{
		const size_t firstFreeSampleIndex = size_t(intptr(Samples.Count())+samplesToShift);
		Algo::CopyTo(Samples().Drop(size_t(-samplesToShift)).Take(firstFreeSampleIndex), Samples());
		Clear(firstFreeSampleIndex, size_t(-samplesToShift));
		return;
	}
	C::memmove(Samples.Data()+samplesToShift, Samples.Data(),
		(Samples.Count()-size_t(samplesToShift))*sizeof(float));
	Clear(0, size_t(samplesToShift));
}

void SoundBuffer::Clear(size_t startSample, size_t sampleCount)
{
	Algo::FillZeros(Samples().Drop(startSample).Take(sampleCount));
}

void SoundBuffer::MixWith(const SoundBuffer* rhs,
	size_t lhsStartSample, size_t rhsStartSample, size_t sampleCount)
{
	if(rhs==null) return;
	size_t endSample = lhsStartSample+sampleCount;
	if(Samples.Count()<endSample) Samples.SetCount(endSample);
	auto dst = Samples.begin()+lhsStartSample, dstEnd = dst+endSample;
	auto src = rhs->Samples.begin()+rhsStartSample, srcEnd=rhs->Samples.end();
	dstEnd = Math::Min(dstEnd, dst+(srcEnd-src));

	Algo::Add({dst, dstEnd}, {src, size_t(dstEnd-dst)});
}

Meta::Pair<float, float> SoundBuffer::GetMinMax(size_t startSample, size_t sampleCount) const
{
	if(startSample>=Samples.Count()) return {-1,1};
	if(sampleCount>Samples.Count()-startSample) sampleCount = Samples.Count()-startSample;
	size_t endSample = Math::Min(startSample+sampleCount, Samples.Count());

	Meta::Pair<float, float> result;
	Algo::MiniMax(Samples(startSample, endSample), &result.first, &result.second);
	return result;
}

void SoundBuffer::SetMinMax(float newMin, float newMax,
	size_t startSample, size_t sampleCount, Meta::Pair<float, float> minMax)
{
	if(startSample>=Samples.Count()) return;
	if(sampleCount==Meta::NumericLimits<size_t>::Max()) sampleCount = Samples.Count()-startSample;
	if(minMax.first==0 && minMax.second==0) minMax = GetMinMax();
	const size_t endSample = Math::Min(startSample+sampleCount, Samples.Count());

	const auto multiplyer = (newMax-newMin) / (minMax.second-minMax.first);
	const auto add = -minMax.first*multiplyer+newMin;

	Algo::MulAdd(Samples(startSample, endSample), multiplyer, add);
}

}
