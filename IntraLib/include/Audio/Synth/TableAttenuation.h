#pragma once

#include "Math/Fixed.h"
#include "Types.h"

namespace Intra { namespace Audio { namespace Synth {

AttenuationPass CreateTableAttenuationPass(ArrayRange<const norm8> table);

}}}
