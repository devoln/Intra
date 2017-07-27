#include "Audio/Synth/ExponentialAttenuation.h"

#include "Simd/Simd.h"

#include "Utils/Span.h"

#include "Math/Math.h"

#include "Funal/Bind.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio { namespace Synth {

void ExponentialAttenuate(Span<float>& dst, CSpan<float> src, float& exp, float ek)
{
	float* const dstEnd = dst.Take(src.Length()).End;
#if INTRA_MINEXE > 2

#elif(INTRA_SIMD_SUPPORT == INTRA_SIMD_NONE)
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
	while(dst.Begin + 7 < dstEnd)
	{
		r0 = Simd::Mul(Simd::SetFloat4U(src.Begin), r1);
		r1 = Simd::Mul(r1, ek_8);
		r3 = Simd::Mul(Simd::SetFloat4U(src.Begin + 4), r4);
		Simd::GetU(dst.Begin, r0);
		r4 = Simd::Mul(r4, ek_8);
		Simd::GetU(dst.Begin+4, r3);
		dst.Begin += 8;
		src.Begin += 8;
	}
	exp = Simd::GetX(r1);
#endif
	while(dst.Begin < dstEnd)
	{
		*dst.Begin++ = *src.Begin++ * exp;
		exp *= ek;
	}
}

void ExponentialAttenuateAdd(Span<float>& dst, CSpan<float> src, float& exp, float ek)
{
	const float* const dstEnd = dst.Take(src.Length()).End;

#if INTRA_MINEXE > 2

#elif(INTRA_SIMD_SUPPORT == INTRA_SIMD_NONE)
	float ek8 = ek*ek;
	ek8 *= ek8;
	ek8 *= ek8;
	while(dst.Begin + 7 < dstEnd)
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
	float ek4 = ek*ek;
	ek4 *= ek4;
	Simd::float4 ek_4 = Simd::SetFloat4(ek4);
	Simd::float4 exp_4 = Simd::SetFloat4(exp, exp*ek, exp*ek*ek, exp*ek*ek*ek);

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
		dst.Begin += 8;
		src.Begin += 8;
	}
	exp = Simd::GetX(r1);
#endif
	while(dst.Begin < dstEnd)
	{
		*dst.Begin++ += *src.Begin++ * exp;
		exp *= ek;
	}
}

void ExponentialLinearAttenuate(Span<float>& dst, CSpan<float> src, float& exp, float ek, float& u, float du)
{
	float* const dstEnd = dst.Take(src.Length()).End;
#if INTRA_MINEXE > 2

#elif(INTRA_SIMD_SUPPORT == INTRA_SIMD_NONE)
	float ek8 = ek*ek;
	ek8 *= ek8;
	ek8 *= ek8;
	const float du8 = du*8;
	while(dst.Begin + 7 < dstEnd)
	{
		const float mult = exp*u;
		*dst.Begin++ = *src.Begin++ * mult;
		*dst.Begin++ = *src.Begin++ * mult;
		*dst.Begin++ = *src.Begin++ * mult;
		*dst.Begin++ = *src.Begin++ * mult;
		*dst.Begin++ = *src.Begin++ * mult;
		*dst.Begin++ = *src.Begin++ * mult;
		*dst.Begin++ = *src.Begin++ * mult;
		*dst.Begin++ = *src.Begin++ * mult;
		exp *= ek8;
		u += du8;
	}
#else
	float ek4 = ek*ek;
	ek4 *= ek4;
	const Simd::float4 ek_4 = Simd::SetFloat4(ek4);
	Simd::float4 exp_4 = Simd::SetFloat4(exp, exp*ek, exp*ek*ek, exp*ek*ek*ek);
	const Simd::float4 du_4 = Simd::SetFloat4(du*4);
	Simd::float4 u_4 = Simd::SetFloat4(u, u + du, u + 2*du, u + 3*du);

	Simd::float4 r0, r1, ek_8, r3, r4;
	r1 = exp_4;
	ek_8 = ek_4;
	r4 = Simd::Mul(ek_8, r1);
	ek_8 = Simd::Mul(ek_8, ek_8);
	while(dst.Begin + 7 < dstEnd)
	{
		r0 = Simd::Mul(Simd::SetFloat4U(src.Begin), Simd::Mul(r1, u_4));
		u_4 = Simd::Add(u_4, du_4);
		r1 = Simd::Mul(r1, ek_8);
		r3 = Simd::Mul(Simd::SetFloat4U(src.Begin + 4), Simd::Mul(r4, u_4));
		u_4 = Simd::Add(u_4, du_4);
		Simd::GetU(dst.Begin, r0);
		r4 = Simd::Mul(r4, ek_8);
		Simd::GetU(dst.Begin + 4, r3);
		dst.Begin += 8;
		src.Begin += 8;
	}
	exp = Simd::GetX(r1);
	u = Simd::GetX(u_4);
#endif
	while(dst.Begin < dstEnd)
	{
		*dst.Begin++ = *src.Begin++ * exp * u;
		exp *= ek;
		u += du;
	}
}

