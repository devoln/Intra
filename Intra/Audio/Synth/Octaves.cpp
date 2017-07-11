#include "Octaves.h"

namespace Intra { namespace Audio { namespace Synth {

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

void GenOctaves(Span<float>& srcResult, Span<float> buffer, uint octavesCount)
{
	if(octavesCount == 1) return;
	if(octavesCount == 2)
	{
		SelfOctaveMix(srcResult, buffer, 0.5f);
		srcResult = buffer;
		return;
	}

	buffer = DecimateX2Linear(srcResult, buffer);
	float volume = 0.5f;
	while(octavesCount--> 1)
	{
		auto result = srcResult;
		while(!result.Empty()) result.PopFirstN(AddMultiplied(srcResult, buffer, volume));
		buffer = DecimateX2LinearInPlace(buffer);
		volume /= 2;
	}
}

}}}
