#pragma once

#include <Core/Debug.h>
#include <Math/Math.h>

struct Envelope
{
	enum: unsigned
	{
		N = 5
	};

	enum: int
	{
		NoNextSegmentIndicator = N - 1
	};

	struct Segment
	{
		unsigned Exponential: 1;

		unsigned SamplesLeft : 31;

		float Volume;

		/// Значение, которое прибавляется к (!Exponential) или умножается на (Exponential) Volume на каждом шаге
		float DU;

		INTRA_FORCEINLINE bool IsConstant() const {return float(Exponential) == DU;}
		INTRA_FORCEINLINE bool IsNoOp() const {return IsConstant() && Volume == 1;}
		INTRA_FORCEINLINE void Advance(size_t samples)
		{
			if(Exponential) Volume *= PowInt(DU, int(samples));
			else Volume += DU*float(samples);
			SamplesLeft -= unsigned(samples);
		}
	};
	Segment CurrentSegment;

	struct Point
	{
		/// Является ли отрезок, начинающийся в данной точке, экспоненциальным или линейным
		unsigned Exponential: 1;

		/// Длина отрезка, начинающегося в данной точке
		unsigned Length: 23;

		/// Громкость на конце отрезка, начинающегося в данной точке
		/// Число с фиксированной запятой, считается как Exponential? (Volume + 1) / 256.0f: Volume / 255.0f
		unsigned Volume: 8;

		INTRA_FORCEINLINE float CalcDU(float curVolume) const
		{
			if(Length == 0) return Exponential? 1.0f: 0.0f;
			if(Exponential) return Intra::Pow((Volume + 1) / (curVolume * 256.0f), 1.0f / Length);
			return (Volume / 255.0f - curVolume) / Length;
		}

		/// Упаковать значение громкости.
		/// Вызывать эту функцию только после установки значения Exponential!
		INTRA_FORCEINLINE void SetVolume(float volume)
		{
			Volume = unsigned(Exponential?
				Intra::Max(volume * 256.0f - 1.0f, 0.0f):
				volume*255.0f);
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

	INTRA_FORCEINLINE void StartLastSegment() {StartSegment(N - 2);}

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
		for(int i = NextSegmentStartPointIndex; i < N - 1; i++)
		{
			auto& pt = Points[i];
			pt.Volume = unsigned(pt.Volume * volumeMultiplier + 0.5f);
		}
	}

	static Envelope Constant(float volume = 1)
	{
		Envelope result;
		result.CurrentSegment = {false, 0x7FFFFFFF, volume, 0};
		result.NextSegmentStartPointIndex = N - 1;
		return result;
	}
};

struct LinearAttenuator
{
	float Factor, FactorStep;
	LinearAttenuator(decltype(null)=null): Factor(1), FactorStep(0) {}
	LinearAttenuator(float startVolume, float deltaPerSample): Factor(1), FactorStep(deltaPerSample) {}

	INTRA_FORCEINLINE void SkipSamples(size_t count) {Factor += FactorStep*count;}
};

struct EnvelopeSegment
{
	ExponentAttenuator Exp;
	LinearAttenuator Linear;

	INTRA_FORCEINLINE EnvelopeSegment(decltype(null)=null) {}

	INTRA_FORCEINLINE EnvelopeSegment(Envelope::Segment segment)
	{
		if(segment.Exponential)
		{
			Exp.Factor = segment.Volume;
			Exp.FactorStep = segment.DU;
			return;
		}
		Linear.Factor = segment.Volume;
		Linear.FactorStep = segment.DU;
	}

	INTRA_FORCEINLINE void operator()(Span<float> inOutSamples)
	{
		auto src = inOutSamples.AsConstRange();
		ExponentialLinearAttenuate(inOutSamples, src, Exp.Factor, Exp.FactorStep, Linear.Factor, Linear.FactorStep);
	}

	INTRA_FORCEINLINE void operator()(Span<float> dstSamples, CSpan<float> srcSamples)
	{ExponentialLinearAttenuateAdd(dstSamples, srcSamples, Exp.Factor, Exp.FactorStep, Linear.Factor, Linear.FactorStep);}

	INTRA_FORCEINLINE void SkipSamples(size_t count)
	{
		Exp.SkipSamples(count);
		Linear.SkipSamples(count);
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

		INTRA_FORCEINLINE unsigned LengthInSamples(int sampleRate) const
		{
			const float durationSamples = sampleRate * Duration + 0.5f;
			return durationSamples <= float(Intra::uint_MAX)? unsigned(durationSamples): Intra::uint_MAX;
		}
	};

	Segment Segments[N];

	static EnvelopeFactory Constant(float volume)
	{
		EnvelopeFactory result;
		for(int i = 0; i < N; i++) result.Segments[i] = {false, volume, 0};
		result.Segments[N - 1].Duration = Intra::Infinity;
		return result;
	}

	static EnvelopeFactory ADSR(float attackTime, float decayTime, float sustainVolume, float releaseTime, bool exponential=false)
	{
		EnvelopeFactory result;
		result.StartVolume = 0;
		for(int i = 0; i < N - 4; i++) result.Segments[i] = {false, 0, 0};
		result.Segments[N - 4] = {exponential, 1, attackTime};
		result.Segments[N - 3] = {exponential, sustainVolume, decayTime};
		result.Segments[N - 2] = {false, sustainVolume, Intra::Infinity};
		result.Segments[N - 1] = {exponential, 1, releaseTime};
		return result;
	}

	Envelope operator()(int sampleRate) const
	{
		int startIndex = 0;
		while(Segments[startIndex].Duration == 0) startIndex++;
		auto& startSeg = Segments[startIndex];

		Envelope result;
		result.NextSegmentStartPointIndex = startIndex;
		auto& resSeg = result.CurrentSegment;
		resSeg.Exponential = startSeg.Exponential;
		resSeg.SamplesLeft = startSeg.LengthInSamples(sampleRate);
		resSeg.Volume = StartVolume;
		resSeg.DU = startSeg.Exponential?
			Intra::Pow(startSeg.EndVolume / StartVolume, 1.0f / resSeg.SamplesLeft):
			(startSeg.EndVolume - StartVolume) / resSeg.SamplesLeft;

		for(int i = startIndex; i < N - 1; i++)
		{
			auto& pt = result.Points[i];
			auto& s = Segments[i + 1];
			pt.Exponential = unsigned(s.Exponential);
			pt.Length = s.LengthInSamples(sampleRate);
			pt.SetVolume(s.EndVolume);
		}

		return result;
	}
};
