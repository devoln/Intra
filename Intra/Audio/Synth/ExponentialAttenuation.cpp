#include "Audio/Synth/ExponentialAttenuation.h"

#include "Simd/Simd.h"

#include "Utils/Span.h"

#include "Math/Math.h"

#include "Funal/Bind.h"

#include "Range/Mutation/Transform.h"
#include "Range/Mutation/Fill.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio { namespace Synth {

void ExponentialAttenuate(Span<float>& dst, CSpan<float>& src, float& exp, float ek)
{
	auto dstPart = dst.Take(src.Length());
	if(exp == 0 || ek == 1)
	{
		Multiply(dstPart, src, exp);
		dst.Begin = dstPart.End;
		src.Begin += dstPart.Length();
		return;
	}

	float* const dstEnd = dstPart.End;
	float* pdst = dst.Begin;
	const float* psrc = src.Begin;
	float mul = exp;
#if INTRA_MINEXE > 2

#elif(INTRA_SIMD_SUPPORT == INTRA_SIMD_NONE)
	float ek8 = ek*ek;
	ek8 *= ek8;
	ek8 *= ek8;
	while(pdst+7 < dstEnd)
	{
		*pdst++ = *psrc++ * mul;
		*pdst++ = *psrc++ * mul;
		*pdst++ = *psrc++ * mul;
		*pdst++ = *psrc++ * mul;
		*pdst++ = *psrc++ * mul;
		*pdst++ = *psrc++ * mul;
		*pdst++ = *psrc++ * mul;
		*pdst++ = *psrc++ * mul;
		mul *= ek8;
	}
#else
	float ek4 = ek*ek;
	ek4 *= ek4;
	Simd::float4 ek_4 = Simd::SetFloat4(ek4);
	//while(((size_t)dst)&15 && dst<dstend) *dst++ = *src++ * Exp, Exp*=ek;
	Simd::float4 exp_4 = Simd::SetFloat4(mul, mul*ek, mul*ek*ek, mul*ek*ek*ek);
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
	while(pdst + 7 < dstEnd)
	{
		r0 = Simd::Mul(Simd::SetFloat4U(psrc), r1);
		r1 = Simd::Mul(r1, ek_8);
		r3 = Simd::Mul(Simd::SetFloat4U(psrc + 4), r4);
		Simd::GetU(pdst, r0);
		r4 = Simd::Mul(r4, ek_8);
		Simd::GetU(pdst + 4, r3);
		pdst += 8;
		psrc += 8;
	}
	mul = Simd::GetX(r1);
#endif
	while(pdst < dstEnd)
	{
		*pdst++ = *psrc++ * mul;
		mul *= ek;
	}

	dst.Begin = pdst;
	src.Begin = psrc;
	exp = mul;
}

void ExponentialAttenuateAdd(Span<float>& dst, CSpan<float>& src, float& exp, float ek)
{
	const auto dstPart = dst.Take(src.Length());
	if(exp == 0 || ek == 1)
	{
		AddMultiplied(dstPart, src, exp);
		dst.Begin = dstPart.End;
		src.Begin += dst.Length();
		return;
	}

	const float* const dstEnd = dstPart.End;
	float* pdst = dst.Begin;
	const float* psrc = src.Begin;
	float mul = exp;

#if INTRA_MINEXE > 2

#elif(INTRA_SIMD_SUPPORT == INTRA_SIMD_NONE)
	float ek8 = ek*ek;
	ek8 *= ek8;
	ek8 *= ek8;
	while(pdst + 7 < dstEnd)
	{
		*pdst++ += *psrc++ * mul;
		*pdst++ += *psrc++ * mul;
		*pdst++ += *psrc++ * mul;
		*pdst++ += *psrc++ * mul;
		*pdst++ += *psrc++ * mul;
		*pdst++ += *psrc++ * mul;
		*pdst++ += *psrc++ * mul;
		*pdst++ += *psrc++ * mul;
		mul *= ek8;
	}
#else
	float ek4 = ek*ek;
	ek4 *= ek4;
	Simd::float4 ek_4 = Simd::SetFloat4(ek4);
	Simd::float4 exp_4 = Simd::SetFloat4(mul, mul*ek, mul*ek*ek, mul*ek*ek*ek);

	Simd::float4 r0, r1, ek_8, r3, r4;
	r1 = exp_4;
	ek_8 = ek_4;
	r4 = Simd::Mul(ek_8, r1);
	ek_8 = Simd::Mul(ek_8, ek_8);
	while(pdst + 7 < dstEnd)
	{
		r0 = Simd::Mul(Simd::SetFloat4U(psrc), r1);
		r1 = Simd::Mul(r1, ek_8);
		r3 = Simd::Mul(Simd::SetFloat4U(psrc+4), r4);
		Simd::GetU(pdst, Simd::Add(Simd::SetFloat4U(pdst), r0));
		r4 = Simd::Mul(r4, ek_8);
		Simd::GetU(pdst+4, Simd::Add(Simd::SetFloat4U(pdst+4), r3));
		pdst += 8;
		psrc += 8;
	}
	mul = Simd::GetX(r1);
#endif
	while(pdst < dstEnd)
	{
		*pdst++ += *psrc++ * mul;
		mul *= ek;
	}

	dst.Begin = pdst;
	src.Begin = psrc;
	exp = mul;
}

