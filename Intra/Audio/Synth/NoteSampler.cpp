#include "NoteSampler.h"
#include "Range/Mutation/Fill.h"
#include "Range/Mutation/Transform.h"

namespace Intra { namespace Audio { namespace Synth {

Span<float> NoteSampler::operator()(Span<float> dst, bool add)
{
	auto notProcessedPart = dst.Drop(ADSR.SamplesLeft());
	dst = dst.Take(ADSR.SamplesLeft());
	if(add && !Modifiers.Empty())
	{
		float tempArr[1024];
		while(!dst.Full() && !Empty())
		{
			auto tempDst = Range::Take(tempArr, dst.Length());
			fill(tempDst, false);
			applyModifiers(tempDst);
			AddAdvance(dst, tempDst);
		}
		return dst;
	}
	fill(dst, add);
	applyModifiers(dst);
	return notProcessedPart;
}

size_t NoteSampler::operator()(Span<float> dstLeft, Span<float> dstRight, bool add)
{
	size_t sampleCount = 0;
	dstLeft = dstLeft.Take(ADSR.SamplesLeft());
	dstRight = dstRight.Take(ADSR.SamplesLeft());
	float tempArr[1024];
	while(!dstLeft.Full() && !Empty())
	{
		auto tempDst = Range::Take(tempArr, dstLeft.Length());
		fill(tempDst, false);
		sampleCount += tempDst.Length();
		applyModifiers(tempDst);
		if(add)
		{
			AddMultiplied(dstLeft.TakeAdvance(tempDst.Length()), tempDst, 0.5f - Pan*0.5f);
			AddMultiplied(dstRight.TakeAdvance(tempDst.Length()), tempDst, 0.5f + Pan*0.5f);
		}
		else
		{
			Multiply(dstLeft.TakeAdvance(tempDst.Length()), tempDst, 0.5f - Pan*0.5f);
			Multiply(dstRight.TakeAdvance(tempDst.Length()), tempDst, 0.5f + Pan*0.5f);
		}
	}
	return sampleCount;
}

void NoteSampler::fill(Span<float> dst, bool add)
{
#ifdef INTRA_DEBUG
	if(!add) Fill(dst, 1000000);
#endif
	for(size_t i = 0; i < WaveTableSamplers.Length(); i++)
	{
		auto remainder = WaveTableSamplers[i](dst, add);
		if(remainder != null)
		{
			if(!add) FillZeros(remainder);
			WaveTableSamplers.RemoveUnordered(i--);
		}
		add = true;
	}
	for(size_t i = 0; i < GenericSamplers.Length(); i++)
	{
		auto remainder = GenericSamplers[i](dst, add);
		if(remainder != null)
		{
			if(!add) FillZeros(remainder);
			GenericSamplers.RemoveUnordered(i--);
		}
		add = true;
	}
	if(!add) FillZeros(dst);
}

void NoteSampler::applyModifiers(Span<float> dst)
{
	for(auto& mod: Modifiers) mod(dst);
	if(ADSR)
	{
		ADSR(dst);
		if(ADSR.SamplesLeft() == 0)
		{
			WaveTableSamplers = null;
			GenericSamplers = null;
		}
	}
}


void NoteSampler::MultiplyPitch(float freqMultiplier)
{
	for(auto& wave: WaveTableSamplers) wave.MultiplyPitch(freqMultiplier);
	for(auto& sampler: GenericSamplers) sampler.MultiplyPitch(freqMultiplier);
}

void NoteSampler::NoteRelease()
{
	for(auto& sampler: GenericSamplers) sampler.NoteRelease();
	if(ADSR) ADSR.NoteRelease();
}

}}}
