#include "Transform.h"
#include "Copy.h"
#include "Fill.h"

#include "Utils/Debug.h"
#include "Utils/Span.h"
#include "Simd/Simd.h"
#include "Funal/Op.h"

namespace Intra { namespace Range {

size_t AddAdvance(Span<float>& dstOp1, CSpan<float>& op2)
{
	const size_t len = Funal::Min(dstOp1.Length(), op2.Length());
	dstOp1 = dstOp1.Take(len);
	op2 = op2.Take(len);

	auto& dst = dstOp1.Begin;
	auto& src = op2.Begin;

#if(INTRA_SIMD_SUPPORT==INTRA_SIMD_NONE)
	while(dst < dstOp1.End - 3)
	{
		*dst++ += *src++;
		*dst++ += *src++;
		*dst++ += *src++;
		*dst++ += *src++;
	}
#else
	while(dst < dstOp1.End && size_t(dst) % 16 != 0) *dst++ += *src++;
	while(dst < dstOp1.End-3)
	{
		Simd::Get(dst, Simd::Add(Simd::SetFloat4(dst), Simd::SetFloat4U(src)));
		dst += 4;
		src += 4;
	}
#endif
	while(dst<dstOp1.End) *dst++ += *src++;
	return len;
}

size_t MultiplyAdvance(Span<float>& dstOp1, CSpan<float>& op2)
{
	const size_t len = Funal::Min(dstOp1.Length(), op2.Length());
	dstOp1 = dstOp1.Take(len);
	op2 = op2.Take(len);
	auto& dst = dstOp1.Begin;
	auto& src = op2.Begin;

#if(INTRA_SIMD_SUPPORT==INTRA_SIMD_NONE)
	while(dst<dstOp1.End-3)
	{
		*dst++ *= *src++;
		*dst++ *= *src++;
		*dst++ *= *src++;
		*dst++ *= *src++;
	}
#else
	while(dst < dstOp1.End && size_t(dst) % 16 != 0) *dst++ *= *src++;
	while(dst < dstOp1.End - 3)
	{
		Simd::Get(dst, Simd::Mul(Simd::SetFloat4(dst), Simd::SetFloat4U(src)));
		dst += 4;
		src += 4;
	}
#endif
	while(dst<dstOp1.End) *dst++ *= *src++;
	return len;
}

size_t MultiplyAdvance(Span<float>& dstOp1, float multiplyer)
{
	if(multiplyer == 0) FillZeros(dstOp1);
	if(multiplyer == 0 || multiplyer == 1) return dstOp1.PopFirstN(dstOp1.Length());

	const size_t len = dstOp1.Length();
	auto& dst = dstOp1.Begin;

#if(INTRA_SIMD_SUPPORT==INTRA_SIMD_NONE)
	while(dst<dstOp1.End-3)
	{
		*dst++ *= multiplyer;
		*dst++ *= multiplyer;
		*dst++ *= multiplyer;
		*dst++ *= multiplyer;
	}
#else
	while(dst < dstOp1.End && size_t(dst) % 16 != 0) *dst++ *= multiplyer;
	Simd::float4 multiplyerVec = Simd::SetFloat4(multiplyer);
	while(dst < dstOp1.End - 3)
	{
		Simd::Get(dst, Simd::Mul(Simd::SetFloat4(dst), multiplyerVec));
		dst += 4;
	}
#endif
	while(dst < dstOp1.End) *dst++ *= multiplyer;
	return len;
}

size_t MultiplyAdvance(Span<float>& dest, CSpan<float>& op1, float multiplyer)
{
	if(multiplyer == 1) return ReadWrite(op1, dest);

	const size_t len = Funal::Min(dest.Length(), op1.Length());
	auto& dst = dest.Begin;
	auto& src = op1.Begin;
	auto dstend = dst + len;
#if(INTRA_SIMD_SUPPORT == INTRA_SIMD_NONE)
	while(dst + 3 < dstend)
	{
		*dst++ = *src++ * multiplyer;
		*dst++ = *src++ * multiplyer;
		*dst++ = *src++ * multiplyer;
		*dst++ = *src++ * multiplyer;
	}
#else
	while(dst < dstend && size_t(dst) % 16 != 0) *dst++ = *src++ * multiplyer;
	Simd::float4 multiplyerVec = Simd::SetFloat4(multiplyer);
	while(dst + 3 < dstend)
	{
		Simd::Get(dst, Simd::Mul(Simd::SetFloat4U(src), multiplyerVec));
		dst += 4;
		src += 4;
	}
#endif
	while(dst < dstend) *dst++ = *src++ * multiplyer;
	return len;
}

size_t AddMultipliedAdvance(Span<float>& dstOp1, CSpan<float>& op2, float op2Multiplyer)
{
	if(op2Multiplyer == 1) return AddAdvance(dstOp1, op2);

	const size_t len = Funal::Min(dstOp1.Length(), op2.Length());
	auto& dst = dstOp1.Begin;
	auto& src = op2.Begin;
	auto dstend = dst + len;
#if(INTRA_SIMD_SUPPORT==INTRA_SIMD_NONE)
	while(dst + 3 < dstend)
	{
		*dst++ += *src++ * op2Multiplyer;
		*dst++ += *src++ * op2Multiplyer;
		*dst++ += *src++ * op2Multiplyer;
		*dst++ += *src++ * op2Multiplyer;
	}
#else
	while(dst + 3 < dstend && size_t(dst) % 16 != 0) *dst++ += *src++ * op2Multiplyer;
	Simd::float4 multiplyerVec = Simd::SetFloat4(op2Multiplyer);
	while(dst + 3 < dstend)
	{
		auto dstVal = Simd::SetFloat4(dst);
		auto srcVal = Simd::SetFloat4U(src);
		Simd::Get(dst, Simd::Add(dstVal, Simd::Mul(srcVal, multiplyerVec)));
		dst += 4;
		src += 4;
	}
#endif
	while(dst < dstend) *dst++ += *src++ * op2Multiplyer;
	return len;
}


size_t MulAddAdvance(Span<float>& dstOp1, float mul, float add)
{
	const size_t len = dstOp1.Length();
	auto& dst = dstOp1.Begin;

#if(INTRA_SIMD_SUPPORT==INTRA_SIMD_NONE)
	while(dst + 3 < dstOp1.End)
	{
		*dst = *dst * mul + add; dst++;
		*dst = *dst * mul + add; dst++;
		*dst = *dst * mul + add; dst++;
		*dst = *dst * mul + add; dst++;
	}
#else
	while(dst < dstOp1.End && size_t(dst) % 16 != 0)
		*dst = *dst * mul + add, dst++;
	Simd::float4 addVec = Simd::SetFloat4(add);
	Simd::float4 mulVec = Simd::SetFloat4(mul);
	while(dst < dstOp1.End-3)
	{
		Simd::float4 v = Simd::SetFloat4(dst);
		v = Simd::Mul(v, mulVec);
		v = Simd::Add(v, addVec);
		Simd::Get(dst, v);
		dst += 4;
	}
#endif

	while(dst < dstOp1.End) *dst = *dst * mul + add, dst++;
	return len;
}

void LinearMultiply(Span<float> dst, float& u, float du)
{
#if INTRA_MINEXE >= 3
#elif(INTRA_SIMD_SUPPORT == INTRA_SIMD_NONE)
	const float du4 = 4*du;
	while(dst.Begin + 3 < dst.End)
	{
		*dst.Begin++ *= u;
		*dst.Begin++ *= u;
		*dst.Begin++ *= u;
		*dst.Begin++ *= u;
		u += du4;
	}
#else
	Simd::float4 u4 = Simd::SetFloat4(u, u + du, u + 2*du, u + 3*du);
	Simd::float4 du4 = Simd::SetFloat4(4*du);
	while(dst.Begin + 3 < dst.End)
	{
		Simd::float4 v = Simd::SetFloat4U(dst.Begin);
		Simd::GetU(dst.Begin, Simd::Mul(v, u4));
		u4 = Simd::Add(u4, du4);
		dst.Begin += 4;
	}
	u = Simd::GetX(u4);
#endif
	while(dst.Begin < dst.End)
	{
		*dst.Begin++ *= u;
		u += du;
	}
}

void LinearMultiply(Span<float> dst, CSpan<float> src, float& u, float du)
{
	dst = dst.Take(src.Length());
	src = src.Take(dst.Length());
#if INTRA_MINEXE >= 3
#elif(INTRA_SIMD_SUPPORT == INTRA_SIMD_NONE)
	const float du4 = 4*du;
	while(dst.Begin + 3 < dst.End)
	{
		*dst.Begin++ = *src.Begin++ * u;
		*dst.Begin++ = *src.Begin++ * u;
		*dst.Begin++ = *src.Begin++ * u;
		*dst.Begin++ = *src.Begin++ * u;
		u += du4;
	}
#else
	Simd::float4 u4 = Simd::SetFloat4(u, u + du, u + 2*du, u + 3*du);
	const Simd::float4 du4 = Simd::SetFloat4(4*du);
	while(dst.Begin + 3 < dst.End)
	{
		const Simd::float4 v = Simd::SetFloat4U(src.Begin);
		Simd::GetU(dst.Begin, Simd::Mul(v, u4));
		u4 = Simd::Add(u4, du4);
		src.Begin += 4;
		dst.Begin += 4;
	}
	u = Simd::GetX(u4);
#endif
	while(dst.Begin < dst.End)
	{
		*dst.Begin++ = *src.Begin++ * u;
		u += du;
	}
}

void LinearMultiplyAdd(Span<float> dst, CSpan<float> src, float& u, float du)
{
	dst = dst.Take(src.Length());
	src = src.Take(dst.Length());
#if INTRA_MINEXE >= 3
#elif(INTRA_SIMD_SUPPORT == INTRA_SIMD_NONE)
	const float du4 = 4*du;
	while(dst.Begin + 3 < dst.End)
	{
		*dst.Begin++ += *src.Begin++ * u;
		*dst.Begin++ += *src.Begin++ * u;
		*dst.Begin++ += *src.Begin++ * u;
		*dst.Begin++ += *src.Begin++ * u;
		u += du4;
	}
#else
	Simd::float4 u4 = Simd::SetFloat4(u, u + du, u + 2*du, u + 3*du);
	const Simd::float4 du4 = Simd::SetFloat4(4*du);
	while(dst.Begin + 3 < dst.End)
	{
		const Simd::float4 vsrc = Simd::SetFloat4U(src.Begin);
		const Simd::float4 vdst = Simd::SetFloat4U(dst.Begin);
		Simd::GetU(dst.Begin, Simd::Add(vdst, Simd::Mul(vsrc, u4)));
		u4 = Simd::Add(u4, du4);
		src.Begin += 4;
		dst.Begin += 4;
	}
	u = Simd::GetX(u4);
#endif
	while(dst.Begin < dst.End)
	{
		*dst.Begin++ += *src.Begin++ * u;
		u += du;
	}
}

}}