void ExponentialLinearAttenuateAdd(Span<float>& dst, CSpan<float> src, float& exp, float ek, float& u, float du)
{
	const float* const dstEnd = dst.Take(src.Length()).End;

#if INTRA_MINEXE > 2

#elif(INTRA_SIMD_SUPPORT == INTRA_SIMD_NONE)
	const float du8 = du*8;
	float ek8 = ek*ek;
	ek8 *= ek8;
	ek8 *= ek8;
	while(dst.Begin + 7 < dstEnd)
	{
		const float mult = u*exp;
		*dst.Begin++ += *src.Begin++ * mult;
		*dst.Begin++ += *src.Begin++ * mult;
		*dst.Begin++ += *src.Begin++ * mult;
		*dst.Begin++ += *src.Begin++ * mult;
		*dst.Begin++ += *src.Begin++ * mult;
		*dst.Begin++ += *src.Begin++ * mult;
		*dst.Begin++ += *src.Begin++ * mult;
		*dst.Begin++ += *src.Begin++ * mult;
		exp *= ek8;
		u += du8;
	}
#else
	float ek4 = ek*ek;
	ek4 *= ek4;
	const Simd::float4 ek_4 = Simd::SetFloat4(ek4);
	Simd::float4 exp_4 = Simd::SetFloat4(exp, exp*ek, exp*ek*ek, exp*ek*ek*ek);
	const Simd::float4 du_4 = Simd::SetFloat4(du*4);
	Simd::float4 u_4 = Simd::SetFloat4(u, u + du, u + 2*du, u + 3*du);

	Simd::float4 r0, r1, ek_8, r3, r4;
	r1 = exp_4;
	ek_8 = ek_4;
	r4 = Simd::Mul(ek_8, r1);
	ek_8 = Simd::Mul(ek_8, ek_8);
	while(dst.Begin + 7 < dstEnd)
	{
		r0 = Simd::Mul(Simd::SetFloat4U(src.Begin), Simd::Mul(r1, u_4));
		u_4 = Simd::Add(u_4, du_4);
		r1 = Simd::Mul(r1, ek_8);
		r3 = Simd::Mul(Simd::SetFloat4U(src.Begin + 4), Simd::Mul(r4, u_4));
		u_4 = Simd::Add(u_4, du_4);
		Simd::GetU(dst.Begin, Simd::Add(Simd::SetFloat4U(dst.Begin), r0));
		r4 = Simd::Mul(r4, ek_8);
		Simd::GetU(dst.Begin + 4, Simd::Add(Simd::SetFloat4U(dst.Begin + 4), r3));
		dst.Begin += 8;
		src.Begin += 8;
	}
	exp = Simd::GetX(r1);
	u = Simd::GetX(u_4);
#endif
	while(dst.Begin < dstEnd)
	{
		*dst.Begin++ += *src.Begin++ * exp * u;
		exp *= ek;
		u += du;
	}
}

#if INTRA_DISABLED
static void exponent_attenuation_inplace(float*& ptr, float* end, float& exp, float ek)
{
#if INTRA_MINEXE > 2

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
		ptr += 4;
	}
	exp = Simd::GetX(exp_4);
#elif INTRA_PLATFORM_ARCH == INTRA_PLATFORM_ARM

#endif
	while(ptr<end)
	{
		*ptr++ *= exp;
		exp *= ek;
	}
}
#endif

void ExponentAttenuator::operator()(Span<float> inOutSamples)
{
	ExponentialAttenuate(inOutSamples, inOutSamples, mFactor, mFactorStep);
}

}}}

INTRA_WARNING_POP
