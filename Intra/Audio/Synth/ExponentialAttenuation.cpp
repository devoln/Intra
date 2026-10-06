#include <Simd/SimdVector.h>

#include "Audio/Synth/ExponentialAttenuation.h"

#include "Utils/Span.h"

#include "Math/Math.h"

#include "Funal/Bind.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio { namespace Synth {

void ExponentialAttenuate(Span<float>& dst, CSpan<float> src, float& exp, float ek)
{
	float* const dstEnd = dst.Take(src.Length()).End;
#if INTRA_MINEXE > 2
#else
	if constexpr(Config::TargetMaxSimdLength<float> >= 4)
	{
		using F32x4 = SimdVector<float, 4>;
		float ek4 = ek*ek;
		ek4 *= ek4;
		const auto ek_4 = SimdVectorFilled<float, 4>(ek4);
		F32x4 exp_4{exp, exp*ek, exp*ek*ek, exp*ek*ek*ek};
		F32x4 r1 = exp_4;
		auto ek_8 = ek_4;
		auto r4 = ek_8 * r1;
		ek_8 = ek_8 * ek_8;
		while(dst.Begin + 7 < dstEnd)
		{
			const auto r0 = SimdLoad<4>(src.Begin) * r1;
			r1 = r1 * ek_8;
			const auto r3 = SimdLoad<4>(src.Begin + 4) * r4;
			SimdStore(dst.Begin, r0);
			r4 = r4 * ek_8;
			SimdStore(dst.Begin + 4, r3);
			dst.Begin += 8;
			src.Begin += 8;
		}
		exp = r1[0];
	}
	else
	{
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
	}
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
#else
	if constexpr(Config::TargetMaxSimdLength<float> >= 4)
	{
		using F32x4 = SimdVector<float, 4>;
		float ek4 = ek*ek;
		ek4 *= ek4;
		const auto ek_4 = SimdVectorFilled<float, 4>(ek4);
		F32x4 r1{exp, exp*ek, exp*ek*ek, exp*ek*ek*ek};
		auto ek_8 = ek_4;
		auto r4 = ek_8 * r1;
		ek_8 = ek_8 * ek_8;
		while(dst.Begin < dstEnd - 7)
		{
			const auto r0 = SimdLoad<4>(src.Begin) * r1;
			r1 = r1 * ek_8;
			const auto r3 = SimdLoad<4>(src.Begin + 4) * r4;
			SimdStore(dst.Begin, SimdLoad<4>(dst.Begin) + r0);
			r4 = r4 * ek_8;
			SimdStore(dst.Begin + 4, SimdLoad<4>(dst.Begin + 4) + r3);
			dst.Begin += 8;
			src.Begin += 8;
		}
		exp = r1[0];
	}
	else
	{
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
	}
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
#else
	if constexpr(Config::TargetMaxSimdLength<float> >= 4)
	{
		using F32x4 = SimdVector<float, 4>;
		float ek4 = ek*ek;
		ek4 *= ek4;
		const auto ek_4 = SimdVectorFilled<float, 4>(ek4);
		F32x4 r1{exp, exp*ek, exp*ek*ek, exp*ek*ek*ek};
		const auto du_4 = SimdVectorFilled<float, 4>(du*4);
		F32x4 u_4{u, u + du, u + 2*du, u + 3*du};
		auto ek_8 = ek_4;
		auto r4 = ek_8 * r1;
		ek_8 = ek_8 * ek_8;
		while(dst.Begin + 7 < dstEnd)
		{
			const auto r0 = SimdLoad<4>(src.Begin) * (r1 * u_4);
			u_4 = u_4 + du_4;
			r1 = r1 * ek_8;
			const auto r3 = SimdLoad<4>(src.Begin + 4) * (r4 * u_4);
			u_4 = u_4 + du_4;
			SimdStore(dst.Begin, r0);
			r4 = r4 * ek_8;
			SimdStore(dst.Begin + 4, r3);
			dst.Begin += 8;
			src.Begin += 8;
		}
		exp = r1[0];
		u = u_4[0];
	}
	else
	{
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
	}
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
#else
	if constexpr(Config::TargetMaxSimdLength<float> >= 4)
	{
		using F32x4 = SimdVector<float, 4>;
		float ek4 = ek*ek;
		ek4 *= ek4;
		const auto ek_4 = SimdVectorFilled<float, 4>(ek4);
		F32x4 r1{exp, exp*ek, exp*ek*ek, exp*ek*ek*ek};
		const auto du_4 = SimdVectorFilled<float, 4>(du*4);
		F32x4 u_4{u, u + du, u + 2*du, u + 3*du};
		auto ek_8 = ek_4;
		auto r4 = ek_8 * r1;
		ek_8 = ek_8 * ek_8;
		while(dst.Begin + 7 < dstEnd)
		{
			const auto r0 = SimdLoad<4>(src.Begin) * (r1 * u_4);
			u_4 = u_4 + du_4;
			r1 = r1 * ek_8;
			const auto r3 = SimdLoad<4>(src.Begin + 4) * (r4 * u_4);
			u_4 = u_4 + du_4;
			SimdStore(dst.Begin, SimdLoad<4>(dst.Begin) + r0);
			r4 = r4 * ek_8;
			SimdStore(dst.Begin + 4, SimdLoad<4>(dst.Begin + 4) + r3);
			dst.Begin += 8;
			src.Begin += 8;
		}
		exp = r1[0];
		u = u_4[0];
	}
	else
	{
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
	}
#endif
	while(dst.Begin < dstEnd)
	{
		*dst.Begin++ += *src.Begin++ * exp * u;
		exp *= ek;
		u += du;
	}
}

void ExponentAttenuator::operator()(Span<float> inOutSamples)
{
	ExponentialAttenuate(inOutSamples, inOutSamples, mFactor, mFactorStep);
}

}}}

INTRA_WARNING_POP