void ExponentialLinearAttenuate(Span<float>& dst, CSpan<float>& src, float& exp, float ek, float& u, float du)
{
	auto dstPart = dst.Take(src.Length());
	if(exp == 0 || (u == 0 && du == 0))
	{
		FillZeros(dstPart);
		dst.Begin = dstPart.End;
		src.Begin += dstPart.Length();
		u += du*float(dstPart.Length());
		return;
	}

	if(du == 0)
	{
		float expmul =  exp*u;
		ExponentialAttenuate(dst, src, expmul, ek);
		exp = expmul/u;
		return;
	}

	if(ek == 1)
	{
		float linmul = u*exp;
		LinearMultiply(dst, src, linmul, du*exp);
		dst.Begin = dstPart.End;
		src.Begin += dstPart.Length();
		u = linmul/exp;
		return;
	}

	float* const dstEnd = dstPart.End;
	float* pdst = dst.Begin;
	const float* psrc = src.Begin;
	float expmul = exp;
	float linmul = u;

#if INTRA_MINEXE > 2

#elif(INTRA_SIMD_SUPPORT == INTRA_SIMD_NONE)
	float ek8 = ek*ek;
	ek8 *= ek8;
	ek8 *= ek8;
	const float du8 = du*8;
	while(pdst + 7 < dstEnd)
	{
		const float mult = expmul*linmul;
		*pdst++ = *psrc++ * mult;
		*pdst++ = *psrc++ * mult;
		*pdst++ = *psrc++ * mult;
		*pdst++ = *psrc++ * mult;
		*pdst++ = *psrc++ * mult;
		*pdst++ = *psrc++ * mult;
		*pdst++ = *psrc++ * mult;
		*pdst++ = *psrc++ * mult;
		expmul *= ek8;
		linmul += du8;
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
	while(pdst + 7 < dstEnd)
	{
		r0 = Simd::Mul(Simd::SetFloat4U(psrc), Simd::Mul(r1, u_4));
		u_4 = Simd::Add(u_4, du_4);
		r1 = Simd::Mul(r1, ek_8);
		r3 = Simd::Mul(Simd::SetFloat4U(psrc + 4), Simd::Mul(r4, u_4));
		u_4 = Simd::Add(u_4, du_4);
		Simd::GetU(pdst, r0);
		r4 = Simd::Mul(r4, ek_8);
		Simd::GetU(pdst + 4, r3);
		pdst += 8;
		psrc += 8;
	}
	expmul = Simd::GetX(r1);
	linmul = Simd::GetX(u_4);
#endif
	while(pdst < dstEnd)
	{
		*pdst++ = *psrc++ * expmul * linmul;
		expmul *= ek;
		linmul += du;
	}

	dst.Begin = pdst;
	src.Begin = psrc;
	exp = expmul;
	u = linmul;
}

void ExponentialLinearAttenuateAdd(Span<float>& dst, CSpan<float>& src, float& exp, float ek, float& u, float du)
{
	auto dstPart = dst.Take(src.Length());
	if(exp == 0 || (u == 0 && du == 0))
	{
		dst.Begin = dstPart.End;
		src.Begin += dstPart.Length();
		u += du*float(dstPart.Length());
		return;
	}

	if(du == 0)
	{
		float expmul = exp*u;
		ExponentialAttenuateAdd(dst, src, expmul, ek);
		exp = expmul/u;
		return;
	}

	if(ek == 1)
	{
		float linmul = u*exp;
		LinearMultiplyAdd(dst, src, linmul, du*exp);
		dst.Begin = dstPart.End;
		src.Begin += dstPart.Length();
		u = linmul/exp;
		return;
	}

	const float* const dstEnd = dstPart.End;
	float* pdst = dst.Begin;
	const float* psrc = src.Begin;
	float expmul = exp;
	float linmul = u;

#if INTRA_MINEXE > 2

#elif(INTRA_SIMD_SUPPORT == INTRA_SIMD_NONE)
	const float du8 = du*8;
	float ek8 = ek*ek;
	ek8 *= ek8;
	ek8 *= ek8;
	while(pdst + 7 < dstEnd)
	{
		const float mult = linmul*expmul;
		*pdst++ += *psrc++ * mult;
		*pdst++ += *psrc++ * mult;
		*pdst++ += *psrc++ * mult;
		*pdst++ += *psrc++ * mult;
		*pdst++ += *psrc++ * mult;
		*pdst++ += *psrc++ * mult;
		*pdst++ += *psrc++ * mult;
		*pdst++ += *psrc++ * mult;
		expmul *= ek8;
		linmul += du8;
	}
#else
	float ek4 = ek*ek;
	ek4 *= ek4;
	const Simd::float4 ek_4 = Simd::SetFloat4(ek4);
	Simd::float4 exp_4 = Simd::SetFloat4(expmul, expmul*ek, expmul*ek*ek, expmul*ek*ek*ek);
	const Simd::float4 du_4 = Simd::SetFloat4(du*4);
	Simd::float4 u_4 = Simd::SetFloat4(linmul, linmul + du, linmul + 2*du, linmul + 3*du);

	Simd::float4 r0, r1, ek_8, r3, r4;
	r1 = exp_4;
	ek_8 = ek_4;
	r4 = Simd::Mul(ek_8, r1);
	ek_8 = Simd::Mul(ek_8, ek_8);
	while(pdst + 7 < dstEnd)
	{
		r0 = Simd::Mul(Simd::SetFloat4U(psrc), Simd::Mul(r1, u_4));
		u_4 = Simd::Add(u_4, du_4);
		r1 = Simd::Mul(r1, ek_8);
		r3 = Simd::Mul(Simd::SetFloat4U(psrc + 4), Simd::Mul(r4, u_4));
		u_4 = Simd::Add(u_4, du_4);
		Simd::GetU(pdst, Simd::Add(Simd::SetFloat4U(pdst), r0));
		r4 = Simd::Mul(r4, ek_8);
		Simd::GetU(pdst + 4, Simd::Add(Simd::SetFloat4U(pdst + 4), r3));
		pdst += 8;
		psrc += 8;
	}
	expmul = Simd::GetX(r1);
	linmul = Simd::GetX(u_4);
#endif
	while(pdst < dstEnd)
	{
		*pdst++ += *psrc++ * expmul * linmul;
		expmul *= ek;
		linmul += du;
	}

	dst.Begin = pdst;
	src.Begin = psrc;
	exp = expmul;
	u = linmul;
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
	auto src = inOutSamples.AsConstRange();
	ExponentialAttenuate(inOutSamples, src, Factor, FactorStep);
}

void ExponentAttenuator::operator()(Span<float> dstSamples, CSpan<float> srcSamples, bool add)
{
	if(add) ExponentialAttenuateAdd(dstSamples, srcSamples, Factor, FactorStep);
	else ExponentialAttenuate(dstSamples, srcSamples, Factor, FactorStep);
}

void ExponentAttenuator::SkipSamples(size_t count)
{
	Factor *= Math::PowInt(FactorStep, int(count));
}

}}}

INTRA_WARNING_POP
