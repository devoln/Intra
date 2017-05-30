#include "Audio/Synth/ExponentialAttenuation.h"
#include "Simd/Simd.h"
#include "Utils/Span.h"
#include "Math/Math.h"

#define OPTIMIZE

namespace Intra { namespace Audio { namespace Synth {

void ExponentialAttenuate(Span<float>& dst, CSpan<float> src, float& exp, float ek)
{
	float* const dstEnd = dst.Take(src.Length()).End;
#ifndef OPTIMIZE

#elif(INTRA_SIMD_SUPPORT==INTRA_SIMD_NONE)
	float ek8 = ek*ek;
	ek8 *= ek8;
	ek8 *= ek8;
	while(dst.Begin<dstEnd-7)
	{
		*dst.Begin++ = *src.Begin++ * exp;
		*dst.Begin++ = *src.Begin++ * exp;
		*dst.Begin++ = *src.Begin++ * exp;
		*dst.Begin++ = *src.Begin++ * exp;
		*dst.Begin++ = *src.Begin++ * exp;
		*dst.Begin++ = *src.Begin++ * exp;
		*dst.Begin++ = *src.Begin++ * exp;
		*dst.Begin++ = *src.Begin++ * exp;
		exp *= ek8;
	}
#else
	float ek4 = ek*ek;
	ek4 *= ek4;
	Simd::float4 ek_4 = Simd::SetFloat4(ek4);
	//while(((size_t)dst)&15 && dst<dstend) *dst++ = *src++ * Exp, Exp*=ek;
	Simd::float4 exp_4 = Simd::SetFloat4(exp, exp*ek, exp*ek*ek, exp*ek*ek*ek);
	/*while(dst<dstend-3)
	{
		Simd::GetU(dst, Simd::Mul(Simd::SetFloat4U(src), exp_4));
		exp_4 = Simd::Mul(exp_4, ek_4);
		dst+=4; src+=4;
	}
	Exp = Simd::GetX(exp_4);*/

	Simd::float4 r0, r1, ek_8, r3, r4;
	r1 = exp_4;
	ek_8 = ek_4;
	r4 = Simd::Mul(ek_8, r1);
	ek_8 = Simd::Mul(ek_8, ek_8);
	while(dst.Begin<dstEnd-7)
	{
		r0 = Simd::Mul(Simd::SetFloat4U(src.Begin), r1);
		r1 = Simd::Mul(r1, ek_8);
		r3 = Simd::Mul(Simd::SetFloat4U(src.Begin+4), r4);
		Simd::GetU(dst.Begin, r0);
		r4 = Simd::Mul(r4, ek_8);
		Simd::GetU(dst.Begin+4, r3);
		dst.Begin+=8;
		src.Begin+=8;
	}
	exp = Simd::GetX(r1);
#endif
	while(dst.Begin<dstEnd) *dst.Begin++ = *src.Begin++ * exp, exp*=ek;
}

void ExponentialAttenuateAdd(Span<float>& dst, CSpan<float> src, float& exp, float ek)
{
	const float* const dstEnd = dst.Take(src.Length()).End;

#ifndef OPTIMIZE

#elif(INTRA_SIMD_SUPPORT==INTRA_SIMD_NONE)
	float ek8 = ek*ek;
	ek8 *= ek8;
	ek8 *= ek8;
	while(dst.Begin<dstEnd-7)
	{
		*dst.Begin++ += *src.Begin++ * exp;
		*dst.Begin++ += *src.Begin++ * exp;
		*dst.Begin++ += *src.Begin++ * exp;
		*dst.Begin++ += *src.Begin++ * exp;
		*dst.Begin++ += *src.Begin++ * exp;
		*dst.Begin++ += *src.Begin++ * exp;
		*dst.Begin++ += *src.Begin++ * exp;
		*dst.Begin++ += *src.Begin++ * exp;
		exp *= ek8;
	}
#else
	//while(((size_t)dst)&15 && dst<dstend) *dst++ += *src++ * Exp, Exp*=ek;
	float ek4 = ek*ek;
	ek4 *= ek4;
	Simd::float4 ek_4 = Simd::SetFloat4(ek4);
	Simd::float4 exp_4 = Simd::SetFloat4(exp, exp*ek, exp*ek*ek, exp*ek*ek*ek);
	/*while(dst.Begin<dstEnd-7)
	{
		Simd::GetU(dst, Simd::Add(Simd::SetU(dst.Begin), Simd::Mul(Simd::SetU(src), exp_4)));
		exp_4 = Simd::Mul(exp_4, ek_4);
		dst+=4; src+=4;
	}
	Exp = Simd::GetX(exp_4);*/

	Simd::float4 r0, r1, ek_8, r3, r4;
	r1 = exp_4;
	ek_8 = ek_4;
	r4 = Simd::Mul(ek_8, r1);
	ek_8 = Simd::Mul(ek_8, ek_8);
	while(dst.Begin<dstEnd-7)
	{
		r0 = Simd::Mul(Simd::SetFloat4U(src.Begin), r1);
		r1 = Simd::Mul(r1, ek_8);
		r3 = Simd::Mul(Simd::SetFloat4U(src.Begin+4), r4);
		Simd::GetU(dst.Begin, Simd::Add(Simd::SetFloat4U(dst.Begin), r0));
		r4 = Simd::Mul(r4, ek_8);
		Simd::GetU(dst.Begin+4, Simd::Add(Simd::SetFloat4U(dst.Begin+4), r3));
		dst.Begin+=8; src.Begin+=8;
	}
	exp = Simd::GetX(r1);
#endif
	while(dst.Begin<dstEnd) *dst.Begin++ += *src.Begin++ * exp, exp*=ek;
}

#if INTRA_DISABLED
static void exponent_attenuation_inplace(float*& ptr, float* end, float& exp, float ek)
{
#ifndef OPTIMIZE

#elif !defined(INTRA_USE_PDO)
	float ek8 = ek*ek;
	ek8 *= ek8;
	ek8 *= ek8;
	while(ptr<end-7)
	{
		*ptr++ *= exp;
		*ptr++ *= exp;
		*ptr++ *= exp;
		*ptr++ *= exp;
		*ptr++ *= exp;
		*ptr++ *= exp;
		*ptr++ *= exp;
		*ptr++ *= exp;
		exp *= ek8;
	}
#elif INTRA_PLATFORM_ARCH==INTRA_PLATFORM_X86 || INTRA_PLATFORM_ARCH==INTRA_PLATFORM_X86_64
	float ek4 = ek*ek;
	ek4 *= ek4;
	float4 ek_4 = Simd::Set(ek4);
	float4 exp_4 = Simd::Set(1, ek, ek*ek, ek*ek*ek);
	//_mm_prefetch((const char*)ptr, _MM_HINT_NTA);
	while(ptr<end-3)
	{
		Simd::GetU(ptr, Simd::Mul(Simd::SetU(ptr), exp_4));
		exp_4 = Simd::Mul(exp_4, ek_4);
		ptr+=4;
	}
	exp = Simd::GetX(exp_4);
#elif INTRA_PLATFORM_ARCH==INTRA_PLATFORM_ARM

#endif
	while(ptr<end) *ptr++*=exp, exp*=ek;
}
#endif

void ExponentialAttenuationPassFunction(const float& coeff,
	float noteDuration, Span<float> inOutSamples, uint sampleRate)
{
	(void)noteDuration;
	const float ek = Math::Exp(-coeff/float(sampleRate));
	float exp = 1.0f;
	ExponentialAttenuate(inOutSamples, inOutSamples, exp, ek);
}

AttenuationPass CreateExponentialAttenuationPass(float coeff)
{return AttenuationPass(ExponentialAttenuationPassFunction, coeff);}

}}}
