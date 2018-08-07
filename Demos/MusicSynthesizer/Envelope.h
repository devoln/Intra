#pragma once

#include <Utils/Debug.h>
#include <Math/Math.h>

struct Envelope
{
	enum: unsigned {N = 8};

	struct Segment
	{
		bool Exponential: 1;

		unsigned SamplesLeft : 31;

		float Volume;

		//! Значение, которое прибавляется к (!Exponential) или умножается на (Exponential) Volume на каждом шаге
		float DU;
	};
	Segment CurrentSegment;

	struct Point
	{
		//! Является ли отрезок, начинающийся в данной точке, экспоненциальным или линейным
		bool Exponential: 1;

		//! Длина отрезка, начинающегося в данной точке
		unsigned Length: 23;

		//! Громкость на конце отрезка, начинающегося в данной точке
		//! Число с фиксированной запятой, считается как Exponential? (Volume + 1) / 256.0f: Volume / 255.0f
		unsigned Volume: 8;

		forceinline float CalcDU(float curVolume) const
		{
			if(Length == 0) return Exponential? 1: 0;
			if(Exponential) return Intra::Math::Pow((Volume + 1) / (curVolume * 256.0f), 1.0f / Length);
			return (Volume / 255.0f - curVolume) / Length;
		}
	};
	Point Points[N - 1];
	int NextSegmentStartPointIndex;

	void StartNextSegment()
	{
		StartSegment(NextSegmentStartPointIndex++);
		while(NextSegmentStartPointIndex < N - 1 && Points[NextSegmentStartPointIndex].Length == 0)
			NextSegmentStartPointIndex++;
	}

	forceinline void StartLastSegment() {StartSegment(N - 2);}

	void StartSegment(int index)
	{
		NextSegmentStartPointIndex = index + 1;
		if(index >= N - 1) return;
		auto pt = Points[index];
		CurrentSegment.Exponential = pt.Exponential;
		CurrentSegment.SamplesLeft = pt.Length;
		CurrentSegment.DU = pt.CalcDU(CurrentSegment.Volume);
	}

	void MultiplyVolume(float volumeMultiplier)
	{
		CurrentSegment.Volume *= volumeMultiplier;
		for(auto& pt: Points) pt.Volume = unsigned(pt.Volume * volumeMultiplier + 0.5f);
	}
};

struct EnvelopeFactory
{
	enum {N = Envelope::N};

	float StartVolume;

	struct Segment
	{
		bool Exponential;
		float EndVolume;
		float Duration;

		forceinline int LengthInSamples(int sampleRate) const
		{
			const float durationSamples = sampleRate * Duration + 0.5f;
			return durationSamples <= float(Intra::int_MAX)? durationSamples: Intra::int_MAX;
		}
	};

	Segment Segments[N];

	static EnvelopeFactory ADSR(float attackTime, float decayTime, float sustainVolume, float releaseTime, bool exponential=false)
	{
		EnvelopeFactory result;
		result.StartVolume = 0;
		result.Segments[0] = {exponential, 1, attackTime};
		result.Segments[1] = {exponential, sustainVolume, decayTime};
		result.Segments[2] = {false, sustainVolume, Intra::Math::Infinity};
		for(int i = 3; i < N - 1; i++) result.Segments[i] = result.Segments[2];
		result.Segments[N - 1] = {exponential, 1, attackTime};
		return result;
	}

	Envelope operator()(int sampleRate) const
	{
		Envelope result;
		result.NextSegmentStartPointIndex = 0;

		result.CurrentSegment.Exponential = Segments[0].Exponential;
		result.CurrentSegment.SamplesLeft = Segments[0].LengthInSamples(sampleRate);
		result.CurrentSegment.Volume = StartVolume;
		result.CurrentSegment.DU = Segments[0].Exponential?
			Intra::Math::Pow(Segments[0].EndVolume / StartVolume, 1.0f / result.CurrentSegment.SamplesLeft):
			(Segments[0].EndVolume - StartVolume) / result.CurrentSegment.SamplesLeft;

		for(int i = 1; i < N; i++)
		{
			auto& pt = result.Points[i - 1];
			auto& s = Segments[i];
			pt.Exponential = s.Exponential;
			pt.Length = s.LengthInSamples(sampleRate),
			pt.Volume = unsigned(s.Exponential? Intra::Math::Max(s.EndVolume * 256.0f - 1.0f, 0.0f): s.EndVolume*255.0f);
		}

		return result;
	}
};
