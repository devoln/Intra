#include "Audio/Synth/HighLowPass.h"
#include "Cpp/Warnings.h"
#include "Utils/Span.h"
#include "Container/Sequential/Array.h"
#include "Funal/Bind.h"

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
	Span<float> inOutSamples, uint sampleRate)
{
	float c = Math::Tan(float(Math::PI)*params.CutoffFreq/float(sampleRate));
	if(!params.HighPass) c = 1/c;
	const float a1 = 1.0f/(1 + params.RezAmount*c + c*c);
	float a2 = -2*a1;
	const float a3 = a1;
	float  b1 = 2*(c*c-1)*a1;
	const float b2 = (1.0f - params.RezAmount*c + c*c)*a1;
	if(!params.HighPass) a2 = -a2, b1 = -b1;

	Array<float> inCopy = inOutSamples;
	const double dt = 1.0/sampleRate;
	double t = 0.0;
	for(size_t i=2; i<inOutSamples.Length(); i++)
	{
		inOutSamples[i] *= a1;
		inOutSamples[i] += a2*inCopy[i-1] + a3*inCopy[i-2];
		inOutSamples[i] -= b1*inOutSamples[i-1] + b2*inOutSamples[i-2];
		t += dt;
	}
}

PostEffectPass CreateLowPass(float rezAmount, float cutoffFreq)
{
	return Funal::Bind(HighLowPassFunction,
		HighLowPassParams{rezAmount, cutoffFreq, false});
}

PostEffectPass CreateHighPass(float rezAmount, float cutoffFreq)
{
	return Funal::Bind(HighLowPassFunction,
		HighLowPassParams{rezAmount, cutoffFreq, true});
}

INTRA_WARNING_POP

}}}
