#pragma once

#include "Types.h"

namespace Intra { namespace Audio { namespace Synth {

//Копирует src в dst с затуханием, пока не кончится либо dst, либо src.
void ExponentialAttenuate(ArrayRange<float>& dst, ArrayRange<const float> src, float& exp, float ek);
void ExponentialAttenuateAdd(ArrayRange<float>& dst, ArrayRange<const float> src, float& exp, float ek);

AttenuationPass CreateExponentialAttenuationPass(float coeff);

}}}
