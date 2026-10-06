#include <Simd/SimdVector.h>

#include "Transform.h"
#include "Copy.h"
#include "Fill.h"

#include "Utils/Debug.h"
#include "Utils/Span.h"
#include "Funal/Op.h"

namespace Intra { namespace Range {

size_t AddAdvance(Span<float>& dstOp1, CSpan<float>& op2)
{
	const size_t len = Funal::Min(dstOp1.Length(), op2.Length());
	dstOp1 = dstOp1.Take(len);
	op2 = op2.Take(len);

	auto& dst = dstOp1.Begin;
	auto& src = op2.Begin;

	if constexpr(Config::TargetMaxSimdLength<float> >= 4)
	{
		while(dst < dstOp1.End && size_t(dst) % 16 != 0) *dst++ += *src++;
		while(dst < dstOp1.End-3)
		{
			const auto v = SimdLoadAligned<4>(dst) + SimdLoad<4>(src);
			SimdStoreAligned(dst, v);
			dst += 4;
			src += 4;
		}
	}
	else while(dst < dstOp1.End - 3)
	{
		*dst++ += *src++;
		*dst++ += *src++;
		*dst++ += *src++;
		*dst++ += *src++;
	}
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

	if constexpr(Config::TargetMaxSimdLength<float> >= 4)
	{
		while(dst < dstOp1.End && size_t(dst) % 16 != 0) *dst++ *= *src++;
		while(dst < dstOp1.End - 3)
		{
			SimdStoreAligned(dst, SimdLoadAligned<4>(dst) * SimdLoad<4>(src));
			dst += 4;
			src += 4;
		}
	}
	else while(dst<dstOp1.End-3)
	{
		*dst++ *= *src++;
		*dst++ *= *src++;
		*dst++ *= *src++;
		*dst++ *= *src++;
	}
	while(dst<dstOp1.End) *dst++ *= *src++;
	return len;
}

size_t MultiplyAdvance(Span<float>& dstOp1, float multiplyer)
{
	if(multiplyer == 0) FillZeros(dstOp1);
	if(multiplyer == 0 || multiplyer == 1) return dstOp1.PopFirstN(dstOp1.Length());

	const size_t len = dstOp1.Length();
	auto& dst = dstOp1.Begin;

	if constexpr(Config::TargetMaxSimdLength<float> >= 4)
	{
		while(dst < dstOp1.End && size_t(dst) % 16 != 0) *dst++ *= multiplyer;
		const auto multiplyerVec = SimdVectorFilled<float, 4>(multiplyer);
		while(dst < dstOp1.End - 3)
		{
			SimdStoreAligned(dst, SimdLoadAligned<4>(dst) * multiplyerVec);
			dst += 4;
		}
	}
	else while(dst<dstOp1.End-3)
	{
		*dst++ *= multiplyer;
		*dst++ *= multiplyer;
		*dst++ *= multiplyer;
		*dst++ *= multiplyer;
	}
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
	if constexpr(Config::TargetMaxSimdLength<float> >= 4)
	{
		while(dst < dstend && size_t(dst) % 16 != 0) *dst++ = *src++ * multiplyer;
		const auto multiplyerVec = SimdVectorFilled<float, 4>(multiplyer);
		while(dst + 3 < dstend)
		{
			SimdStoreAligned(dst, SimdLoad<4>(src) * multiplyerVec);
			dst += 4;
			src += 4;
		}
	}
	else while(dst + 3 < dstend)
	{
		*dst++ = *src++ * multiplyer;
		*dst++ = *src++ * multiplyer;
		*dst++ = *src++ * multiplyer;
		*dst++ = *src++ * multiplyer;
	}
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
	if constexpr(Config::TargetMaxSimdLength<float> >= 4)
	{
		while(dst + 3 < dstend && size_t(dst) % 16 != 0) *dst++ += *src++ * op2Multiplyer;
		const auto multiplyerVec = SimdVectorFilled<float, 4>(op2Multiplyer);
		while(dst + 3 < dstend)
		{
			const auto dstVal = SimdLoadAligned<4>(dst);
			const auto srcVal = SimdLoad<4>(src);
			SimdStoreAligned(dst, dstVal + srcVal * multiplyerVec);
			dst += 4;
			src += 4;
		}
	}
	else while(dst + 3 < dstend)
	{
		*dst++ += *src++ * op2Multiplyer;
		*dst++ += *src++ * op2Multiplyer;
		*dst++ += *src++ * op2Multiplyer;
		*dst++ += *src++ * op2Multiplyer;
	}
	while(dst < dstend) *dst++ += *src++ * op2Multiplyer;
	return len;
}


size_t MulAddAdvance(Span<float>& dstOp1, float mul, float add)
{
	const size_t len = dstOp1.Length();
	auto& dst = dstOp1.Begin;

	if constexpr(Config::TargetMaxSimdLength<float> >= 4)
	{
		while(dst < dstOp1.End && size_t(dst) % 16 != 0)
			*dst = *dst * mul + add, dst++;
		const auto addVec = SimdVectorFilled<float, 4>(add);
		const auto mulVec = SimdVectorFilled<float, 4>(mul);
		while(dst < dstOp1.End-3)
		{
			auto v = SimdLoadAligned<4>(dst) * mulVec;
			SimdStoreAligned(dst, v + addVec);
			dst += 4;
		}
	}
	else while(dst + 3 < dstOp1.End)
	{
		*dst = *dst * mul + add; dst++;
		*dst = *dst * mul + add; dst++;
		*dst = *dst * mul + add; dst++;
		*dst = *dst * mul + add; dst++;
	}

	while(dst < dstOp1.End) *dst = *dst * mul + add, dst++;
	return len;
}

void LinearMultiply(Span<float> dst, float& u, float du)
{
#if INTRA_MINEXE >= 3
#else
	if constexpr(Config::TargetMaxSimdLength<float> >= 4)
	{
		SimdVector<float, 4> u4{u, u + du, u + 2*du, u + 3*du};
		const auto du4 = SimdVectorFilled<float, 4>(4*du);
		while(dst.Begin + 3 < dst.End)
		{
			SimdStore(dst.Begin, SimdLoad<4>(dst.Begin) * u4);
			u4 = u4 + du4;
			dst.Begin += 4;
		}
		u = u4[0];
	}
	else
	{
		const float du4 = 4*du;
		while(dst.Begin + 3 < dst.End)
		{
			*dst.Begin++ *= u;
			*dst.Begin++ *= u;
			*dst.Begin++ *= u;
			*dst.Begin++ *= u;
			u += du4;
		}
	}
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
#else
	if constexpr(Config::TargetMaxSimdLength<float> >= 4)
	{
		SimdVector<float, 4> u4{u, u + du, u + 2*du, u + 3*du};
		const auto du4 = SimdVectorFilled<float, 4>(4*du);
		while(dst.Begin + 3 < dst.End)
		{
			SimdStore(dst.Begin, SimdLoad<4>(src.Begin) * u4);
			u4 = u4 + du4;
			src.Begin += 4;
			dst.Begin += 4;
		}
		u = u4[0];
	}
	else
	{
		const float du4 = 4*du;
		while(dst.Begin + 3 < dst.End)
		{
			*dst.Begin++ = *src.Begin++ * u;
			*dst.Begin++ = *src.Begin++ * u;
			*dst.Begin++ = *src.Begin++ * u;
			*dst.Begin++ = *src.Begin++ * u;
			u += du4;
		}
	}
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
#else
	if constexpr(Config::TargetMaxSimdLength<float> >= 4)
	{
		SimdVector<float, 4> u4{u, u + du, u + 2*du, u + 3*du};
		const auto du4 = SimdVectorFilled<float, 4>(4*du);
		while(dst.Begin + 3 < dst.End)
		{
			const auto value = SimdLoad<4>(dst.Begin) + SimdLoad<4>(src.Begin) * u4;
			SimdStore(dst.Begin, value);
			u4 = u4 + du4;
			src.Begin += 4;
			dst.Begin += 4;
		}
		u = u4[0];
	}
	else
	{
		const float du4 = 4*du;
		while(dst.Begin + 3 < dst.End)
		{
			*dst.Begin++ += *src.Begin++ * u;
			*dst.Begin++ += *src.Begin++ * u;
			*dst.Begin++ += *src.Begin++ * u;
			*dst.Begin++ += *src.Begin++ * u;
			u += du4;
		}
	}
#endif
	while(dst.Begin < dst.End)
	{
		*dst.Begin++ += *src.Begin++ * u;
		u += du;
	}
}

}}
