#include "Algo/Mutation/Transform.h"
#include "Platform/Debug.h"
#include "Range/Generators/ArrayRange.h"
#include "Math/Simd.h"
#include "Algo/Op.h"

namespace Intra { namespace Algo {

size_t AddAdvance(ArrayRange<float>& dstOp1, ArrayRange<const float>& op2)
{
	const size_t len = Op::Min(dstOp1.Length(), op2.Length());
	dstOp1 = dstOp1.Take(len);
	op2 = op2.Take(len);

	auto& dst = dstOp1.Begin;
	auto& src = op2.Begin;

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
	return len;
}

size_t MultiplyAdvance(ArrayRange<float>& dstOp1, ArrayRange<const float>& op2)
{
	const size_t len = Op::Min(dstOp1.Length(), op2.Length());
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
	while(dst<dstOp1.End && size_t(dst)%16!=0) *dst++ *= *src++;
	while(dst<dstOp1.End-3)
	{
		Simd::Get(dst, Simd::Mul(Simd::SetFloat4(dst), Simd::SetFloat4U(src)));
		dst += 4;
		src += 4;
	}
#endif
	while(dst<dstOp1.End) *dst++ *= *src++;
	return len;
}

size_t MultiplyAdvance(ArrayRange<float>& dstOp1, float multiplyer)
{
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
	while(dst<dstOp1.End && size_t(dst)%16!=0) *dst++ *= multiplyer;
	Simd::float4 multiplyerVec = Simd::SetFloat4(multiplyer);
	while(dst<dstOp1.End-3)
	{
		Simd::Get(dst, Simd::Mul(Simd::SetFloat4(dst), multiplyerVec));
		dst += 4;
	}
#endif
	while(dst<dstOp1.End) *dst++ *= multiplyer;
	return len;
}

size_t MultiplyAdvance(ArrayRange<float>& dest, ArrayRange<const float>& op1, float multiplyer)
{
	const size_t len = Op::Min(dest.Length(), op1.Length());
	dest = dest.Take(len);
	op1 = op1.Take(len);
	auto& dst = dest.Begin;
	auto& src = op1.Begin;
#if(INTRA_SIMD_SUPPORT==INTRA_SIMD_NONE)
	while(dst<dest.End-3)
	{
		*dst++ = *src++ * multiplyer;
		*dst++ = *src++ * multiplyer;
		*dst++ = *src++ * multiplyer;
		*dst++ = *src++ * multiplyer;
	}
#else
	while(dst<dest.End && size_t(dst)%16!=0) *dst++ = *src++ * multiplyer;
	Simd::float4 multiplyerVec = Simd::SetFloat4(multiplyer);
	while(dst < dest.End-3)
	{
		Simd::Get(dst, Simd::Mul(Simd::SetFloat4U(src), multiplyerVec));
		dst += 4;
		src += 4;
	}
#endif
	while(dst<dest.End) *dst++ = *src++ * multiplyer;
	return len;
}


size_t MulAddAdvance(ArrayRange<float>& dstOp1, float mul, float add)
{
	const size_t len = dstOp1.Length();
	auto& dst = dstOp1.Begin;

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
	return len;
}

}}
