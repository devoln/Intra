#pragma once

#include "Types.h"

namespace Intra { namespace Audio { namespace Synth {

struct AttackDecayParams
{
	double AttackTime, DecayTime;
};

AttenuationPass CreateAttackDecayPass(double attackTime, double decayTime);

}}}
