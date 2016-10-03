#include "Sound/SoundBuilder.h"
#include "Algorithms/Algorithms.h"
#include "Algorithms/Range.h"

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

SoundBuffer::SoundBuffer(size_t sampleCount, uint sampleRate, ArrayRange<const float> initData):
	SampleRate(sampleRate), Samples()
{
	if(!initData.Empty()) Samples.AddLastRange(initData);
	Samples.SetCount(sampleCount);
}

void SoundBuffer::CopyFrom(size_t startSample, size_t sampleCount, SoundBuffer* src, size_t srcStartSample)
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
	if(Samples==null)
	{
		core::memset(outSamples.Begin, 0, outSamples.Length()*sizeof(outSamples[0]));
		return;
	}
	const auto numSamples = Math::Min(outSamples.Length(), Samples.Count()-first);
	Algo::Cast(outSamples.Take(numSamples), Samples(first, $).Take(numSamples));
}

void SoundBuffer::ShiftSamples(intptr samplesToShift)
{
	if(samplesToShift==0 || Samples==null) return;
	if(size_t(Math::Abs(samplesToShift))>=Samples.Count()) {Clear(); return;}
	if(samplesToShift<0)
	{
		const size_t firstFreeSampleIndex = size_t(intptr(Samples.Count())+samplesToShift);
		core::memmove(Samples.Data(), Samples.Data()-samplesToShift, firstFreeSampleIndex*sizeof(float));
		core::memset(Samples.Data()+firstFreeSampleIndex, 0, size_t(-samplesToShift)*sizeof(float));
		return;
	}
	core::memmove(Samples.Data()+samplesToShift, Samples.Data(), (Samples.Count()-size_t(samplesToShift))*sizeof(float));
	core::memset(Samples.Data(), 0, size_t(samplesToShift)*sizeof(float));
}

void SoundBuffer::Clear(size_t startSample, size_t sampleCount)
{
	if(startSample>=Samples.Count()) return;
	if(sampleCount == ~size_t(0)) sampleCount = Samples.Count()-startSample;
	core::memset(&Samples[startSample], 0, sampleCount*sizeof(float));
}


//static inline int NOK(int a, int b) {return a*b/NOD(a, b);}

static Array<float> get_sine_periods(uint sampleRate, float phase, uint frequency, size_t maxSamples)
{
	uint nod = Math::GreatestCommonDivisor(sampleRate, frequency);
	const uint fpsamples = Math::Min(sampleRate/nod, uint(maxSamples));
	Array<float> fullPeriods(fpsamples);
	const float da = float(PI*2.0*frequency/sampleRate);
	for(uint q=0; q<fpsamples; q++)
		fullPeriods.AddLast(Sin(da*float(q)+phase));
	return fullPeriods;
}

void SoundBuffer::Pulse(uint frequency, float phase, size_t startSample, size_t sampleCount)
{
	if(sampleCount==Meta::NumericLimits<size_t>::Max()) sampleCount = Samples.Count()-startSample;
	const size_t endSample = startSample+sampleCount;
	const auto fullPeriods = get_sine_periods(SampleRate, phase, frequency, sampleCount);
	const size_t fpsamples = fullPeriods.Count(), fpcount=sampleCount/fpsamples;

	size_t s = startSample;
	for(size_t i=0; i<fpcount; i++)
		for(size_t j=0; j<fpsamples; j++)
			Samples[s++] *= fullPeriods[j];

	for(size_t j=0; s<endSample; j++)
		Samples[s++] *= fullPeriods[j];
}

void SoundBuffer::MixWith(const SoundBuffer* rhs, size_t lhsStartSample, size_t rhsStartSample, size_t sampleCount)
{
	if(rhs==null) return;
	size_t endSample = lhsStartSample+sampleCount;
	if(Samples.Count()<endSample)
	{
		size_t oldCount=Samples.Count();
		Samples.SetCount(endSample);
		core::memset(&Samples[oldCount], 0, (endSample-oldCount)*sizeof(Samples[0]));
	}
	auto dst = Samples.begin()+lhsStartSample, dstEnd = dst+endSample;
	auto src = rhs->Samples.begin()+rhsStartSample, srcEnd=rhs->Samples.end();
	dstEnd = Math::Min(dstEnd, dst+(srcEnd-src));

	Algo::Add<float>({dst, dstEnd}, {src, size_t(dstEnd-dst)});
}

core::pair<float, float> SoundBuffer::GetMinMax(size_t startSample, size_t sampleCount) const
{
	if(startSample>=Samples.Count()) return {-1,1};
	if(sampleCount>Samples.Count()-startSample) sampleCount = Samples.Count()-startSample;
	size_t endSample = Math::Min(startSample+sampleCount, Samples.Count());

	core::pair<float, float> result;
	Algo::MiniMax(Samples(startSample, endSample), &result.first, &result.second);
	return result;
}

void SoundBuffer::SetMinMax(float newMin, float newMax, size_t startSample, size_t sampleCount, core::pair<float, float> minMax)
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
