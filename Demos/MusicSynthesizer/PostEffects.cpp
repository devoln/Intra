#include "PostEffects.hh"

#include <Utils/Span.h>
#include <Math/SineRange.h>
#include <Math/Math.h>
#include <Random/FastUniform.h>
#include <Container/Sequential/Array.h>
#include <Range/Mutation/Transform.h>

#include <Range/Sort/Quick.h>
#include <Range/Reduction.h>
#include <IO/Std.h>

namespace PostEffects {

void Echo::operator()(Span<float> inOutSamples, uint sampleRate) const
{
	Array<float> copy(inOutSamples);
	const double duration = double(inOutSamples.Length())/sampleRate;
	float t = 0;
	const float dt = 1.0f/float(sampleRate);
	while(!inOutSamples.Empty())
	{
		float st = t+Delay;
		if(st>=0 && st<duration)
		{
			inOutSamples.First() *= MainVolume;
			inOutSamples.First() += copy[uint(st*float(sampleRate))]*SecondaryVolume;
		}
		t += dt;
		inOutSamples.PopFirst();
	}
}

void FilterDrive::operator()(Span<float> inOutSamples, uint sampleRate) const
{
	(void)sampleRate;
	for(float& sample: inOutSamples)
		sample = Math::Atan(sample*K);
}

void FilterHP::operator()(Span<float> inOutSamples, uint sampleRate) const
{
	(void)sampleRate;
	float S = 0;
	for(float& sample: inOutSamples)
	{
		S *= K;
		S += sample - K*sample;
		sample -= S;
	}
}

void Fade::operator()(Span<float> inOutSamples, uint sampleRate) const
{
	(void)sampleRate;
	if(FadeIn > 0)
	{
		const float k = 1.0f/float(FadeIn*FadeIn);
		for(size_t i = 0; i < inOutSamples.Length(); i++)
			inOutSamples[i] *= float(Math::Sqr(i+1))*k;
	}
	if(FadeOut > 0)
	{
		const size_t a = inOutSamples.Length() > FadeIn? inOutSamples.Length()-FadeOut: FadeIn;
		const float k = 1.0f/float(Math::Sqr(FadeOut));
		for(uint i = 1; i < FadeOut; i++)
			inOutSamples[a+i] *= float(Math::Sqr(FadeOut-i))*k;
	}
}

HallReverb::HallReverb(size_t delayLength, size_t numDelays, float reverbVolume, float k):
	mD(numDelays), mAccum(delayLength*3), mK(k)
{
	if(delayLength == 0 || numDelays == 0) return;
	INTRA_ASSERT(delayLength >= 4 && Math::IsPow2(delayLength));
	INTRA_ASSERT(0 <= k && k <= 1);
	Random::FastUniform<float> rand((delayLength | (numDelays << 16)) + size_t(k*234389432) + 5498439);
	const float revK = 1 - k;
	const float maxDelayVolumeCoeff = reverbVolume*2 / (float(numDelays*numDelays)*(0.5f + k/2));
	for(size_t i = 0; i < numDelays; i++)
	{
		mD[i].Offset = 2 + rand.Seed % (delayLength - 2);
		if(mD[i].Offset > mMaxDelay) mMaxDelay = mD[i].Offset;
		const float volume = (float(numDelays) - float(i)*revK) * rand(maxDelayVolumeCoeff);
		const float pan = rand();
		mD[i].LeftVolume = volume*(1 - pan);
		mD[i].RightVolume = volume*pan;
	}
}


void HallReverb::ProcessSample(float* ioL, float* ioR, float reverbSample)
{
	float* const accumData = mAccum.Data();
	const size_t delayLength = mAccum.Length() / 3, delayLengthMask = delayLength - 1;

	mRF *= 0.7f;
	mRF += reverbSample * 0.3f;
	mS = mS*0.5f + mRF;
	const size_t prevAccumIndex = mAccumIndex;
	const size_t prevAccumIndex3 = prevAccumIndex*3;
	mAccumIndex = (mAccumIndex + 1) & delayLengthMask;
	const float accum = accumData[mAccumIndex*3];
	for(auto& d: mD)
	{
		const size_t index = (prevAccumIndex + d.Offset) & delayLengthMask;
		float* ptr = accumData + index*3;
		*ptr++ += mS*(d.LeftVolume + d.RightVolume); //d.Offset >= 2  ==>  ptr != &accum
		*ptr++ += accum*d.LeftVolume;
		*ptr += accum*d.RightVolume;
	}
	accumData[mAccumIndex*3] *= mK;
	*ioL += accumData[prevAccumIndex3 + 1];
	*ioR += accumData[prevAccumIndex3 + 2];
	accumData[prevAccumIndex3 + 1] = 0;
	accumData[prevAccumIndex3 + 2] = 0;
}

void HallReverb::operator()(Span<float> dstLeft, Span<float> dstRight, CSpan<float> reverbBuffer)
{
	if(operator==(null)) return;
	INTRA_ASSERT(dstLeft.Length() == dstRight.Length() && dstRight.Length() >= reverbBuffer.Length());
	if(!reverbBuffer.Empty())
	{
		while(!reverbBuffer.Empty()) ProcessSample(dstLeft.Begin++, dstRight.Begin++, *reverbBuffer.Begin++);
		mBufferedReverbSamples = mMaxDelay;
	}
	if(dstLeft.Empty()) return;

	//На вход больше не поступает источников реверберации.
	//Наложим оставшееся эхо, после чего реверберацию можно будет отключить до появления следующего источника реверберации
	while(!dstLeft.Empty() && mBufferedReverbSamples > 0)
	{
		ProcessSample(dstLeft.Begin++, dstRight.Begin++, 0);
		mBufferedReverbSamples--;
	}
	while(!dstLeft.Empty() && (Math::Abs(mS) > 0.01f || Math::Abs(mRF) > 0.01f))
		ProcessSample(dstLeft.Begin++, dstRight.Begin++, 0);
}


}
