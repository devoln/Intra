#include "NoteSampler.h"

#include <Range/Mutation/Fill.h>
#include <Range/Mutation/Transform.h>

size_t NoteSampler::GenerateMono(Span<float> ioDst)
{
	auto notProcessedPart = ioDst.Drop(ADSR.SamplesLeft());
	ioDst = ioDst.Take(ADSR.SamplesLeft());
	size_t samplesToProcess = ioDst.Length();
	if(!Modifiers.Empty() || ADSR)
	{
		float tempArr[1024] = {0};
		while(!ioDst.Full() && !Empty())
		{
			auto tempDst = Take(tempArr, ioDst.Length());
			fill(tempDst);
			applyModifiers(tempDst);
			AddAdvance(ioDst, tempDst);
		}
		return samplesToProcess;
	}
	fill(ioDst);
	applyModifiers(ioDst);
	return samplesToProcess;
}

size_t NoteSampler::GenerateStereo(Span<float> dstLeft, Span<float> dstRight, Span<float> dstReverb)
{
	if(Modifiers.Empty() && !ADSR && GenericSamplers.Empty())
	{
		const size_t sampleCount = Min(ADSR.SamplesLeft(), dstLeft.Length());
		fillStereo(dstLeft.Take(sampleCount), dstRight.Take(sampleCount), dstReverb.Take(sampleCount));
		return sampleCount;
	}

	//TODO: добавить раздельный синтез каналов dstLeft и dstRight
	size_t sampleCount = 0;
	dstLeft = dstLeft.Take(ADSR.SamplesLeft());
	dstRight = dstRight.Take(ADSR.SamplesLeft());
	float tempArr[1024] = {0};
	while(!dstLeft.Full() && !Empty())
	{
		auto tempDst = Take(tempArr, dstLeft.Length());
		fill(tempDst);
		sampleCount += tempDst.Length();
		applyModifiers(tempDst);
		AddMultiplied(dstLeft.TakeAdvance(tempDst.Length()), tempDst, 0.5f - Pan*0.5f);
		AddMultiplied(dstRight.TakeAdvance(tempDst.Length()), tempDst, 0.5f + Pan*0.5f);
		AddMultiplied(dstReverb.TakeAdvance(tempDst.Length()), tempDst, ReverbCoeff);
	}
	return sampleCount;
}

void NoteSampler::fill(Span<float> ioDst)
{
	for(size_t i = 0; i < WaveTableSamplers.Length(); i++)
	{
		auto remainder = WaveTableSamplers[i].GenerateMono(ioDst);
		if(remainder != null)
		{
			WaveTableSamplers.RemoveUnordered(i--);
		}
	}
	for(size_t i = 0; i < GenericSamplers.Length(); i++)
	{
		const size_t samplesProcessed = GenericSamplers[i]->GenerateMono(ioDst);
		if(samplesProcessed < ioDst.Length())
			GenericSamplers.RemoveUnordered(i--);
	}
}

void NoteSampler::fillStereo(Span<float> ioDstLeft, Span<float> ioDstRight, Span<float> ioDstReverb)
{
	for(size_t i = 0; i < WaveTableSamplers.Length(); i++)
	{
		size_t samplesProcessed = WaveTableSamplers[i].GenerateStereo(ioDstLeft, ioDstRight, ioDstReverb);
		if(samplesProcessed != ioDstLeft.Length())
			WaveTableSamplers.RemoveUnordered(i--);
	}
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
	for(auto& sampler: GenericSamplers) sampler->MultiplyPitch(freqMultiplier);
}

void NoteSampler::NoteRelease()
{
	for(auto& sampler: WaveTableSamplers) sampler.NoteRelease();
	for(auto& sampler: GenericSamplers) sampler->NoteRelease();
	if(ADSR) ADSR.NoteRelease();
}

void NoteSampler::SetPan(float pan)
{
	for(auto& sampler: WaveTableSamplers) sampler.SetPan(pan);
	//for(auto& sampler: GenericSamplers) sampler->SetPan(pan);
	Pan = pan;
}

void NoteSampler::MultiplyVolume(float volumeMultiplier)
{
	for(auto& sampler: WaveTableSamplers) sampler.MultiplyVolume(volumeMultiplier);
	//for(auto& sampler: GenericSamplers) sampler.MultiplyVolume(pan);
}

void NoteSampler::SetReverbCoeff(float reverbCoeff)
{
	for(auto& sampler: WaveTableSamplers) sampler.SetReverbCoeff(reverbCoeff);
	ReverbCoeff = reverbCoeff;
}
