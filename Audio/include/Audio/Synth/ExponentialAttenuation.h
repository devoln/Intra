#pragma once

#include "Types.h"

namespace Intra { namespace Audio { namespace Synth {

//Копирует src в dst с затуханием, пока не кончится либо dst, либо src.
void ExponentialAttenuate(Span<float>& dst, CSpan<float> src, float& exp, float ek);
void ExponentialAttenuateAdd(Span<float>& dst, CSpan<float> src, float& exp, float ek);

AttenuationPass CreateExponentialAttenuationPass(float coeff);

}}}
