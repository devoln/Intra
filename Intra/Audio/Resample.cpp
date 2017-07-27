#include "Resample.h"
#include "Math/Math.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio {

void ResampleLinear(CSpan<float> src, Span<float> dst)
{
	const size_t srcL1 = src.Length() - 1;
	const size_t dstL1 = dst.Length() - 1;
	const float ratio = float(srcL1) / float(dstL1);
	for(size_t i = 0; i < dstL1; i++)
		dst[i] = LinearSample(src, float(i)*ratio);
	dst.Last() = src.Last();
}


Span<float> DecimateX2LinearInPlace(Span<float> inOutSamples)
{
	const size_t newLen = inOutSamples.Length() / 2;
	for(size_t i = 0; i < newLen; i++)
		inOutSamples.Begin[i] = (inOutSamples.Begin[2*i] + inOutSamples.Begin[2*i + 1]) * 0.5f;
	return inOutSamples.TakeExactly(newLen);
}

Span<float> DecimateX2Linear(Span<float> dst, CSpan<float> src)
{
	const size_t newLen = src.Length() / 2;
	INTRA_DEBUG_ASSERT(dst.Length() >= newLen);
	for(size_t i = 0; i < newLen; i++)
		dst.Begin[i] = (src.Begin[2*i] + src.Begin[2*i + 1]) * 0.5f;
	return dst.TakeExactly(newLen);
}

Span<float> UpsampleX2Linear(Span<float> dst, CSpan<float> src)
{
	const size_t newLen = src.Length()*2;
	dst = dst.TakeExactly(newLen);
	if(src.Empty()) return dst;

	dst.First() = src.First();
	for(size_t i = 1; i < src.Length(); i++)
	{
		dst.Begin[2*i - 1] = src.Begin[i-1];
		dst.Begin[2*i] = (src.Begin[i-1] + src.Begin[i]) * 0.5f;
	}
	dst.Last() = src.Last();
	return dst;
}

}}

INTRA_WARNING_POP
