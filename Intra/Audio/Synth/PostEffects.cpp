#include "Audio/Synth/PostEffects.hh"
#include "Utils/Span.h"
#include "Math/MathRanges.h"
#include "Math/Math.h"
#include "Container/Sequential/Array.h"

namespace Intra { namespace Audio { namespace Synth { namespace PostEffects {

void Chorus::operator()(Span<float> inOutSamples, uint sampleRate) const
{
	Array<float> copy(inOutSamples);
	float radFreq = Frequency*2*float(Math::PI);
	const double duration = double(inOutSamples.Length())/sampleRate;
	float t = 0.0f;
	float dt = 1.0f/float(sampleRate);
	Math::SineRange<float> sineRange(MaxDelay, 0, radFreq*dt);
	size_t samplesProcessed = 0;
	while(!inOutSamples.Empty())
	{
		auto st = sineRange.First();
		sineRange.PopFirst();
		if(t>=-st && t<duration-st)
		{
			size_t index = samplesProcessed+size_t(st*float(sampleRate));
			if(index>=copy.Count()) index = copy.Count()-1;
			inOutSamples.First() *= MainVolume;
			inOutSamples.First() += copy[index]*SecondaryVolume;
		}
		t += dt;
		inOutSamples.PopFirst();
		samplesProcessed++;
	}
}

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

void FilterQ::operator()(Span<float> samples, uint sampleRate) const
{
	(void)sampleRate;
	const float F = Frq/7019.0f;
	float P = 0, S = 0;
	for(float& sample: samples)
	{
		P += S*F+sample;
		S = (S-P*F)*K;
		sample = P;
	}
}

void Fade::operator()(Span<float> inOutSamples, uint sampleRate) const
{
	(void)sampleRate;
	if(FadeIn>0)
	{
		const float k = 1.0f/float(FadeIn*FadeIn);
		for(size_t i=0; i<inOutSamples.Length(); i++)
			inOutSamples[i] *= float(Math::Sqr(i+1))*k;
	}
	if(FadeOut>0)
	{
		const size_t a = inOutSamples.Length()>FadeIn? inOutSamples.Length()-FadeOut: FadeIn;
		const float k = 1.0f/float(Math::Sqr(FadeOut));
		for(uint i=1; i<FadeOut; i++)
			inOutSamples[a+i] *= float(Math::Sqr(FadeOut-i))*k;
	}
}

}}}}
