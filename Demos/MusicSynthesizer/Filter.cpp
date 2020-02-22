#include "Filter.h"

#include "Container/Sequential/Array.h"
#include "Types.h"
#include "Core/Range/Mutation/Transform.h"
#include "Core/Range/Reduction.h"

#include <stdio.h>

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

FilterCoeffs FilterCoeffs::Calculate(float rezAmount, float cutoffRatio, FilterType type)
{
	float c = Tan(float(PI)*cutoffRatio);
	if(type == FilterType::LowPass) c = 1 / c;
	FilterCoeffs result;
	result.C = 1 / (1 + rezAmount*c + c*c);
	result.A1 = -2*result.C;
	result.A2 = result.C;
	result.B1 = 2*(1 - c*c)*result.C;
	result.B2 = (-1 + rezAmount*c - c*c)*result.C;
	if(type == FilterType::LowPass)
	{
		result.A1 = -result.A1;
		result.B1 = -result.B1;
	}
	return result;
}

void Filter::operator()(Span<float> inOutSamples)
{
	//TODO: сделать оптимизированные версии с использованием SIMD
	if(A2 == 0)
	{
		if(A1 == 0)
		{
			if(B2 == 0)
			{
				if(B1 == 0)
				{
					Multiply(inOutSamples, C);
					return;
				}
				for(float& sample: inOutSamples)
				{
					sample *= C;
					sample += B1*PrevSample;

					PrevSample = sample;
				}
				return;
			}

			for(float& sample: inOutSamples)
			{
				sample *= C;
				sample += B1*PrevSample + B2*PrevSample2;

				PrevSample2 = PrevSample;
				PrevSample = sample;
			}
			return;
		}
		if(B2 == 0)
		{
			if(B1 == 0)
			{
				for(float& sample: inOutSamples)
				{
					const float cur = sample;

					sample *= C;
					sample += A1*PrevSrc;

					PrevSrc = cur;
				}
				return;
			}

			for(float& sample: inOutSamples)
			{
				const float cur = sample;

				sample *= C;
				sample += A1*PrevSrc;
				sample += B1*PrevSample;

				PrevSrc = cur;
				PrevSample = sample;
			}
			return;
		}

		for(float& sample: inOutSamples)
		{
			const float cur = sample;

			sample *= C;
			sample += A1*PrevSrc;
			sample += B1*PrevSample + B2*PrevSample2;

			PrevSrc = cur;
			PrevSample2 = PrevSample;
			PrevSample = sample;
		}
		return;
	}

	if(B2 == 0)
	{
		if(B1 == 0)
		{
			for(float& sample: inOutSamples)
			{
				const float cur = sample;

				sample *= C;
				sample += A1*PrevSrc + A2*PrevSrc2;

				PrevSrc2 = PrevSrc;
				PrevSrc = cur;
			}
			return;
		}
		for(float& sample: inOutSamples)
		{
			const float cur = sample;

			sample *= C;
			sample += A1*PrevSrc + A2*PrevSrc2;
			sample += B1*PrevSample;

			PrevSrc2 = PrevSrc;
			PrevSrc = cur;
			PrevSample = sample;
		}
		return;
	}

	for(float& sample: inOutSamples)
	{
		const float cur = sample;

		sample *= C;
		sample += A1*PrevSrc + A2*PrevSrc2;
		sample += B1*PrevSample + B2*PrevSample2;

		PrevSrc2 = PrevSrc;
		PrevSrc = cur;
		PrevSample2 = PrevSample;
		PrevSample = sample;
	}
}

void ResonanceFilter::operator()(Span<float> inOutSamples)
{
	for(float& sample: inOutSamples)
	{
		sample += S*DeltaPhase + PrevSample;
		PrevSample = sample;
		S -= sample*DeltaPhase;
		S *= QFactor;
	}
}

void DynamicResonanceFilter::operator()(Span<float> inOutSamples)
{
	for(float& sample: inOutSamples)
	{
		sample += S*DeltaPhase + PrevSample;
		PrevSample = sample;
		S -= sample*DeltaPhase;
		S *= 1 - 1/InvQFactor;
		InvQFactor += InvQFactorStep;
	}
}

void DriveEffect::operator()(Span<float> inOutSamples)
{
	static const float halfPi = float(PI)*0.5f;
	for(float& sample: inOutSamples)
	{
		const float s = sample*K;
		const float x = s + 0.5f / (1 + s*s) - 0.5f;
		//sample = Atan(x);
		sample = x > 0? halfPi * x / (x + 1): -halfPi * x / (x - 1);
	}
}

void SoftHighPassFilter::operator()(Span<float> inOutSamples)
{
	const float K1 = 1 - K;
	for(float& sample: inOutSamples)
	{
		S *= K;
		S += sample*K1;
		sample -= S;
	}
}

void NormalizeEffect::operator()(Span<float> inOutSamples)
{
	auto minimax = MiniMax(inOutSamples.AsConstRange());
	float absMax = Max(Abs(minimax.first), Abs(minimax.second));
	if(AbsMax < absMax) AbsMax = absMax;
	const float multiplier = 1 / AbsMax;
	Multiply(inOutSamples, multiplier*Volume);
}

INTRA_WARNING_POP
