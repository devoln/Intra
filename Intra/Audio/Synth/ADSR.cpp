#include "ADSR.h"
#include "Math/Math.h"
#include "Simd/Simd.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio { namespace Synth {

AdsrAttenuator::AdsrAttenuator(float attackTime, float decayTime,
	float sustainVolume, float releaseTime, uint sampleRate, size_t sampleCount):
	mAttackSamples(size_t(attackTime*sampleRate)),
	mDecaySamples(size_t(decayTime*sampleRate)),
	mSustainVolume(sustainVolume),
	mSustainSamples(sampleCount - Math::Min(mAttackSamples + mDecaySamples + mReleaseSamples, sampleCount)),
	mReleaseSamples(size_t(releaseTime*sampleRate))
{
	if(mAttackSamples != 0) beginAttack();
	else beginDecay();
}

void AdsrAttenuator::operator()(Span<float> inOutSamples)
{
	if(mAttackSamples != 0)
	{
		attack(inOutSamples);
		if(mAttackSamples == 0) beginDecay();
	}
	if(mAttackSamples != 0) return;

	if(mDecaySamples != 0) decay(inOutSamples);
	if(mDecaySamples != 0) return;
	if(mSustainSamples != 0) mSustainSamples -= inOutSamples.PopFirstN(mSustainSamples);
	if(mSustainSamples != 0) return;
	if(mSustainVolume != -1)
	{
		beginRelease();
		mSustainVolume = -1; //Знак того, что уже началась стадия Release
	}
	release(inOutSamples);
}

void AdsrAttenuator::beginAttack()
{
	mU = 0;
	mDU = 1.0f / mAttackSamples;
}

void AdsrAttenuator::beginDecay()
{
	mU = 1;
	mDU = (mSustainVolume - mU) / mDecaySamples;
}

void AdsrAttenuator::NoteOff()
{
	mAttackSamples = 0;
	mDecaySamples = 0;
	mSustainSamples = 0;
}

void AdsrAttenuator::beginRelease()
{
	mDU = -mU / mReleaseSamples;
}

static void LinearMultiply(float*& ptr, float* end, float& u, float du)
{
#if INTRA_MINEXE >= 3
#elif(INTRA_SIMD_SUPPORT == INTRA_SIMD_NONE)
	const float du4 = 4*du;
	while(ptr + 3 < end)
	{
		*ptr++ *= u;
		*ptr++ *= u;
		*ptr++ *= u;
		*ptr++ *= u;
		u += du4;
	}
#else
	Simd::float4 u4 = Simd::SetFloat4(0, du, 2*du, 3*du);
	Simd::float4 du4 = Simd::SetFloat4(4*du);
	while(ptr + 3 < end)
	{
		Simd::float4 v = Simd::SetFloat4U(ptr);
		Simd::GetU(ptr, Simd::Mul(v, u4));
		u4 = Simd::Add(u4, du4);
		ptr += 4;
	}
	u = Simd::GetX(u4);
#endif
	while(ptr < end)
	{
		*ptr++ *= u;
		u += du;
	}
}

void AdsrAttenuator::attack(Span<float>& inOutSamples)
{
	auto end = inOutSamples.Take(mAttackSamples).End;
	mAttackSamples -= size_t(end - inOutSamples.Begin);
	LinearMultiply(inOutSamples.Begin, end, mU, mDU);
}

void AdsrAttenuator::decay(Span<float>& inOutSamples)
{
	auto end = inOutSamples.Take(mDecaySamples).End;
	mDecaySamples -= size_t(end - inOutSamples.Begin);
	LinearMultiply(inOutSamples.Begin, end, mU, mDU);
}

void AdsrAttenuator::release(Span<float>& inOutSamples)
{
	auto end = inOutSamples.Take(mReleaseSamples).End;
	mReleaseSamples -= size_t(end - inOutSamples.Begin);
	LinearMultiply(inOutSamples.Begin, end, mU, mDU);
}

}}}

INTRA_WARNING_POP
