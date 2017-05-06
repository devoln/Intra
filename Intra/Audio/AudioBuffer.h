#pragma once

#include "Cpp/Warnings.h"
#include "Utils/Span.h"
#include "Math/Vector.h"
#include "Container/Sequential/Array.h"
#include "Audio/SoundTypes.h"

namespace Intra { namespace Audio {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

struct AudioBuffer
{
	AudioBuffer(null_t=null): SampleRate(0), Samples(null) {}
	AudioBuffer(size_t sampleCount, uint sampleRate=44100, CSpan<float> initData=null);

	double Duration() const {return SampleRate==0? 0: double(Samples.Count())/double(SampleRate);}
	uint TimeToSamples(double time) const {return uint(time*SampleRate);}
	double SamplesToTime(size_t samples) const {return double(samples)/double(SampleRate);}

	void ConvertToShorts(size_t first, Span<short> outSamples) const;
	void CastToShorts(size_t first, Span<short> outSamples) const;
	void ShiftSamples(intptr samplesToShift);
	void Clear(size_t startSample=0, size_t sampleCount=Meta::NumericLimits<size_t>::Max());

	template<typename T> void Fill(T callable, size_t startSample, size_t sampleCount, double t0=0.0)
	{
		const size_t endSample = startSample+sampleCount;
		if(endSample>Samples.Count()) Samples.SetCount(endSample);
		double t = t0, dt = 1.0/SampleRate;
		for(size_t i = startSample; i<endSample; i++, t+=dt)
			Samples[i] = callable(float(t));
	}

	template<typename T> void Operate(T callable, const AudioBuffer* rhs,
		size_t startLhsSample, size_t startRhsSample, size_t sampleCount, double t0=0.0)
	{
		const size_t endLhsSample = startLhsSample+sampleCount;
		const intptr offset = intptr(startRhsSample-startLhsSample);
		double t = t0, dt = 1.0/SampleRate;
		for(size_t i = startLhsSample; i<endLhsSample; i++, t+=dt)
			Samples[i] = callable(float(t), Samples[i], rhs->Samples[size_t(intptr(i)+offset)]);
	}

	template<typename T> void Modify(T callable, size_t startSample, size_t sampleCount, double t0=0.0)
	{
		const size_t endSample = startSample+sampleCount;
		double t = t0, dt = 1.0/SampleRate;
		for(size_t i = startSample; i<endSample; i++)
			Samples[i] = callable(float(t), Samples[i]), t+=dt;
	}


	void ModifyByFunction(float(*f)(float t, float sample, const void* params), const void* parameters,
		size_t startSample, size_t sampleCount, double t0=0.0)
	{
		Modify([=](float t, float sample){return f(t, sample, parameters);}, startSample, sampleCount, t0);
	}

	void OperateByFunction(float(*f)(float t, float sample1, float sample2, const void* params), const void* parameters,
		const AudioBuffer* rhs, size_t startLhsSample, size_t startRhsSample, size_t sampleCount, double t0=0.0)
	{
		Operate([f,parameters](float t, float sample1, float sample2)
			{return f(t, sample1, sample2, parameters);},
			rhs, startLhsSample, startRhsSample, sampleCount, t0);
	}

	void CopyFrom(size_t startSample, size_t sampleCount, AudioBuffer* src, size_t srcStartSample);
	void MixWith(const AudioBuffer& rhs, size_t lhsStartSample, size_t rhsStartSample, size_t sampleCount);

	void FillWithFunction(float(*f)(float t, const void* params), const void* parameters, size_t startSample, size_t sampleCount)
	{
		Fill([=](float t){return f(t, parameters);}, startSample, sampleCount);
	}

	void Pulse(uint frequency, float phase, size_t startSample, size_t sampleCount);

	float GetSample(size_t i) const {return i<Samples.Count()? Samples[i]: 0;}

	void SetSample(uint i, float sample)
	{
		if(i>=Samples.Count())
			Samples.SetCount(i+1);
		Samples[i] = sample;
	}

	Meta::Pair<float, float> GetMinMax(size_t startSample=0, size_t sampleCount = Meta::NumericLimits<size_t>::Max()) const;
	void SetMinMax(float newMin, float newMax, size_t startSample=0,
		size_t samplesCount = Meta::NumericLimits<size_t>::Max(), Meta::Pair<float, float> oldMinMax={0,0});

	bool operator==(null_t) const {return SampleRate==0;}
	bool operator!=(null_t) const {return !operator==(null);}

	AudioBuffer& operator=(null_t) {SampleRate=0; Samples=null; return *this;}

	SoundInfo Info() const {return {Samples.Count(), SampleRate, 1, Data::ValueType::Float};}

	uint SampleRate;
	Array<float> Samples;
};

INTRA_WARNING_POP

}}
