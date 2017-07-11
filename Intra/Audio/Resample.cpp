#include "Resample.h"
#include "Math/Math.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio {

void ResampleLinear(CSpan<float> src, Span<float> dst)
{
	const size_t srcL1 = src.Length() - 1;
	const size_t dstL1 = dst.Length() - 1;
	const float ratio = float(srcL1) / dstL1;
	for(size_t i = 0; i <= dstL1; i++)
		dst[i] = LinearSample(src, i*ratio);
}


Span<float> DecimateX2LinearInPlace(Span<float> inOutSamples)
{
	const size_t newLen = (inOutSamples.Length() + 1) / 2;
	for(size_t i = 1; i < newLen; i++)
		inOutSamples[i] = (inOutSamples[2*i - 1] + inOutSamples[2*i]) / 2;
	return inOutSamples.Take(newLen);
}

Span<float> DecimateX2Linear(Span<float> dst, CSpan<float> src)
{
	const size_t newLen = (src.Length() + 1) / 2;
	INTRA_DEBUG_ASSERT(dst.Length() >= newLen);
	for(size_t i = 1; i < newLen; i++)
		dst[i] = (src[2*i - 1] + src[2*i]) / 2;
	return dst.Take(newLen);
}

}}

INTRA_WARNING_POP
