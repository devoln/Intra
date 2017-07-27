#include "TubeSampler.h"
#include "Random/FastUniform.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio { namespace Synth {


TubeSampler::TubeSampler(float dphi, float volume, float detuneVal, float detuneFactor, float kRand):
	mPhaseDelta(dphi), mDetuneFactor(detuneFactor), mRandCoeff(kRand*detuneFactor),
	mFF(mPhaseDelta * detuneVal), mA(0), mP(0), mS(0.01f*volume), mSeed(157898685) {}

Span<float> TubeSampler::operator()(Span<float> dst, bool add)
{
	const float detuneFactor = mPhaseDelta*mDetuneFactor;
	const float invDetuneFactor = 1.0f - mDetuneFactor;
	const float randCoeff = mPhaseDelta*mRandCoeff;
	if(!add) while(!dst.Empty())
	{
		mP += mS*mFF;
		*dst.Begin++ = mP;
		mS -= mP*mFF;
		const float rand0 = float(mSeed *= 16807) * 2.32830645e-10f;
		mS *= (mA > rand0 + 0.5f)? 0.99f: 1.01f;
		mA = mA*0.99f + (mP*mP + mS*mS);
		mFF = mFF*invDetuneFactor + detuneFactor + rand0*randCoeff;
	}
	else while(!dst.Empty())
	{
		mP += mS*mFF;
		*dst.Begin++ += mP;
		mS -= mP*mFF;
		const float rand0 = float(mSeed *= 16807) * 2.32830645e-10f;
		mS *= (mA > rand0 + 0.5f)? 0.99f: 1.01f;
		mA = mA*0.99f + (mP*mP + mS*mS);
		mFF = mFF*invDetuneFactor + detuneFactor + rand0*randCoeff;
	}
	return dst;
}

}}}

INTRA_WARNING_POP
