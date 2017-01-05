#pragma once

#include "Types.h"

namespace Intra { namespace Audio { namespace Synth {

PostEffectPass CreateLowPass(float rezAmount, float cutoffFreq);
PostEffectPass CreateHighPass(float rezAmount, float cutoffFreq);

}}}
