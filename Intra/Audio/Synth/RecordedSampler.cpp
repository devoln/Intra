#include "RecordedSampler.h"

#include "Range/Mutation/Copy.h"
#include "Range/Mutation/Transform.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio { namespace Synth {

Span<float> RecordedSampler::operator()(Span<float> dst, bool add)
{
	if(!add) MultiplyAdvance(dst, Data, Volume);
	else AddMultipliedAdvance(dst, Data, Volume);
	return dst;
}


}}}

INTRA_WARNING_POP
