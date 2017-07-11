#include "Filter.h"
#include "Cpp/Warnings.h"
#include "Container/Sequential/Array.h"
#include "Types.h"
#include "Range/Mutation/Transform.h"

namespace Intra { namespace Audio { namespace Synth {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

FilterCoeffs FilterCoeffs::Calculate(float rezAmount, float cutoffRatio, FilterType type)
{
	float c = Math::Tan(float(Math::PI)*cutoffRatio);
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

INTRA_WARNING_POP

}}}
