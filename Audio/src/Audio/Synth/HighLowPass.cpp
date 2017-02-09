#include "Audio/Synth/HighLowPass.h"
#include "Platform/CppWarnings.h"
#include "Range/Generators/ArrayRange.h"
#include "Containers/Array.h"

namespace Intra { namespace Audio { namespace Synth {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

//r = rez amount, from Sqrt(2) to ~0.1
//f = cutoff frequency
//(from ~0 Hz to SampleRate/2 - though many synths seem to filter only up to SampleRate/4)
struct HighLowPassParams
{
	float RezAmount;
	float CutoffFreq;
	bool HighPass;
};

static void HighLowPassFunction(const HighLowPassParams& params,
	ArrayRange<float> inOutSamples, uint sampleRate)
{
	double t = 0.0, dt = 1.0/sampleRate;
	Array<float> in = inOutSamples;
	auto out = inOutSamples;

	float c = Math::Tan(float(Math::PI)*params.CutoffFreq/float(sampleRate));
	float a1, a2, a3, b1, b2;
	if(params.HighPass)
	{
		a1 = 1.0f/(1 + params.RezAmount*c + c*c);
		a2 = -2*a1;
		a3 = a1;
		b1 = 2*(c*c-1)*a1;
		b2 = (1.0f - params.RezAmount*c + c*c)*a1;
	}
	else
	{
		c = 1/c;
		a1 = 1.0f/(1 + params.RezAmount*c + c*c);
		a2 = 2*a1;
		a3 = a1;
		b1 = 2*(1-c*c)*a1;
		b2 = (1.0f - params.RezAmount*c + c*c)*a1;
	}
		
	for(size_t i=2; i<inOutSamples.Length(); i++)
	{
		out[i] *= a1;
		out[i] += a2*in[i-1] + a3*in[i-2];
		out[i] -= b1*out[i-1] + b2*out[i-2];
		t += dt;
	}
}

PostEffectPass CreateLowPass(float rezAmount, float cutoffFreq)
{
	return PostEffectPass(HighLowPassFunction,
		HighLowPassParams{rezAmount, cutoffFreq, false});
}

PostEffectPass CreateHighPass(float rezAmount, float cutoffFreq)
{
	return PostEffectPass(HighLowPassFunction,
		HighLowPassParams{rezAmount, cutoffFreq, true});
}

INTRA_WARNING_POP

}}}
