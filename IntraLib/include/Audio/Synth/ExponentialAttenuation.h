#pragma once

#include "Types.h"

namespace Intra { namespace Audio { namespace Synth {

// опирует src в dst с затуханием, пока не кончитс€ либо dst, либо src.
void ExponentialAttenuate(ArrayRange<float>& dst, ArrayRange<const float> src, float& exp, float ek);
void ExponentialAttenuateAdd(ArrayRange<float>& dst, ArrayRange<const float> src, float& exp, float ek);

AttenuationPass CreateExponentialAttenuationPass(float coeff);

}}}
