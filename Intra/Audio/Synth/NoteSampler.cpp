#include "NoteSampler.h"
#include "Range/Mutation/Fill.h"

namespace Intra { namespace Audio { namespace Synth {

Span<float> NoteSampler::operator()(Span<float> dst, bool add)
{
	if(add && !Modifiers.Empty())
	{
		float tempArr[1024];
		while(!dst.Full())
		{
			fill(tempArr, false);
			for(auto& mod: Modifiers) mod(tempArr);
			WriteTo(tempArr, dst);
		}
		return dst;
	}
	fill(dst, add);
	for(auto& mod: Modifiers) mod(dst);
	return dst.Tail(0);
}

void NoteSampler::fill(Span<float> dst, bool add)
{
	for(size_t i = 0; i < PeriodicSamplers.Length(); i++)
	{
		auto remainder = PeriodicSamplers[i](dst, add);
		if(remainder != null)
		{
			if(!add) FillZeros(remainder);
			PeriodicSamplers.RemoveUnordered(i--);
		}
		add = true;
	}
	for(size_t i = 0; i < GenericSamplers.Length(); i++)
	{
		auto remainder = GenericSamplers[i](dst, add);
		if(remainder != null)
		{
			if(!add) FillZeros(remainder);
			GenericSamplers.RemoveUnordered(i--);
		}
		add = true;
	}
}

}}}
