#include "Algo/Mutation/Transform.h"
#include "Platform/Debug.h"
#include "Range/Generators/ArrayRange.h"
#include "Math/Simd.h"

namespace Intra { namespace Algo {

void Add(ArrayRange<float> dstOp1, ArrayRange<const float> op2)
{
	INTRA_ASSERT(dstOp1.Length()==op2.Length());
	auto dst = dstOp1.Begin;
	auto src = op2.Begin;

#if(INTRA_SIMD_SUPPORT==INTRA_SIMD_NONE)
	while(dst<dstOp1.End-3)
	{
		*dst++ += *src++;
		*dst++ += *src++;
		*dst++ += *src++;
		*dst++ += *src++;
	}
#else
	while(dst<dstOp1.End && size_t(dst)%16!=0) *dst++ += *src++;
	while(dst<dstOp1.End-3)
	{
		Simd::Get(dst, Simd::Add(Simd::SetFloat4(dst), Simd::SetFloat4U(src)));
		dst += 4;
		src += 4;
	}
#endif
	while(dst<dstOp1.End) *dst++ += *src++;
}

void Multiply(ArrayRange<float> dstOp1, ArrayRange<const float> op2)
{
	INTRA_ASSERT(dstOp1.Length()==op2.Length());
	auto dst = dstOp1.Begin;
	auto src = op2.Begin;

#if(INTRA_SIMD_SUPPORT==INTRA_SIMD_NONE)
	while(dst<dstOp1.End-3)
	{
		*dst++ *= *src++;
		*dst++ *= *src++;
		*dst++ *= *src++;
		*dst++ *= *src++;
	}
#else
	while(dst<dstOp1.End && size_t(dst)%16!=0) *dst++ *= *src++;
	while(dst<dstOp1.End-3)
	{
		Simd::Get(dst, Simd::Mul(Simd::SetFloat4(dst), Simd::SetFloat4U(src)));
		dst += 4;
		src += 4;
	}
#endif
	while(dst<dstOp1.End) *dst++ *= *src++;
}

void Multiply(ArrayRange<float> dstOp1, float multiplyer)
{
	auto dst = dstOp1.Begin;

#if(INTRA_SIMD_SUPPORT==INTRA_SIMD_NONE)
	while(dst<dstOp1.End-3)
	{
		*dst++ *= multiplyer;
		*dst++ *= multiplyer;
		*dst++ *= multiplyer;
		*dst++ *= multiplyer;
	}
#else
	while(dst<dstOp1.End && size_t(dst)%16!=0) *dst++ *= multiplyer;
	Simd::float4 multiplyerVec = Simd::SetFloat4(multiplyer);
	while(dst<dstOp1.End-3)
	{
		Simd::Get(dst, Simd::Mul(Simd::SetFloat4(dst), multiplyerVec));
		dst += 4;
	}
#endif
	while(dst<dstOp1.End) *dst++ *= multiplyer;
}

void Multiply(ArrayRange<float> dst, ArrayRange<const float> op1, float multiplyer)
{
	INTRA_ASSERT(dst.Length()==op1.Length());
#if(INTRA_SIMD_SUPPORT==INTRA_SIMD_NONE)
	while(dst.Begin<dst.End-3)
	{
		*dst.Begin++ = *op1.Begin++ * multiplyer;
		*dst.Begin++ = *op1.Begin++ * multiplyer;
		*dst.Begin++ = *op1.Begin++ * multiplyer;
		*dst.Begin++ = *op1.Begin++ * multiplyer;
	}
#else
	while(dst.Begin<dst.End && size_t(dst.Begin)%16!=0) *dst.Begin++ = *op1.Begin++ * multiplyer;
	Simd::float4 multiplyerVec = Simd::SetFloat4(multiplyer);
	while(dst.Begin<dst.End-3)
	{
		Simd::Get(dst.Begin, Simd::Mul(Simd::SetFloat4U(op1.Begin), multiplyerVec));
		dst.Begin += 4;
		op1.Begin += 4;
	}
#endif
	while(dst.Begin<dst.End) *dst.Begin++ = *op1.Begin++ * multiplyer;
}


void MulAdd(ArrayRange<float> dstOp1, float mul, float add)
{
	auto dst = dstOp1.Begin;

#if(INTRA_SIMD_SUPPORT==INTRA_SIMD_NONE)
	while(dst<dstOp1.End-3)
	{
		*dst = *dst * mul + add; dst++;
		*dst = *dst * mul + add; dst++;
		*dst = *dst * mul + add; dst++;
		*dst = *dst * mul + add; dst++;
	}
#else
	while(dst<dstOp1.End && size_t(dst)%16!=0) *dst = *dst * mul + add, dst++;
	Simd::float4 addVec = Simd::SetFloat4(add);
	Simd::float4 mulVec = Simd::SetFloat4(mul);
	while(dst<dstOp1.End-3)
	{
		Simd::float4 v = Simd::SetFloat4(dst);
		v = Simd::Mul(v, mulVec);
		v = Simd::Add(v, addVec);
		Simd::Get(dst, v);
		dst += 4;
	}
#endif

	while(dst<dstOp1.End) *dst = *dst * mul + add, dst++;
}

}}
