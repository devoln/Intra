#include <Math/Math.h>
#include <Utils/Finally.h>
#include <Range/Mutation/Fill.h>
#include <Range/Mutation/Transform.h>

#include "ExponentialAttenuation.h"
#include "ADSR.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

AdsrAttenuator::AdsrAttenuator(null_t):
	AttackSamples(0),
	DecaySamples(0),
	SustainVolume(1),
	ReleaseSamples(0),
	U(-1), DU(0)
{}

AdsrAttenuator::AdsrAttenuator(float attackTime, float decayTime,
	float sustainVolume, float releaseTime, uint sampleRate, bool linear):
	AttackSamples(size_t(attackTime*float(sampleRate))),
	DecaySamples(size_t(decayTime*float(sampleRate))),
	SustainVolume(sustainVolume),
	ReleaseSamples(releaseTime == Cpp::Infinity? ~size_t(): size_t(releaseTime*float(sampleRate))),
	Linear(linear)
{
	if(AttackSamples == 0 && DecaySamples == 0 && SustainVolume == 1 && ReleaseSamples == ~size_t())
	{
		U = -1;
		DU = Linear? 0.0f: 1.0f;
		return;
	}
	if(AttackSamples != 0) beginAttack();
	else if(DecaySamples != 0) beginDecay();
	else beginSustain();
}

void AdsrAttenuator::operator()(Span<float> inOutSamples)
{
	auto dst = inOutSamples;
	auto src = inOutSamples.AsConstRange();
	operator()(dst, src, false);
}

void AdsrAttenuator::operator()(Span<float> dstSamples, CSpan<float> srcSamples, float coeff, ExponentAttenuator& exp)
{
	const float oldExp = exp.Factor;
	const auto dstSamplesBefore = dstSamples.Length();
	exp.Factor *= coeff;
	INTRA_FINALLY(
		if(coeff == 0)
		{
			exp.Factor = oldExp;
			exp.SkipSamples(dstSamplesBefore - dstSamples.Length());
		}
		else exp.Factor /= coeff;
	);

	if(AttackSamples != 0)
	{
		attack(dstSamples, srcSamples, exp);
		if(AttackSamples == 0)
		{
			if(DecaySamples != 0) beginDecay();
			else beginSustain();
		}
	}
	if(AttackSamples != 0) return;

	if(DecaySamples != 0)
	{
		decay(dstSamples, srcSamples, exp);
		if(DecaySamples == 0) beginSustain();
	}
	if(DecaySamples != 0) return;
	if(IsNoteReleased()) release(dstSamples, srcSamples, exp);
	else sustain(dstSamples, srcSamples, exp);
}

void AdsrAttenuator::operator()(Span<float> dstSamples, CSpan<float> srcSamples, float coeff)
{
	ExponentAttenuator exp;
	operator()(dstSamples, srcSamples, coeff, exp);
}

void AdsrAttenuator::operator()(Span<float> dstSamples, CSpan<float> srcSamples)
{operator()(dstSamples, srcSamples, 1);}

size_t AdsrAttenuator::CurrentStateSamplesLeft() const
{
	if(AttackSamples != 0) return AttackSamples;
	if(DecaySamples != 0) return DecaySamples;
	if(IsNoteReleased()) return ReleaseSamples;
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
	if(IsNoteReleased())
	{
		const size_t processed = Math::Min(numSamples, ReleaseSamples);
		numSamples -= processed;
		ReleaseSamples -= processed;
	}
}

void AdsrAttenuator::beginAttack()
{
	INTRA_ASSERT(SustainVolume != -1);
	if(Linear)
	{
		U = 0;
		DU = 1.0f / float(AttackSamples);
	}
	else
	{
		U = 0.01f;
		DU = Math::Pow(100.0f, 1.0f/float(AttackSamples));
	}
}

void AdsrAttenuator::beginSustain()
{
	INTRA_ASSERT(SustainVolume != -1);
	U = SustainVolume;
	DU = Linear? 0.0f: 1.0f;
}

void AdsrAttenuator::beginDecay()
{
	INTRA_ASSERT(SustainVolume != -1);
	U = 1;
	if(Linear) DU = (SustainVolume - U) / float(DecaySamples);
	else DU = Math::Pow(SustainVolume, 1.0f / float(DecaySamples));
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
	if(IsInfinite())
	{
		DU = Linear? 0.0f: 1.0f;
		return;
	}
	if(Linear) DU = -U / float(ReleaseSamples);
	else DU = Math::Pow(U*float(ReleaseSamples), -1.0f / float(ReleaseSamples));
}

void AdsrAttenuator::attack(Span<float>& dstSamples, CSpan<float>& srcSamples, ExponentAttenuator& exp)
{
	auto dst = dstSamples.Take(srcSamples.Length()).Take(AttackSamples);
	AttackSamples -= dst.Length();
	dstSamples.PopFirstExactly(dst.Length());
	work(dst, srcSamples, exp);
}

void AdsrAttenuator::decay(Span<float>& dstSamples, CSpan<float>& srcSamples, ExponentAttenuator& exp)
{
	auto dst = dstSamples.Take(srcSamples.Length()).Take(DecaySamples);
	DecaySamples -= dst.Length();
	dstSamples.PopFirstExactly(dst.Length());
	work(dst, srcSamples, exp);
}

void AdsrAttenuator::sustain(Span<float>& dstSamples, CSpan<float>& srcSamples, ExponentAttenuator& exp)
{
	INTRA_ASSERT(!IsNoteReleased());
	const auto dst = dstSamples.Take(srcSamples.Length());
	dstSamples.PopFirstExactly(dst.Length());
	if(U == 0)
	{
		srcSamples.PopFirstExactly(dst.Length());
		exp.SkipSamples(dst.Length());
	}
	else
	{
		exp.Factor *= U;
		exp(dst, srcSamples);
		exp.Factor /= U;
	}
}

void AdsrAttenuator::release(Span<float>& dstSamples, CSpan<float>& srcSamples, ExponentAttenuator& exp)
{
	INTRA_ASSERT(IsNoteReleased());
	auto dst = dstSamples.Take(srcSamples.Length()).Take(ReleaseSamples);
	if(!IsInfinite()) ReleaseSamples -= dst.Length();
	dstSamples.PopFirstExactly(dst.Length());
	work(dst, srcSamples, exp);
}

void AdsrAttenuator::work(Span<float> dst, CSpan<float>& srcSamples, ExponentAttenuator& exp)
{
	if(exp.Factor == 0)
	{
		srcSamples.PopFirstExactly(dst.Length());
		if(Linear) U += DU*float(dst.Length());
		else U *= Math::PowInt(DU, int(dst.Length()));
		return;
	}

	if(Linear) ExponentialLinearAttenuateAdd(dst, srcSamples, exp.Factor, exp.FactorStep, U, DU);
	else
	{
		U *= exp.Factor;
		const float atStep = DU*exp.FactorStep;
		auto len = dst.Length();
		ExponentialAttenuateAdd(dst, srcSamples, U, atStep);
		exp.SkipSamples(len);
		U /= exp.Factor;
	}
}

INTRA_WARNING_POP
