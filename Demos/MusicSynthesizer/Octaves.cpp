#include "Octaves.h"
#include "Random/FastUniform.h"

INTRA_BEGIN
namespace Audio { namespace Synth {

void SelfOctaveMix(CSpan<float> src, Span<float> dst, float multiplier)
{
	src = src.Take(dst.Length());
	dst = dst.Take(src.Length());
	INTRA_DEBUG_ASSERT(src != dst);
	const size_t len = src.Length();
	size_t i = 0;
	while(2*i < len)
	{
		dst[i] = src[i] + multiplier*src[2*i];
		i++;
	}
	while(i < len)
	{
		dst[i] = src[i] + multiplier*src[2*i - len];
		i++;
	}
}

void GenOctaves(Span<float>& srcResult, Span<float> buffer, uint octavesCount, uint maxSampleDelay)
{
	if(octavesCount == 1) return;
	if(octavesCount == 2)
	{
		SelfOctaveMix(srcResult, buffer, 0.5f);
		srcResult = buffer;
		return;
	}

	Random::FastUniform<uint> rand(3453411347u ^ uint(srcResult.Length()) ^ uint(buffer.Length() << 10) ^ (octavesCount << 20));
	buffer = DecimateX2Linear(buffer, srcResult);
	float volume = 0.5f;
	while(octavesCount --> 1)
	{
		auto result = srcResult;
		result.PopFirstN(AddMultiplied(result, buffer.Drop(rand(maxSampleDelay)), volume));
		while(!result.Empty()) result.PopFirstN(AddMultiplied(result, buffer, volume));
		buffer = DecimateX2LinearInPlace(buffer);
		volume /= 2;
	}
}

}}}
