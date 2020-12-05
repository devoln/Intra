#include "Resample.h"
#include "Intra/Range/StringView.h"
#include "Intra/Math/Math.h"

INTRA_BEGIN
void ResampleLinear(CSpan<float> src, Span<float> dst)
{
	const auto srcL1 = src.Length() - 1;
	const auto dstL1 = dst.Length() - 1;
	const float ratio = float(srcL1) / float(dstL1);
	for(index_t i = 0; i < dstL1; i++)
		dst[i] = LinearSample(src, float(i)*ratio);
	dst.Last() = src.Last();
}


Span<float> DecimateX2LinearInPlace(Span<float> inOutSamples)
{
	const auto newLen = index_t(size_t(inOutSamples.Length()) / 2);
	for(index_t i = 0; i < newLen; i++)
		inOutSamples[i] = (inOutSamples[2*i] + inOutSamples[2*i + 1]) * 0.5f;
	return inOutSamples.TakeExactly(newLen);
}

Span<float> DecimateX2Linear(Span<float> dst, CSpan<float> src)
{
	const auto newLen = index_t(size_t(src.Length()) / 2);
	INTRA_PRECONDITION(dst.Length() >= newLen);
	for(index_t i = 0; i < newLen; i++)
		dst[i] = (src[2*i] + src[2*i + 1]) * 0.5f;
	return dst.TakeExactly(newLen);
}

Span<float> UpsampleX2Linear(Span<float> dst, CSpan<float> src)
{
	const auto newLen = src.Length()*2;
	dst = dst.TakeExactly(newLen);
	if(src.Empty()) return dst;

	dst.First() = src.First();
	for(index_t i = 1; i < src.Length(); i++)
	{
		dst[2*i - 1] = src[i-1];
		dst[2*i] = (src[i-1] + src[i]) * 0.5f;
	}
	dst.Last() = src.Last();
	return dst;
}
INTRA_END
