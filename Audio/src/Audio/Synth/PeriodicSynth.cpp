#include "Audio/Synth/PeriodicSynth.h"
#include "Range/Generators/ArrayRange.h"
#include "Math/Math.h"
#include "Algo/Mutation/Copy.h"
#include "Algo/Mutation/Transform.h"

namespace Intra { namespace Audio { namespace Synth {

uint GetGoodSignalPeriod(double samplesPerPeriod, uint maxPeriods)
{
	if(Math::Fract(samplesPerPeriod)<=0.05) return 1;
	double fractionalCount = 1.0/(Math::Floor(Math::Fract(samplesPerPeriod)*20)/20);
	double minDeltaCnt = 1;
	uint minDeltaN = 0;
	for(uint n=1; fractionalCount*n<maxPeriods || minDeltaCnt>0.1; n++)
	{
		double delta = Math::Fract(fractionalCount*n);
		if(delta>0.5) delta = 1.0-delta;
		delta/=n;
		if(minDeltaCnt>delta)
		{
			minDeltaCnt = delta;
			minDeltaN = n;
		}
	}
	return uint(Math::Round(fractionalCount*minDeltaN));
}

void RepeatFragmentInBuffer(ArrayRange<const float> fragmentSamples, ArrayRange<float> inOutSamples, bool add)
{
	const size_t copyPassCount = inOutSamples.Length()/fragmentSamples.Length();
	if(!add) for(size_t c=0; c<copyPassCount; c++)
		Algo::CopyToAdvance(fragmentSamples, inOutSamples);
	else for(size_t c=0; c<copyPassCount; c++)
	{
		Algo::Add(inOutSamples.TakeExactly(fragmentSamples.Length()), fragmentSamples);
		inOutSamples.PopFirstExactly(fragmentSamples.Length());
	}

	if(!add) Algo::CopyToAdvance(fragmentSamples.Take(inOutSamples.Length()), inOutSamples);
	else Algo::Add(inOutSamples, fragmentSamples.Take(inOutSamples.Length()));
}

}}}
