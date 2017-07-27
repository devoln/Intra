#include "ADSR.h"
#include "Math/Math.h"

#include "Simd/Simd.h"

#include "Range/Mutation/Transform.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio { namespace Synth {

AdsrAttenuator::AdsrAttenuator(null_t):
	AttackSamples(0),
	DecaySamples(0),
	SustainVolume(1),
	ReleaseSamples(0),
	U(-1), DU(0)
{}

AdsrAttenuator::AdsrAttenuator(float attackTime, float decayTime,
	float sustainVolume, float releaseTime, uint sampleRate):
	AttackSamples(size_t(attackTime*float(sampleRate))),
	DecaySamples(size_t(decayTime*float(sampleRate))),
	SustainVolume(sustainVolume),
	ReleaseSamples(releaseTime == Cpp::Infinity? ~size_t(): size_t(releaseTime*float(sampleRate)))
{
	if(AttackSamples == 0 && DecaySamples == 0 && SustainVolume == 1 && ReleaseSamples == ~size_t())
	{
		U = -1;
		DU = 0;
		return;
	}
	if(AttackSamples != 0) beginAttack();
	else if(DecaySamples != 0) beginDecay();
	else beginSustain();
}

void AdsrAttenuator::operator()(Span<float> inOutSamples)
{
	if(AttackSamples != 0)
	{
		attack(inOutSamples);
		if(AttackSamples == 0)
		{
			if(DecaySamples != 0) beginDecay();
			else beginSustain();
		}
	}
	if(AttackSamples != 0) return;

	if(DecaySamples != 0)
	{
		decay(inOutSamples);
		if(DecaySamples == 0) beginSustain();
	}
	if(DecaySamples != 0) return;
	if(SustainVolume == -1) release(inOutSamples);
	else Multiply(inOutSamples, U);
}

size_t AdsrAttenuator::CurrentStateSamplesLeft() const
{
	if(AttackSamples != 0) return AttackSamples;
	if(DecaySamples != 0) return DecaySamples;
	if(SustainVolume == -1) return ReleaseSamples;
	return ~size_t();
}

void AdsrAttenuator::SamplesProcessedExternally(size_t numSamples)
{
	if(AttackSamples != 0)
	{
		const size_t processed = Math::Min(numSamples, AttackSamples);
		numSamples -= processed;
		AttackSamples -= processed;
		if(AttackSamples == 0)
		{
			if(DecaySamples != 0) beginDecay();
			else beginSustain();
		}
	}
	if(numSamples == 0) return;

	if(DecaySamples != 0)
	{
		const size_t processed = Math::Min(numSamples, DecaySamples);
		numSamples -= processed;
		DecaySamples -= processed;
		if(DecaySamples == 0) beginSustain();
	}
	if(numSamples == 0) return;
	if(SustainVolume == -1)
	{
		const size_t processed = Math::Min(numSamples, ReleaseSamples);
		numSamples -= processed;
		ReleaseSamples -= processed;
	}
}

void AdsrAttenuator::beginAttack()
{
	INTRA_ASSERT(SustainVolume != -1);
	U = 0;
	DU = 1.0f / float(AttackSamples);
}

void AdsrAttenuator::beginSustain()
{
	INTRA_ASSERT(SustainVolume != -1);
	U = SustainVolume;
	DU = 0;
}

void AdsrAttenuator::beginDecay()
{
	INTRA_ASSERT(SustainVolume != -1);
	U = 1;
	DU = (SustainVolume - U) / float(DecaySamples);
}

void AdsrAttenuator::NoteRelease()
{
	if(!*this) return;
	AttackSamples = 0;
	DecaySamples = 0;
	SustainVolume = -1;
	beginRelease();
}

void AdsrAttenuator::beginRelease()
{
	DU = -U / float(ReleaseSamples);
}

void AdsrAttenuator::attack(Span<float>& inOutSamples)
{
	auto dst = inOutSamples.Take(AttackSamples);
	AttackSamples -= dst.Length();
	LinearMultiply(dst, U, DU);
	inOutSamples.PopFirstExactly(dst.Length());
}

void AdsrAttenuator::decay(Span<float>& inOutSamples)
{
	auto dst = inOutSamples.Take(DecaySamples);
	DecaySamples -= dst.Length();
	LinearMultiply(dst, U, DU);
	inOutSamples.PopFirstExactly(dst.Length());
}

void AdsrAttenuator::release(Span<float>& inOutSamples)
{
	if(ReleaseSamples == ~size_t()) return;
	auto dst = inOutSamples.Take(ReleaseSamples);
	ReleaseSamples -= dst.Length();
	LinearMultiply(dst, U, DU);
	inOutSamples.PopFirstExactly(dst.Length());
}

}}}

INTRA_WARNING_POP
